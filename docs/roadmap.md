# Roadmap: ASR -> Translation -> TTS

## Phase 1: Translation Engine (current)
- [x] llama.cpp + HY-MT1.5-1.8B GGUF
- [x] C++ API with streaming
- [x] C ABI for FFI
- [x] CLI tools
- [x] Android/iOS scaffolding

## Phase 2: Mobile App
- [ ] Android app with text translation UI
- [ ] iOS SwiftUI app
- [ ] Test Q4_K_M vs Q8_0 on device

## Phase 3: ASR Integration
- [ ] sherpa-onnx for streaming ASR (SenseVoice / Whisper)
- [ ] VAD (Silero)
- [ ] Pipe ASR output -> translator

## Phase 4: TTS
- [ ] sherpa-onnx TTS (Kokoro / VITS)
- [ ] End-to-end: mic -> ASR -> MT -> TTS -> speaker

## Architecture
```
[Mic] -> [VAD+ASR (sherpa-onnx)] -> [MT (libhymt)] -> [TTS (sherpa-onnx)] -> [Speaker]
```
