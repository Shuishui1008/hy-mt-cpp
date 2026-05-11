// SPDX-License-Identifier: MIT
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include "hymt/translator.h"

int main(int argc, char** argv) {
  hymt::TranslatorConfig cfg;
  std::string src_lang, tgt_lang = "en";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&]() -> const char* {
      if (i + 1 < argc) return argv[++i];
      std::fprintf(stderr, "%s needs value\n", argv[i]);
      std::exit(1); return "";
    };
    if (arg == "-m" || arg == "--model")    cfg.model_path = next();
    else if (arg == "-s" || arg == "--src") src_lang = next();
    else if (arg == "-t" || arg == "--tgt") tgt_lang = next();
    else if (arg == "--threads")    cfg.n_threads = std::atoi(next());
    else if (arg == "--ctx")        cfg.n_ctx = std::atoi(next());
    else if (arg == "--gpu-layers") cfg.n_gpu_layers = std::atoi(next());
    else if (arg == "--temp")       cfg.temperature = (float)std::atof(next());
    else if (arg == "--max-tokens") cfg.max_new_tokens = std::atoi(next());
  }

  if (cfg.model_path.empty()) {
    std::fprintf(stderr, "Usage: hymt_repl -m <model.gguf> [--src X] [--tgt X]\n");
    return 1;
  }

  std::fprintf(stderr, "Loading: %s ...\n", cfg.model_path.c_str());
  hymt::Translator translator(cfg);
  std::fprintf(stderr, "Ready. Type !quit to exit, !src/!tgt to change lang, !stats toggle.\n\n");

  bool show_stats = false;
  std::string line;

  while (true) {
    std::fprintf(stderr, "[%s->%s] > ",
                 src_lang.empty() ? "auto" : src_lang.c_str(), tgt_lang.c_str());
    std::fflush(stderr);
    if (!std::getline(std::cin, line)) break;
    if (line.empty()) continue;
    if (line == "!quit" || line == "!q") break;
    if (line.rfind("!src ", 0) == 0) { src_lang = line.substr(5); continue; }
    if (line.rfind("!tgt ", 0) == 0) { tgt_lang = line.substr(5); continue; }
    if (line == "!stats") { show_stats = !show_stats; continue; }

    hymt::TranslateOptions opt;
    opt.source_lang = src_lang;
    opt.target_lang = tgt_lang;

    std::fprintf(stdout, "  ");
    translator.TranslateStream(line,
        [](const std::string& p) { std::fputs(p.c_str(), stdout); std::fflush(stdout); return true; },
        opt);
    std::putchar('\n');

    if (show_stats) {
      const auto& s = translator.last_stats();
      std::fprintf(stderr, "  [prefill %.0fms (%.0f t/s) | decode %.0fms (%.1f t/s)]\n",
                   s.prefill_ms, s.prefill_tokens_per_s, s.decode_ms, s.decode_tokens_per_s);
    }
  }
  return 0;
}
