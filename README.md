# hy-mt-cpp

Minimal C++ inference engine for [Tencent HY-MT1.5-1.8B](https://huggingface.co/tencent/HY-MT1.5-1.8B-GGUF) translation model (GGUF), powered by [llama.cpp](https://github.com/ggml-org/llama.cpp).

**33 languages** mutual translation on CPU. Optional Metal/CUDA/Vulkan acceleration.

## Quick Start

```bash
git clone https://github.com/Shuishui1008/hy-mt-cpp.git
cd hy-mt-cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Download model (Q8_0 ~1.8GB)
../scripts/download_model.sh Q8_0

# Translate
./hymt_cli -m ../models/hy-mt1.5-1.8b-q8_0.gguf -s zh -t en -i "你好世界" --stats
```

## Features

- Streaming token-by-token output
- C++ API (RAII) + C ABI (for JNI/Swift/ctypes)
- CLI: one-shot (`hymt_cli`) + interactive REPL (`hymt_repl`)
- Android JNI + Kotlin wrapper
- iOS Objective-C bridge
- Greedy / top-k / top-p sampling
- Performance stats (prefill/decode tok/s)

## Model Variants

| Quant | Size | Quality | Use Case |
|-------|------|---------|----------|
| Q8_0 | ~1.8GB | Best | Desktop benchmark |
| Q4_K_M | ~1.1GB | Good | Mobile (high-end) |
| Q2_K | ~0.7GB | OK | Mobile (all) |
| **2-bit STQ** | ~574MB | Good (QAT) | Mobile (all) — requires AngelSlim build |
| **1.25-bit STQ** | ~440MB | Decent (QAT) | Ultra-light edge — requires AngelSlim build |

> **Note:** The 2-bit and 1.25-bit models use Tencent's custom STQ quantization.
> See [docs/2bit-model.md](docs/2bit-model.md) for build instructions.

## Build Options

```bash
cmake .. -DHYMT_USE_METAL=ON    # macOS Metal
cmake .. -DHYMT_USE_CUDA=ON     # NVIDIA GPU
cmake .. -DHYMT_USE_VULKAN=ON   # Cross-platform GPU
cmake .. -DHYMT_USE_OPENMP=OFF  # Disable OpenMP
cmake .. -DLLAMA_CPP_TAG=b5000  # Pin llama.cpp version

# For AngelSlim 2-bit STQ model support:
cmake .. -DLLAMA_CPP_REPO=https://github.com/Tencent/AngelSlim.git -DLLAMA_CPP_TAG=main
```

## Roadmap

See [docs/roadmap.md](docs/roadmap.md) — planned: ASR (sherpa-onnx) -> MT -> TTS pipeline for real-time on-device speech translation.

## License

MIT
