// SPDX-License-Identifier: MIT
#include "hymt/translator.h"
#include "prompt.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

#include "llama.h"

namespace hymt {

namespace {

using clock_ = std::chrono::steady_clock;

inline double ms_since(clock_::time_point t0) {
  return std::chrono::duration<double, std::milli>(clock_::now() - t0).count();
}

void SilentLog(ggml_log_level, const char*, void*) {}

int32_t PickThreads(int32_t req) {
  if (req > 0) return req;
  unsigned hc = std::thread::hardware_concurrency();
  return hc > 0 ? static_cast<int32_t>((hc + 1) / 2) : 4;
}

}  // namespace

// -------------------------------------------------------------------------
struct Translator::Impl {
  TranslatorConfig cfg;
  llama_model*     model   = nullptr;
  llama_context*   ctx     = nullptr;
  const llama_vocab* vocab = nullptr;
  llama_sampler*   sampler = nullptr;

  ~Impl() {
    if (sampler) llama_sampler_free(sampler);
    if (ctx)     llama_free(ctx);
    if (model)   llama_model_free(model);
  }

  void BuildSampler() {
    if (sampler) { llama_sampler_free(sampler); sampler = nullptr; }

    auto params = llama_sampler_chain_default_params();
    params.no_perf = false;
    sampler = llama_sampler_chain_init(params);

    if (cfg.repeat_penalty > 1.0f) {
      llama_sampler_chain_add(sampler,
          llama_sampler_init_penalties(64, cfg.repeat_penalty, 0.0f, 0.0f));
    }

    if (cfg.temperature <= 0.0f) {
      llama_sampler_chain_add(sampler, llama_sampler_init_greedy());
    } else {
      if (cfg.top_k > 0)
        llama_sampler_chain_add(sampler, llama_sampler_init_top_k(cfg.top_k));
      if (cfg.top_p > 0.0f && cfg.top_p < 1.0f)
        llama_sampler_chain_add(sampler, llama_sampler_init_top_p(cfg.top_p, 1));
      llama_sampler_chain_add(sampler, llama_sampler_init_temp(cfg.temperature));
      llama_sampler_chain_add(sampler, llama_sampler_init_dist(cfg.seed));
    }
  }

  std::vector<llama_token> Tokenize(const std::string& text, bool add_special) {
    int n_max = (int)text.size() + 64;
    std::vector<llama_token> out(n_max);
    int n = llama_tokenize(vocab, text.c_str(), (int)text.size(),
                           out.data(), n_max, add_special, true);
    if (n < 0) {
      out.resize(-n);
      n = llama_tokenize(vocab, text.c_str(), (int)text.size(),
                         out.data(), (int)out.size(), add_special, true);
      if (n < 0) throw std::runtime_error("tokenize failed");
    }
    out.resize(n);
    return out;
  }

  std::string TokenToPiece(llama_token tok) {
    char buf[256];
    int n = llama_token_to_piece(vocab, tok, buf, sizeof(buf), 0, false);
    if (n < 0) return "";
    return std::string(buf, n);
  }

  std::string ApplyTemplate(const std::string& user_content) {
    llama_chat_message msg{"user", user_content.c_str()};
    const char* tmpl = llama_model_chat_template(model, nullptr);

    int needed = llama_chat_apply_template(tmpl, &msg, 1, true, nullptr, 0);
    if (needed <= 0) {
      // Fallback: generic chatml
      return "<|im_start|>user\n" + user_content + "<|im_end|>\n<|im_start|>assistant\n";
    }
    std::string out(needed + 1, '\0');
    int written = llama_chat_apply_template(tmpl, &msg, 1, true,
                                            &out[0], (int)out.size());
    if (written < 0) throw std::runtime_error("chat template failed");
    out.resize(written);
    return out;
  }
};

// -------------------------------------------------------------------------
Translator::Translator(const TranslatorConfig& cfg) : impl_(new Impl()) {
  impl_->cfg = cfg;

  if (cfg.quiet) llama_log_set(SilentLog, nullptr);

  static bool inited = false;
  if (!inited) { llama_backend_init(); inited = true; }

  // Load model
  llama_model_params mp = llama_model_default_params();
  mp.n_gpu_layers = cfg.n_gpu_layers;

  impl_->model = llama_model_load_from_file(cfg.model_path.c_str(), mp);
  if (!impl_->model)
    throw std::runtime_error("failed to load model: " + cfg.model_path);

  impl_->vocab = llama_model_get_vocab(impl_->model);

  // Create context
  llama_context_params cp = llama_context_default_params();
  cp.n_ctx           = cfg.n_ctx;
  cp.n_batch         = cfg.n_batch;
  cp.n_threads       = PickThreads(cfg.n_threads);
  cp.n_threads_batch = cp.n_threads;

  impl_->ctx = llama_init_from_model(impl_->model, cp);
  if (!impl_->ctx)
    throw std::runtime_error("failed to create context");

  impl_->BuildSampler();
}

Translator::~Translator() = default;
Translator::Translator(Translator&&) noexcept = default;
Translator& Translator::operator=(Translator&&) noexcept = default;

std::string Translator::Translate(const std::string& text,
                                  const TranslateOptions& opt) {
  return TranslateStream(text, nullptr, opt);
}

std::string Translator::TranslateStream(const std::string& text,
                                        const TokenCallback& on_token,
                                        const TranslateOptions& opt) {
  const int32_t max_new = opt.max_new_tokens >= 0
                            ? opt.max_new_tokens : impl_->cfg.max_new_tokens;

  // Build prompt
  std::string user = BuildUserPrompt(text, opt.source_lang, opt.target_lang,
                                     opt.terminology, opt.context);
  std::string prompt = impl_->ApplyTemplate(user);

  // Tokenize
  auto tokens = impl_->Tokenize(prompt, false);
  if (tokens.empty()) throw std::runtime_error("empty prompt");

  if ((int32_t)tokens.size() + max_new > impl_->cfg.n_ctx)
    throw std::runtime_error("prompt + max_new_tokens exceeds n_ctx");

  // Clear KV cache
  llama_memory_clear(llama_get_memory(impl_->ctx), true);

  // Prefill
  auto t0 = clock_::now();
  llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size());
  if (llama_decode(impl_->ctx, batch) != 0)
    throw std::runtime_error("decode (prefill) failed");
  double prefill_ms = ms_since(t0);

  // Generate
  std::string result;
  int n_gen = 0;
  auto t1 = clock_::now();

  while (n_gen < max_new) {
    llama_token id = llama_sampler_sample(impl_->sampler, impl_->ctx, -1);
    llama_sampler_accept(impl_->sampler, id);

    if (llama_vocab_is_eog(impl_->vocab, id)) break;

    std::string piece = impl_->TokenToPiece(id);
    if (!piece.empty()) {
      result += piece;
      if (on_token && !on_token(piece)) break;
    }

    llama_batch nb = llama_batch_get_one(&id, 1);
    if (llama_decode(impl_->ctx, nb) != 0)
      throw std::runtime_error("decode (gen) failed");
    ++n_gen;
  }

  double decode_ms = ms_since(t1);

  stats_.prompt_tokens       = (double)tokens.size();
  stats_.generated_tokens    = (double)n_gen;
  stats_.prefill_ms          = prefill_ms;
  stats_.decode_ms           = decode_ms;
  stats_.prefill_tokens_per_s = prefill_ms > 0 ? tokens.size() * 1000.0 / prefill_ms : 0;
  stats_.decode_tokens_per_s  = decode_ms  > 0 ? n_gen * 1000.0 / decode_ms : 0;

  return result;
}

}  // namespace hymt
