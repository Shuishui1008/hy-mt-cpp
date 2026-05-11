#!/usr/bin/env bash
# Download HY-MT1.5-1.8B GGUF model
set -euo pipefail

QUANT="${1:-Q8_0}"
SOURCE="${2:-hf}"
MODEL_DIR="$(cd "$(dirname "$0")/.." && pwd)/models"
mkdir -p "$MODEL_DIR"

FILENAME="hy-mt1.5-1.8b-${QUANT,,}.gguf"
HF_URL="https://huggingface.co/tencent/HY-MT1.5-1.8B-GGUF/resolve/main/${FILENAME}"
MS_URL="https://www.modelscope.cn/models/Tencent-Hunyuan/HY-MT1.5-1.8B-GGUF/resolve/master/${FILENAME}"

DEST="${MODEL_DIR}/${FILENAME}"
[ -f "$DEST" ] && echo "Exists: $DEST" && exit 0

echo "Downloading ${FILENAME} from ${SOURCE}..."
URL="$HF_URL"
[ "$SOURCE" = "ms" ] && URL="$MS_URL"

if command -v curl &>/dev/null; then
  curl -L --progress-bar -o "$DEST" "$URL"
else
  wget --show-progress -O "$DEST" "$URL"
fi
echo "Done: $DEST"
