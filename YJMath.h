#pragma once

#include <cmath>

namespace YJMath {

inline float map(float value, float low, float high, float Low, float High) {
  return Low + (High - Low) * ((value - low) / (high - low));
}

inline float lerp(float a, float b, float t) { return (1.0f - t) * a + t * b; }
inline float mtof(float m) { return 8.175799f * powf(2.0f, m / 12.0f); }
inline float ftom(float f) { return 12.0f * log2f(f / 8.175799f); }
inline float dbtoa(float db) { return 1.0f * powf(10.0f, db / 20.0f); }
inline float atodb(float a) { return 20.0f * log10f(a / 1.0f); }
inline float sigmoid(float x) { return 2.0f / (1.0f + expf(-x)) - 1.0f; }
// XXX softclip, etc.

template <typename F>
inline F wrap(F value, F high = 1, F low = 0) {
  jassert(high > low);
  if (value >= high) {
    F range = high - low;
    value -= range;
    if (value >= high) {
      // less common case; must use division
      value -= (F)(unsigned)((value - low) / range);
    }
  } else if (value < low) {
    F range = high - low;
    value += range;
    if (value < low) {
      // less common case; must use division
      value += (F)(unsigned)((high - value) / range);
    }
  }
  return value;
}

/// (0, 1)
inline float sin7(float x) {
    // 7 multiplies + 7 addition/subtraction
    // 14 operations
    return x * (x * (x * (x * (x * (x * (66.5723768716453f * x - 233.003319050759f) + 275.754490892928f) - 106.877929605423f) + 0.156842000875713f) - 9.85899292126983f) + 7.25653181200263f) - 8.88178419700125e-16f;
}

// functor class
class Phasor {
        // 1. Initialize variables to zero (Fixes Silence/Garbage data)
        float frequency_ = 0.0f;
        float offset_    = 0.0f;
        float phase_     = 0.0f;

    public:
        // Constructor
        Phasor(float hertz = 440.0f, float sampleRate = 44100.0f, float offset = 0.0f) 
            : offset_(offset), phase_(0.0f)
        {
            frequency(hertz, sampleRate);
        }

        ~Phasor() = default;

        // Reset (Call in prepareToPlay)
        void reset() {
            phase_ = 0.0f;
            frequency_ = 0.0f;
            offset_ = 0.0f;
        }

        // Set Frequency (With Division by Zero Safety)
        void frequency(float hertz, float sampleRate) {
            if (sampleRate > 0.0f) {
                frequency_ = hertz / sampleRate;
            } else {
                frequency_ = 0.0f;
            }
        }

        // Functor
        float operator()() { return process(); }

        // Process Logic (Inlined here for speed & to avoid linker errors)
        inline float process() {
            if (phase_ >= 1.0f) phase_ -= 1.0f;
            
            float output = phase_ + offset_;
            if (output >= 1.0f) output -= 1.0f;

            phase_ += frequency_;
            return output;
        }
    };

} // namespace YJMath