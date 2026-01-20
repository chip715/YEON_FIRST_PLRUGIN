#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class Phasor 
{
  float frequency_; // normalized frequency
  float offset_;
  float phase_;
  

 public:
 Phasor() = default;

  Phasor(float hertz, float sampleRate, float offset = 0)
      : frequency_(hertz / sampleRate), offset_(offset), phase_(0) {}

      // overload the "call" operator
  float operator()() {
    return process();
  }

  void frequency(float hertz, float sampleRate) {
    frequency_ = hertz / sampleRate;
  }

  float process() {
    // wrap
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;
    }
    float output = phase_ + offset_;
    if (output >= 1.0f) {
      output -= 1.0f;
    }

    phase_ += frequency_; // "side effect" // changes internal state
    return output;
  }
};

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
     // ... all your existing functions ...
        Phasor myPhasor;

        // variables for my Schoffhauzer Waveforms using FM Modulation 
            // 1. The Feedback Memory (must persist between blocks)
            float osc1_history;    // Memory for Sawtooth A x[n-1]
            float osc2_history;    // Memory for Sawtooth B (the offset one)
            float filter_history; // This stores the HF filter history

     
            
            /// (0, 1)
            // Sine approximation function it take number betwee n 0 to 1
            float sin7(float x) 
            {
                return static_cast<float>(x * (x * (x * (x * (x * (x * (66.5723768716453 * x - 233.003319050759) + 275.754490892928) - 106.877929605423) + 0.156842000875713) - 9.85899292126983) + 7.25653181200263) - 8.88178419700125e-16);
            }
            
        
};
