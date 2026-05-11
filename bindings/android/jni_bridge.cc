// SPDX-License-Identifier: MIT
#include <jni.h>
#include <string>
#include "hymt/c_api.h"

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_hymt_Translator_nativeCreate(JNIEnv* env, jobject,
                                      jstring model_path, jint threads, jint gpu) {
  const char* path = env->GetStringUTFChars(model_path, nullptr);
  hymt_config_t cfg = hymt_default_config();
  cfg.model_path = path;
  cfg.n_threads = threads;
  cfg.n_gpu_layers = gpu;
  char* err = nullptr;
  auto* t = hymt_create(&cfg, &err);
  env->ReleaseStringUTFChars(model_path, path);
  if (!t) {
    jclass ex = env->FindClass("java/lang/RuntimeException");
    env->ThrowNew(ex, err ? err : "create failed");
    if (err) hymt_free_string(err);
    return 0;
  }
  return reinterpret_cast<jlong>(t);
}

JNIEXPORT void JNICALL
Java_com_hymt_Translator_nativeDestroy(JNIEnv*, jobject, jlong h) {
  hymt_destroy(reinterpret_cast<hymt_translator_t*>(h));
}

JNIEXPORT jstring JNICALL
Java_com_hymt_Translator_nativeTranslate(JNIEnv* env, jobject, jlong h,
                                         jstring text, jstring src, jstring tgt) {
  auto* t = reinterpret_cast<hymt_translator_t*>(h);
  const char* c_text = env->GetStringUTFChars(text, nullptr);
  const char* c_src = src ? env->GetStringUTFChars(src, nullptr) : "";
  const char* c_tgt = tgt ? env->GetStringUTFChars(tgt, nullptr) : "en";
  char* out = nullptr;
  hymt_translate(t, c_text, c_src, c_tgt, nullptr, nullptr, &out);
  env->ReleaseStringUTFChars(text, c_text);
  if (src) env->ReleaseStringUTFChars(src, c_src);
  if (tgt) env->ReleaseStringUTFChars(tgt, c_tgt);
  jstring result = out ? env->NewStringUTF(out) : env->NewStringUTF("");
  if (out) hymt_free_string(out);
  return result;
}

JNIEXPORT jfloatArray JNICALL
Java_com_hymt_Translator_nativeGetStats(JNIEnv* env, jobject, jlong h) {
  hymt_stats_t s{};
  hymt_last_stats(reinterpret_cast<hymt_translator_t*>(h), &s);
  jfloatArray arr = env->NewFloatArray(6);
  float buf[6] = {(float)s.prompt_tokens, (float)s.generated_tokens,
                  (float)s.prefill_ms, (float)s.decode_ms,
                  (float)s.prefill_tokens_per_s, (float)s.decode_tokens_per_s};
  env->SetFloatArrayRegion(arr, 0, 6, buf);
  return arr;
}

}
