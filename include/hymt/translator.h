// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace hymt {

struct TranslatorConfig {
  std::string model_path;
  int32_t n_ctx        = 4096;
  int32_t n_threads    = 0;   // 0 => auto
  int32_t n_batch      = 512;
  int32_t n_gpu_layers = 0;   // 0 = CPU only
  float   temperature  = 0.0f;
  float   top_p        = 0.95f;
  int32_t top_k        = 40;
  float   repeat_penalty = 1.0f;
  uint32_t seed        = 0xDEADBEEF;
  int32_t max_new_tokens = 1024;
  bool    quiet        = true;
};

struct TranslateOptions {
  std::string source_lang;
  std::string target_lang = "en";
  std::string terminology;
  std::string context;
  float   temperature    = -1.0f;
  int32_t max_new_tokens = -1;
};

// Return false to abort generation.
using TokenCallback = std::function<bool(const std::string& piece)>;

class Translator {
 public:
  explicit Translator(const TranslatorConfig& cfg);
  ~Translator();

  Translator(const Translator&)            = delete;
  Translator& operator=(const Translator&) = delete;
  Translator(Translator&&) noexcept;
  Translator& operator=(Translator&&) noexcept;

  std::string Translate(const std::string& text,
                        const TranslateOptions& opt = {});

  std::string TranslateStream(const std::string& text,
                              const TokenCallback& on_token,
                              const TranslateOptions& opt = {});

  struct Stats {
    double prompt_tokens       = 0;
    double generated_tokens    = 0;
    double prefill_ms          = 0;
    double decode_ms           = 0;
    double prefill_tokens_per_s = 0;
    double decode_tokens_per_s  = 0;
  };
  const Stats& last_stats() const { return stats_; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
  Stats stats_;
};

}  // namespace hymt
