// SPDX-License-Identifier: MIT
package com.hymt

class Translator(modelPath: String, threads: Int = 0, gpuLayers: Int = 0) : AutoCloseable {
    private var handle: Long = nativeCreate(modelPath, threads, gpuLayers)

    fun translate(text: String, srcLang: String = "", tgtLang: String = "en"): String {
        check(handle != 0L) { "Translator closed" }
        return nativeTranslate(handle, text, srcLang, tgtLang)
    }

    data class Stats(val promptTok: Float, val genTok: Float,
                     val prefillMs: Float, val decodeMs: Float,
                     val prefillTps: Float, val decodeTps: Float)

    fun lastStats(): Stats {
        val a = nativeGetStats(handle)
        return Stats(a[0], a[1], a[2], a[3], a[4], a[5])
    }

    override fun close() { if (handle != 0L) { nativeDestroy(handle); handle = 0 } }

    companion object { init { System.loadLibrary("hymt_jni") } }
    private external fun nativeCreate(p: String, t: Int, g: Int): Long
    private external fun nativeDestroy(h: Long)
    private external fun nativeTranslate(h: Long, text: String, src: String, tgt: String): String
    private external fun nativeGetStats(h: Long): FloatArray
}
