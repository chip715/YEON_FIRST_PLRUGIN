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
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    // Reset variables to 0 to ensure clean start
    osc1_history = 0.0f;
    osc2_history = 0.0f;
    filter_history = 0.0f;
    myPhasor.reset();

   
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

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.


    //Clear the buffer for  output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    //get sample rate
    float sampleRate = static_cast<float>(getSampleRate());
       
        //list of variables to get from apvts
    float gain = apvts.getRawParameterValue("Gain")->load();
    float sineCurrentFrequency = apvts.getRawParameterValue("sineCurrentFrequency")->load();
    float pw= apvts.getRawParameterValue("pw")->load();; // pulse width of the pulse, 0..1

    // Update phasor frequency
    myPhasor.frequency(sineCurrentFrequency, sampleRate);
    float w = sineCurrentFrequency / sampleRate;

    //schoffhauzer Scaling calculations
    float n = std::max(0.0f,0.5f - w);
    float scaling = 13.0f * n * n * n * n; //calculate scaling but clamping to prevent nyquist ringing
    //float DC = 0.376f - w*0.752f; // DC compensation

    float norm = 1.0f - 2.0f * w; // calculate normalization


    // variables and constants
    float const a0 = 2.5f; // precalculated coeffs
    float const a1 = -1.5f; // for HF compensation
    
   //main audio buffer pointer for faster access
    auto* ChannelInfoForCopy = buffer.getWritePointer(0);

    //Synthesis loop
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) 
    {
        //Get phasor value
        float phase = myPhasor.process();//get phasor value

        //Ocilator 1 (reference)
        //The formula from the reading is x[n]= (x[x[n-1] + sin(phase + scaling* x[n-1])) /2
        float feedback1= phase+(osc1_history*scaling); //feedback from last output
        float osc1 = std::sin(feedback1*juce::MathConstants<float>::twoPi); //calculate sinewave in raidans
        float out1 = (osc1 + osc1_history) * 0.5f; //final output with naive integrator
        osc1_history = out1; //store history

        // //Ocilator 2 (phase offset)
        float phase2_raw = phase + pw; //add pulse width offset
        if(phase2_raw >= 1.0f) phase2_raw -= 1.0f; //wrap

        float feedback2= phase2_raw+(osc2_history*scaling); //feedback from last output
        float osc2 = std::sin(feedback2*juce::MathConstants<float>::twoPi); //calculate sinewave in raidans
        float out2 = (osc2 + osc2_history) * 0.5f; //final output with naive integrator
        osc2_history = out2; //store history

        // Subtract the two saw to get pulse wave
        float raw_pulse = out1-out2; //raw pulse wave

        // High-Frequency Compensation Filter
        float filtered_pulse = (a0 * raw_pulse) + (a1 * filter_history);

        filter_history = raw_pulse; //store filter history

        // Write to buffer with normalization and gain
        ChannelInfoForCopy[sample] = filtered_pulse * norm * gain; //apply normalization and gain
    
    }

    for (int currentchannel = 1; currentchannel < totalNumOutputChannels; ++currentchannel)
    {
        //auto* channelData = buffer.getWritePointer (currentchannel);
        //juce::ignoreUnused (channelData);
        //copying the first channel to the rest of the channels
       buffer.copyFrom(currentchannel, 0, buffer, 0, 0, buffer.getNumSamples());
    
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
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"sineCurrentFrequency",1}, "sineCurrentFrequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 440.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"pw", 1}, "pw", juce::NormalisableRange<float>(0.1f, 0.9f, 0.01f), 0.5f));
   
    return {params .begin(), params.end()};
}