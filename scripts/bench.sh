#!/usr/bin/env bash
# Quick benchmark
set -euo pipefail
MODEL="${1:-./models/hy-mt1.5-1.8b-q8_0.gguf}"
THREADS="${2:-0}"
CLI="./build/hymt_cli"

[ ! -f "$CLI" ] && echo "Build first: mkdir build && cd build && cmake .. && make -j" && exit 1
[ ! -f "$MODEL" ] && echo "Model not found: $MODEL" && exit 1

echo "=== HY-MT1.5 Benchmark (threads=$THREADS) ==="
for text in "你好" "今天天气怎么样？" "机器翻译是自然语言处理中一个重要的研究方向。"; do
  echo "---"
  echo "Input: $text"
  $CLI -m "$MODEL" -s zh -t en -i "$text" --threads "$THREADS" --stats --no-stream
  echo ""
done
