// SPDX-License-Identifier: MIT
#include "hymt/c_api.h"
#include "hymt/translator.h"

#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

struct hymt_translator {
  std::unique_ptr<hymt::Translator> impl;
};

static char* dup_str(const std::string& s) {
  char* p = (char*)std::malloc(s.size() + 1);
  if (p) { std::memcpy(p, s.data(), s.size()); p[s.size()] = '\0'; }
  return p;
}

extern "C" {

hymt_config_t hymt_default_config(void) {
  hymt_config_t c{};
  c.model_path     = nullptr;
  c.n_ctx          = 4096;
  c.n_threads      = 0;
  c.n_batch        = 512;
  c.n_gpu_layers   = 0;
  c.temperature    = 0.0f;
  c.top_p          = 0.95f;
  c.top_k          = 40;
  c.repeat_penalty = 1.0f;
  c.seed           = 0xDEADBEEF;
  c.max_new_tokens = 1024;
  c.quiet          = 1;
  return c;
}

hymt_translator_t* hymt_create(const hymt_config_t* cfg, char** err_out) {
  if (!cfg || !cfg->model_path) {
    if (err_out) *err_out = dup_str("model_path is required");
    return nullptr;
  }
  try {
    hymt::TranslatorConfig tc;
    tc.model_path     = cfg->model_path;
    tc.n_ctx          = cfg->n_ctx > 0 ? cfg->n_ctx : 4096;
    tc.n_threads      = cfg->n_threads;
    tc.n_batch        = cfg->n_batch > 0 ? cfg->n_batch : 512;
    tc.n_gpu_layers   = cfg->n_gpu_layers;
    tc.temperature    = cfg->temperature;
    tc.top_p          = cfg->top_p;
    tc.top_k          = cfg->top_k;
    tc.repeat_penalty = cfg->repeat_penalty > 0 ? cfg->repeat_penalty : 1.0f;
    tc.seed           = cfg->seed;
    tc.max_new_tokens = cfg->max_new_tokens > 0 ? cfg->max_new_tokens : 1024;
    tc.quiet          = cfg->quiet != 0;

    auto* h = new hymt_translator;
    h->impl = std::make_unique<hymt::Translator>(tc);
    return h;
  } catch (const std::exception& e) {
    if (err_out) *err_out = dup_str(e.what());
    return nullptr;
  }
}

void hymt_destroy(hymt_translator_t* t) { delete t; }

int hymt_translate(hymt_translator_t* t, const char* text,
                   const char* source_lang, const char* target_lang,
                   hymt_token_cb on_token, void* user_data, char** out) {
  if (!t || !text || !out) return -1;
  try {
    hymt::TranslateOptions opt;
    opt.source_lang = source_lang ? source_lang : "";
    opt.target_lang = target_lang ? target_lang : "en";

    std::string result;
    if (on_token) {
      result = t->impl->TranslateStream(text,
          [&](const std::string& piece) {
            return on_token(piece.c_str(), user_data) != 0;
          }, opt);
    } else {
      result = t->impl->Translate(text, opt);
    }
    *out = dup_str(result);
    return 0;
  } catch (const std::exception& e) {
    *out = dup_str(std::string("ERROR: ") + e.what());
    return -2;
  }
}

void hymt_last_stats(const hymt_translator_t* t, hymt_stats_t* out) {
  if (!t || !out) return;
  const auto& s = t->impl->last_stats();
  out->prompt_tokens        = s.prompt_tokens;
  out->generated_tokens     = s.generated_tokens;
  out->prefill_ms           = s.prefill_ms;
  out->decode_ms            = s.decode_ms;
  out->prefill_tokens_per_s = s.prefill_tokens_per_s;
  out->decode_tokens_per_s  = s.decode_tokens_per_s;
}

void hymt_free_string(char* s) { std::free(s); }
const char* hymt_version(void) { return "0.1.0"; }

}  // extern "C"
