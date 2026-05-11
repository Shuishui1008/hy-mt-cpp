// SPDX-License-Identifier: MIT
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
#include "hymt/translator.h"

static void PrintUsage(const char* prog) {
  std::fprintf(stderr,
      "Usage: %s [options]\n\n"
      "Options:\n"
      "  -m, --model PATH       Path to GGUF model (required)\n"
      "  -s, --src LANG         Source language (e.g. zh, en, ja). Empty=auto\n"
      "  -t, --tgt LANG         Target language (default: en)\n"
      "  -i, --text TEXT        Text to translate (reads stdin if omitted)\n"
      "  --threads N            CPU threads (0=auto)\n"
      "  --ctx N                Context size (default: 4096)\n"
      "  --gpu-layers N         GPU offload layers (default: 0)\n"
      "  --temp F               Temperature (0=greedy)\n"
      "  --max-tokens N         Max new tokens (default: 1024)\n"
      "  --no-stream            Disable streaming\n"
      "  --stats                Print perf stats\n"
      "  -h, --help             Show help\n", prog);
}

static std::string ReadStdin() {
  std::string text, line;
  while (std::getline(std::cin, line)) {
    if (!text.empty()) text += '\n';
    text += line;
  }
  return text;
}

int main(int argc, char** argv) {
  hymt::TranslatorConfig cfg;
  hymt::TranslateOptions opt;
  std::string input_text;
  bool stream = true, show_stats = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&]() -> const char* {
      if (i + 1 < argc) return argv[++i];
      std::fprintf(stderr, "Error: %s needs value\n", argv[i]);
      std::exit(1); return "";
    };
    if (arg == "-m" || arg == "--model")    cfg.model_path = next();
    else if (arg == "-s" || arg == "--src") opt.source_lang = next();
    else if (arg == "-t" || arg == "--tgt") opt.target_lang = next();
    else if (arg == "-i" || arg == "--text") input_text = next();
    else if (arg == "--threads")     cfg.n_threads = std::atoi(next());
    else if (arg == "--ctx")         cfg.n_ctx = std::atoi(next());
    else if (arg == "--gpu-layers")  cfg.n_gpu_layers = std::atoi(next());
    else if (arg == "--temp")        cfg.temperature = (float)std::atof(next());
    else if (arg == "--max-tokens")  cfg.max_new_tokens = std::atoi(next());
    else if (arg == "--no-stream")   stream = false;
    else if (arg == "--stats")       show_stats = true;
    else if (arg == "-h" || arg == "--help") { PrintUsage(argv[0]); return 0; }
    else { std::fprintf(stderr, "Unknown: %s\n", argv[i]); return 1; }
  }

  if (cfg.model_path.empty()) {
    std::fprintf(stderr, "Error: --model required\n");
    PrintUsage(argv[0]); return 1;
  }
  if (input_text.empty()) {
    if (isatty(fileno(stdin)))
      std::fprintf(stderr, "Reading stdin (Ctrl+D to end):\n");
    input_text = ReadStdin();
  }
  if (input_text.empty()) {
    std::fprintf(stderr, "Error: no input\n"); return 1;
  }

  std::fprintf(stderr, "Loading: %s\n", cfg.model_path.c_str());
  hymt::Translator translator(cfg);
  std::fprintf(stderr, "Ready.\n");

  std::string result;
  if (stream) {
    result = translator.TranslateStream(input_text,
        [](const std::string& p) { std::fputs(p.c_str(), stdout); std::fflush(stdout); return true; },
        opt);
    std::putchar('\n');
  } else {
    result = translator.Translate(input_text, opt);
    std::puts(result.c_str());
  }

  if (show_stats) {
    const auto& s = translator.last_stats();
    std::fprintf(stderr,
        "\n--- Stats ---\n"
        "  Prompt: %.0f tok | Gen: %.0f tok\n"
        "  Prefill: %.1f ms (%.1f t/s)\n"
        "  Decode:  %.1f ms (%.1f t/s)\n",
        s.prompt_tokens, s.generated_tokens,
        s.prefill_ms, s.prefill_tokens_per_s,
        s.decode_ms, s.decode_tokens_per_s);
  }
  return 0;
}
