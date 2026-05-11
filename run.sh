#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# run.sh — Build & run hy-mt-cpp in different modes.
#
# Usage:
#   ./run.sh build          Build for x86 (default, standard llama.cpp)
#   ./run.sh build-stq      Build for ARM with STQ1_0 support (cross-compile)
#   ./run.sh run [args...]  Run translation with the x86 build
#   ./run.sh bench          Quick benchmark (x86)
#   ./run.sh clean          Remove all build dirs
#
# Environment variables:
#   ANDROID_NDK       Path to Android NDK (required for build-stq)
#   MODEL             Override model path for 'run' command
#   THREADS           Number of threads (default: auto)

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODEL_DIR="${PROJECT_DIR}/models"
BUILD_X86="${PROJECT_DIR}/build"
BUILD_STQ="${PROJECT_DIR}/build-stq"
LLAMA_STQ_DIR="${PROJECT_DIR}/third_party/llama-stq"

# Default model for x86
MODEL_X86="${MODEL:-${MODEL_DIR}/HY-MT1.5-1.8B-Q4_K_M.gguf}"

# STQ PR branch info
LLAMA_CPP_REPO="https://github.com/ggml-org/llama.cpp.git"
LLAMA_CPP_PR="pull/22836/head"
LLAMA_CPP_BRANCH="stq-support"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ============================================================================
# build: x86 native compilation with standard llama.cpp (Q4_K_M / Q8_0 / Q2_K)
# ============================================================================
cmd_build() {
  info "=== Building for x86_64 (standard llama.cpp) ==="

  mkdir -p "$BUILD_X86"
  cmake -B "$BUILD_X86" -S "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DHYMT_USE_OPENMP=ON

  cmake --build "$BUILD_X86" -j"$(nproc)"

  info "Build complete: $BUILD_X86/hymt_cli"
  info ""
  info "Download model (if not already present):"
  info "  ./scripts/download_model.sh Q4_K_M"
  info ""
  info "Run:"
  info "  ./run.sh run -s zh -t en -i \"你好世界\" --stats"
}

