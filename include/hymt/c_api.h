// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hymt_translator hymt_translator_t;

typedef struct {
  const char* model_path;
  int32_t n_ctx;
  int32_t n_threads;
  int32_t n_batch;
  int32_t n_gpu_layers;
  float   temperature;
  float   top_p;
  int32_t top_k;
  float   repeat_penalty;
  uint32_t seed;
  int32_t max_new_tokens;
  int32_t quiet;
} hymt_config_t;

hymt_config_t hymt_default_config(void);

hymt_translator_t* hymt_create(const hymt_config_t* cfg, char** err_out);
void hymt_destroy(hymt_translator_t* t);

typedef int (*hymt_token_cb)(const char* piece, void* user_data);

int hymt_translate(hymt_translator_t* t,
                   const char* text,
                   const char* source_lang,
                   const char* target_lang,
                   hymt_token_cb on_token,
                   void* user_data,
                   char** out);

typedef struct {
  double prompt_tokens;
  double generated_tokens;
  double prefill_ms;
  double decode_ms;
  double prefill_tokens_per_s;
  double decode_tokens_per_s;
} hymt_stats_t;

void hymt_last_stats(const hymt_translator_t* t, hymt_stats_t* out);
void hymt_free_string(char* s);
const char* hymt_version(void);

#ifdef __cplusplus
}
#endif
