// From kskalski in
// https://github.com/Kitt-AI/snowboy/commit/651114d5e910e7715bf93d79f96ef3b0dec0a3b7#diff-675f20a7b75e805baec0d16e913ae4e3

#undef _GLIBCXX_USE_CXX11_ABI
#define _GLIBCXX_USE_CXX11_ABI 0

#include "KittAi/snowboy-detect-cxx-compat.h"

static snowboy::SnowboyDetect* c(void* p) {
  return reinterpret_cast<snowboy::SnowboyDetect*>(p);
}
// static const snowboy::SnowboyDetect* c(const void* p) {
//   return reinterpret_cast<const snowboy::SnowboyDetect*>(p);
// }

Snowboy::Snowboy(const char* resourceName, const char* modelPath,
                 const char* sensitivity, float audioGain, bool applyFrontend )
    : detector_(new snowboy::SnowboyDetect(resourceName, modelPath)) {
  c(detector_)->SetSensitivity(sensitivity);
  c(detector_)->SetAudioGain(audioGain);
  c(detector_)->ApplyFrontend(applyFrontend);
}

int Snowboy::SampleRate() const { return c(detector_)->SampleRate(); }

int Snowboy::NumChannels() const { return c(detector_)->NumChannels(); }

int Snowboy::BitsPerSample() const { return c(detector_)->BitsPerSample(); }

int Snowboy::RunDetection(const int16_t* data, int num_samples) {
  return c(detector_)->RunDetection(data, num_samples);
}