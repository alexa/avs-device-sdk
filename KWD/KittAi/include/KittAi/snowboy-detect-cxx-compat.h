#pragma once

#include <cstdint>

#include "snowboy-detect.h"

class Snowboy {
 public:
  struct Model {
    const char* filename;
    float sensitivity;
  };

  Snowboy(const char* resourceName, const char* modelPath,
          const char* sensitivity, float audioGain, bool applyFrontend);

  int SampleRate() const;
  int NumChannels() const;
  int BitsPerSample() const;

  int RunDetection(const int16_t* data, int num_samples);

 private:
  void* detector_;
};