# ============================================================================
# build-stq: ARM cross-compile with STQ1_0 support (for Android arm64)
# ============================================================================
cmd_build_stq() {
  info "=== Building for ARM64 with STQ1_0 support (Android cross-compile) ==="

  # ---- Step 1: Clone llama.cpp PR #22836 (STQ support) ----
  if [ ! -d "$LLAMA_STQ_DIR" ]; then
    info "Cloning llama.cpp with STQ support (PR #22836)..."
    mkdir -p "$(dirname "$LLAMA_STQ_DIR")"
    git clone --depth=50 "$LLAMA_CPP_REPO" "$LLAMA_STQ_DIR"
    pushd "$LLAMA_STQ_DIR" > /dev/null
    git fetch origin "$LLAMA_CPP_PR":"$LLAMA_CPP_BRANCH"
    git checkout "$LLAMA_CPP_BRANCH"
    popd > /dev/null
    info "llama.cpp (STQ branch) ready at: $LLAMA_STQ_DIR"
  else
    info "Using existing llama.cpp STQ: $LLAMA_STQ_DIR"
    pushd "$LLAMA_STQ_DIR" > /dev/null
    git checkout "$LLAMA_CPP_BRANCH" 2>/dev/null || true
    popd > /dev/null
  fi

  # ---- Step 2: Detect Android NDK ----
  if [ -z "${ANDROID_NDK:-}" ]; then
    # Try common locations
    for candidate in \
      "$HOME/Android/Sdk/ndk"/*/ \
      "$HOME/Library/Android/sdk/ndk"/*/ \
      /opt/android-ndk*/ \
      "$ANDROID_HOME/ndk"/*/ ; do
      if [ -f "${candidate}build/cmake/android.toolchain.cmake" ] 2>/dev/null; then
        ANDROID_NDK="$(cd "$candidate" && pwd)"
        break
      fi
    done
  fi

  if [ -z "${ANDROID_NDK:-}" ]; then
    error "ANDROID_NDK not set and not found in common locations."
    error ""
    error "Please set it:"
    error "  export ANDROID_NDK=/path/to/android-ndk-r26d"
    error "  ./run.sh build-stq"
    error ""
    error "Or install via Android Studio: SDK Manager -> SDK Tools -> NDK"
    exit 1
  fi

  local TOOLCHAIN="${ANDROID_NDK}/build/cmake/android.toolchain.cmake"
  if [ ! -f "$TOOLCHAIN" ]; then
    error "Toolchain not found: $TOOLCHAIN"
    error "Check your ANDROID_NDK path: $ANDROID_NDK"
    exit 1
  fi

  info "Using NDK: $ANDROID_NDK"

  # ---- Step 3: CMake cross-compile for arm64-v8a ----
  mkdir -p "$BUILD_STQ"
  cmake -B "$BUILD_STQ" -S "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-26 \
    -DCMAKE_BUILD_TYPE=Release \
    -DHYMT_USE_OPENMP=ON \
    -DHYMT_USE_METAL=OFF \
    -DHYMT_BUILD_C_API=ON \
    -DHYMT_BUILD_CLI=ON \
    -DLLAMA_CPP_LOCAL="$LLAMA_STQ_DIR"

  cmake --build "$BUILD_STQ" -j"$(nproc)"

  info ""
  info "=== ARM64 build complete! ==="
  info "Binaries: $BUILD_STQ/hymt_cli (arm64-v8a)"
  info "Shared lib: $BUILD_STQ/libhymt.so (for Android app)"
  info ""
  info "To test on device:"
  info "  adb push $BUILD_STQ/hymt_cli /data/local/tmp/"
  info "  adb push models/Hy-MT1.5-1.8B-1.25bit.gguf /data/local/tmp/"
  info "  adb shell /data/local/tmp/hymt_cli \\"
  info "    -m /data/local/tmp/Hy-MT1.5-1.8B-1.25bit.gguf \\"
  info "    -s zh -t en -i \"你好世界\" --stats"
  info ""
  info "Model download (1.25-bit STQ, 440MB — smallest, ARM-optimized):"
  info "  huggingface-cli download tencent/Hy-MT1.5-1.8B-1.25bit-GGUF --local-dir models/"
}

# ============================================================================
# run: Execute translation with x86 build
# ============================================================================
cmd_run() {
  local CLI="$BUILD_X86/hymt_cli"

  if [ ! -f "$CLI" ]; then
    error "x86 build not found. Run: ./run.sh build"
    exit 1
  fi

  if [ ! -f "$MODEL_X86" ]; then
    error "Model not found: $MODEL_X86"
    error ""
    error "Download it:"
    error "  ./scripts/download_model.sh Q4_K_M"
    error ""
    error "Or set MODEL env var:"
    error "  MODEL=models/other.gguf ./run.sh run -s zh -t en -i \"text\""
    exit 1
  fi

  # Run with LD_LIBRARY_PATH pointing to the shared libs
  LD_LIBRARY_PATH="${BUILD_X86}/bin:${BUILD_X86}:${LD_LIBRARY_PATH:-}" \
    "$CLI" -m "$MODEL_X86" "$@"
}

# ============================================================================
# bench: Quick benchmark
# ============================================================================
cmd_bench() {
  local CLI="$BUILD_X86/hymt_cli"

  if [ ! -f "$CLI" ]; then
    error "x86 build not found. Run: ./run.sh build"
    exit 1
  fi

  if [ ! -f "$MODEL_X86" ]; then
    error "Model not found: $MODEL_X86"
    error "Run: ./scripts/download_model.sh Q4_K_M"
    exit 1
  fi

  info "=== HY-MT1.5-1.8B Benchmark ==="
  info "Model: $MODEL_X86"
  info "CPU: $(grep -m1 'model name' /proc/cpuinfo 2>/dev/null | cut -d: -f2 || echo 'unknown')"
  info ""

  local sentences=(
    "你好"
    "今天天气怎么样？"
    "机器翻译是自然语言处理中一个重要的研究方向。"
    "随着深度学习技术的快速发展，神经机器翻译已经在很多语言对上超越了传统的统计方法。"
  )

  for text in "${sentences[@]}"; do
    echo "---"
    echo "Input: $text"
    LD_LIBRARY_PATH="${BUILD_X86}/bin:${BUILD_X86}:${LD_LIBRARY_PATH:-}" \
      "$CLI" -m "$MODEL_X86" -s zh -t en -i "$text" --stats --no-stream 2>&1
    echo ""
  done

  info "=== Benchmark complete ==="
}

# ============================================================================
# clean
# ============================================================================
cmd_clean() {
  info "Cleaning build directories..."
  rm -rf "$BUILD_X86" "$BUILD_STQ"
  info "Done. (third_party/ and models/ preserved)"
}

# ============================================================================
# help
# ============================================================================
cmd_help() {
  cat <<'EOF'
hy-mt-cpp build & run script

Usage:
  ./run.sh build          Build for x86_64 (standard llama.cpp, Q4_K_M/Q8_0/Q2_K)
  ./run.sh build-stq      Build for ARM64 with STQ1_0 (Android cross-compile, 1.25-bit)
  ./run.sh run [args...]  Run translation (x86 build)
  ./run.sh bench          Quick benchmark (x86)
  ./run.sh clean          Remove build dirs

Examples:
  ./run.sh build
  ./run.sh run -s zh -t en -i "你好世界" --stats
  ./run.sh run -s en -t ja -i "Hello world"
  MODEL=models/Q8_0.gguf ./run.sh run -s zh -t en -i "测试"

  export ANDROID_NDK=/opt/android-ndk-r26d
  ./run.sh build-stq

Environment:
  ANDROID_NDK   Path to Android NDK (for build-stq)
  MODEL         Override model path (for run/bench)
  THREADS       Number of threads
EOF
}

# ============================================================================
# Main dispatch
# ============================================================================
case "${1:-help}" in
  build)      cmd_build ;;
  build-stq)  cmd_build_stq ;;
  run)        shift; cmd_run "$@" ;;
  bench)      cmd_bench ;;
  clean)      cmd_clean ;;
  -h|--help|help) cmd_help ;;
  *)
    error "Unknown command: $1"
    cmd_help
    exit 1
    ;;
esac
