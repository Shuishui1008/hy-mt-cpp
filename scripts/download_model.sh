#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Download HY-MT1.5-1.8B GGUF model
set -euo pipefail

QUANT="${1:-Q8_0}"
SOURCE="${2:-hf}"
MODEL_DIR="$(cd "$(dirname "$0")/.." && pwd)/models"
mkdir -p "$MODEL_DIR"

# Try multiple filename conventions (HF repos vary in naming)
QUANT_LOWER=$(echo "$QUANT" | tr '[:upper:]' '[:lower:]')
QUANT_UPPER=$(echo "$QUANT" | tr '[:lower:]' '[:upper:]')

# Possible filenames on HuggingFace
HF_NAMES=(
  "hy-mt1.5-1.8b-${QUANT_LOWER}.gguf"
  "HY-MT1.5-1.8B-${QUANT_UPPER}.gguf"
  "hy-mt1.5-1.8b.${QUANT_UPPER}.gguf"
)

HF_REPO="tencent/HY-MT1.5-1.8B-GGUF"
MS_REPO="Tencent-Hunyuan/HY-MT1.5-1.8B-GGUF"

DEST="${MODEL_DIR}/hy-mt1.5-1.8b-${QUANT_LOWER}.gguf"
[ -f "$DEST" ] && echo "Exists: $DEST" && exit 0

echo "============================================="
echo "  HY-MT1.5-1.8B GGUF Downloader"
echo "  Quant  : ${QUANT}"
echo "  Source : ${SOURCE}"
echo "============================================="

# ---------------------------------------------------------------------------
# Method 1: huggingface-cli (best — handles auth, resume, correct filenames)
# ---------------------------------------------------------------------------
try_hf_cli() {
  if command -v huggingface-cli &>/dev/null; then
    echo ">>> Using huggingface-cli..."
    huggingface-cli download "$HF_REPO" \
      --include "*${QUANT_UPPER}*" "*${QUANT_LOWER}*" \
      --local-dir "$MODEL_DIR" --local-dir-use-symlinks False
    # Find and rename to our standard name
    local found=$(find "$MODEL_DIR" -maxdepth 1 -iname "*${QUANT_LOWER}*.gguf" -o -iname "*${QUANT_UPPER}*.gguf" | head -1)
    if [ -n "$found" ] && [ "$found" != "$DEST" ]; then
      mv "$found" "$DEST"
    fi
    [ -f "$DEST" ] && return 0
  fi
  return 1
}

# ---------------------------------------------------------------------------
# Method 2: modelscope SDK (for mainland China)
# ---------------------------------------------------------------------------
try_ms_sdk() {
  if python3 -c "import modelscope" &>/dev/null 2>&1; then
    echo ">>> Using modelscope SDK..."
    python3 -c "
from modelscope.hub.snapshot_download import snapshot_download
import os, glob

local_dir = '${MODEL_DIR}'
snapshot_download('${MS_REPO}', local_dir=local_dir,
                  allow_patterns=['*${QUANT_UPPER}*', '*${QUANT_LOWER}*', '*.gguf'])

# Find the downloaded file
for pattern in ['*${QUANT_LOWER}*.gguf', '*${QUANT_UPPER}*.gguf']:
    found = glob.glob(os.path.join(local_dir, '**', pattern), recursive=True)
    if found:
        target = '${DEST}'
        if found[0] != target:
            os.rename(found[0], target)
        print(f'Downloaded: {target}')
        break
"
    [ -f "$DEST" ] && return 0
  fi
  return 1
}

# ---------------------------------------------------------------------------
# Method 3: Direct curl with multiple URL attempts
# ---------------------------------------------------------------------------
try_curl() {
  local urls=()
  if [ "$SOURCE" = "ms" ]; then
    for name in "${HF_NAMES[@]}"; do
      urls+=("https://modelscope.cn/models/${MS_REPO}/resolve/master/${name}")
    done
  else
    for name in "${HF_NAMES[@]}"; do
      urls+=("https://huggingface.co/${HF_REPO}/resolve/main/${name}")
    done
  fi

  for url in "${urls[@]}"; do
    echo ">>> Trying: $url"
    local http_code
    http_code=$(curl -sL -o "$DEST" -w "%{http_code}" "$url")
    if [ "$http_code" = "200" ]; then
      # Verify it's actually a GGUF file (starts with "GGUF" magic bytes)
      local magic=$(head -c 4 "$DEST" 2>/dev/null | cat -v)
      if echo "$magic" | grep -q "GGUF"; then
        echo ">>> Success!"
        return 0
      else
        echo ">>> Got HTTP 200 but file is not valid GGUF, trying next..."
        rm -f "$DEST"
      fi
    else
      echo ">>> HTTP $http_code, trying next..."
      rm -f "$DEST"
    fi
  done
  return 1
}

# ---------------------------------------------------------------------------
# Main: try methods in order
# ---------------------------------------------------------------------------
if [ "$SOURCE" = "ms" ]; then
  try_ms_sdk || try_curl || {
    echo ""
    echo "ERROR: Failed to download from ModelScope."
    echo ""
    echo "Manual options:"
    echo "  1. pip install modelscope && python -c \\"
    echo "     \"from modelscope.hub.snapshot_download import snapshot_download; \\"
    echo "      snapshot_download('${MS_REPO}', local_dir='${MODEL_DIR}')\""
    echo ""
    echo "  2. Visit https://modelscope.cn/models/${MS_REPO}/files"
    echo "     and download the GGUF file manually to: ${MODEL_DIR}/"
    echo ""
    echo "  3. Try HuggingFace instead: $0 ${QUANT} hf"
    exit 1
  }
else
  try_hf_cli || try_curl || {
    echo ""
    echo "ERROR: Failed to download from HuggingFace."
    echo ""
    echo "Manual options:"
    echo "  1. pip install huggingface_hub && huggingface-cli download \\"
    echo "     ${HF_REPO} --include '*${QUANT}*' --local-dir ${MODEL_DIR}/"
    echo ""
    echo "  2. Visit https://huggingface.co/${HF_REPO}/tree/main"
    echo "     and download the Q2_K GGUF file manually to: ${MODEL_DIR}/"
    echo ""
    echo "  3. Try ModelScope: $0 ${QUANT} ms"
    exit 1
  }
fi

echo ""
echo ">>> Done! Model saved to: $DEST"
echo ""
echo "Test:"
echo "  ./build/hymt_cli -m ${DEST} -s zh -t en -i \"你好世界\" --stats"
