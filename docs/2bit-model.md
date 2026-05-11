# Using the AngelSlim 2-bit Model (574MB)

The `Hy-MT1.5-1.8B-2bit-GGUF` model from Tencent's [AngelSlim](https://github.com/Tencent/AngelSlim) toolkit uses a custom **STQ2_0** (Stretched Ternary Quantization) format. This provides excellent quality at just 574MB — ideal for mobile deployment.

## Compatibility Status

| Model | Standard llama.cpp (ggml-org) | Tencent/AngelSlim fork |
|-------|-------------------------------|------------------------|
| HY-MT1.5-1.8B-Q8_0.gguf | ✅ Works | ✅ Works |
| HY-MT1.5-1.8B-Q4_K_M.gguf | ✅ Works | ✅ Works |
| HY-MT1.5-1.8B-Q2_K.gguf | ✅ Works | ✅ Works |
| **Hy-MT1.5-1.8B-2bit** (STQ) | ❌ Not supported | ✅ Works |
| **Hy-MT1.5-1.8B-1.25bit** (STQ) | ❌ Not supported | ✅ Works |

The STQ format is a custom ggml type (`GGML_TYPE_STQ2_0`) that is **not yet merged** into upstream llama.cpp. You must build against the Tencent/AngelSlim fork to load these models.

## Build with AngelSlim STQ Support

### Option A: Use Tencent's fork via CMake (Recommended)

```bash
cd hy-mt-cpp
mkdir build-2bit && cd build-2bit

# Point to Tencent's AngelSlim repo which contains the STQ kernel
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLAMA_CPP_REPO=https://github.com/Tencent/AngelSlim.git \
  -DLLAMA_CPP_TAG=main

make -j$(nproc)
```

### Option B: Use a local AngelSlim clone

```bash
# Clone AngelSlim (which includes llama.cpp with STQ support)
git clone https://github.com/Tencent/AngelSlim.git /path/to/angelslim

# Build hy-mt-cpp pointing to it
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLAMA_CPP_LOCAL=/path/to/angelslim

make -j$(nproc)
```

## Download the 2-bit Model

### From HuggingFace

```bash
# Official Tencent repo (preferred)
huggingface-cli download tencent/Hy-MT1.5-1.8B-2bit-GGUF --local-dir models/

# Or AngelSlim repo
huggingface-cli download AngelSlim/Hy-MT1.5-1.8B-2bit-GGUF --local-dir models/
```

### From ModelScope (China)

```bash
pip install modelscope
python3 -c "
from modelscope import snapshot_download
snapshot_download('AngelSlim/Hy-MT1.5-1.8B-2bit-GGUF', local_dir='./models')
"
```

## Run

```bash
# Find the actual GGUF filename
ls models/*.gguf

# Run translation
./hymt_cli -m models/<filename>.gguf -s zh -t en -i "你好世界" --stats
```

## Expected Performance (2-bit vs Q4_K_M)

| Metric | Q4_K_M (~1.1GB) | 2-bit STQ (~574MB) |
|--------|------------------|---------------------|
| Size | 1.1 GB | 574 MB |
| Quality | Good | Good (QAT-optimized) |
| Speed | ~85 tok/s | ~100+ tok/s (smaller) |
| RAM | ~1.5 GB | ~0.9 GB |
| Mobile | High-end phones | All modern phones |

The 2-bit model uses Quantization-Aware Training (QAT), which means quality is significantly better than post-training Q2_K quantization at the same bit width.

## When Will Standard llama.cpp Support This?

The STQ kernel support is being actively developed. Check:
- https://huggingface.co/tencent/Hy-MT1.5-1.8B-2bit-GGUF/discussions
- https://github.com/ggml-org/llama.cpp/pulls (search for "STQ")

Once merged upstream, you can switch back to the standard build:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release  # defaults to ggml-org/llama.cpp
```

## Troubleshooting

**"unknown type" or "failed to load model" error:**
- You're using standard llama.cpp without STQ support
- Rebuild with `-DLLAMA_CPP_REPO=https://github.com/Tencent/AngelSlim.git`

**Model file is actually JSON (not GGUF):**
- The download URL was wrong. Use `huggingface-cli download` instead of curl.
- Verify with: `head -c 4 model.gguf` — should show `GGUF` magic bytes.
