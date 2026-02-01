#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "YJMath.h"

// functor class
//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
        , apvts(*this, nullptr, juce::Identifier("APVTS"), createParameters())

{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);\
    karp =YJMath::KarplusStrong(static_cast<float>(sampleRate));
    karp.frequency(440.0f);
    // Reset variables to 0 to ensure clean start

   
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float g = apvts.getParameter("Gain")->getValue();
    float f = apvts.getParameter("currentFrequency_in_midi")->getValue();
    float t = apvts.getParameter("vfilt")->getValue();
    float d = apvts.getParameter("decay")->getValue();
    
    // 0.0 to 1.0
    g = YJMath::dbtoa(YJMath::map(g, 0.0f, 1.0f, -60.0f, 0.0f)); // -60 dB to 0 dB
    f = YJMath::mtof(YJMath::map(f, 0.0f, 1.0f, 36.0f, 96.0f)); // MIDI 36 to 96
    float feedback = YJMath::map(d, 0.0f, 1.0f, 0.95f, 0.999f);

    q.frequency(f, static_cast<float>(getSampleRate()));
    q.virtualfilter(t);
    c.frequency(f, static_cast<float>(getSampleRate()));


                    //    float b[buffer.getNumSamples()]; // allocate array
                    // for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                    //     // static ky::Phasor env;
                    //     // env.frequency(1.0f / 0.5f, static_cast<float>(getSampleRate())); // 0.5 second period
                    //     // float s = q() * g * (1 - env());
                    //     // delayLine.write(s + 0.7 * delayLine.read(getSampleRate() * 0.3f));
                    //     // b[sample] = s + delayLine.read(getSampleRate() * 0.7f);

                    //     b[sample] = c() * g;}



    //Calulaate the KARP
    karp.frequency(f);
    karp.setFeedback(feedback);


auto* leftChannel  = buffer.getWritePointer(0);
auto* rightChannel = (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float output = karp(); // Generate sound (will be silence until button is clicked)
        
        output *= g; // Apply gain

        leftChannel[sample] = output;
        if (rightChannel != nullptr)
            rightChannel[sample] = output;
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters() 
{
   std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
      // Add parameters here, e.g.:
    // layout.add(std::make_unique<juce::AudioParameterFloat>("paramID", "Param Name", 0.0f, 1.0f, 0.5f))
    //sine wave frequency parameter
    //gain parameter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"Gain",1}, "Gain", juce::NormalisableRange<float>(0.0f, 4.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"currentFrequency_in_midi",1}, "currentFrequency_in_midi", juce::NormalisableRange<float>(36.0f, 96.0f, 1.0f), 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"pw", 1}, "pw", juce::NormalisableRange<float>(0.1f, 0.9f, 0.01f), 0.5f));
   params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"vfilt", 1}, "vfilt", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
   params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"decay", 1}, "decay", juce::NormalisableRange<float>(0.0f, 0.999f, 0.01f), 0.5f));
    return {params .begin(), params.end()};
}