#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    myPhasor = Phasor(440.0f, static_cast<float>(sampleRate));
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



    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


        //list of variables to get from apvts
        float gain = apvts.getRawParameterValue("Gain")->load();
        float sineCurrentFrequency = apvts.getRawParameterValue("sineCurrentFrequency")->load();
        float pw= apvts.getRawParameterValue("pw")->load();; // pulse width of the pulse, 0..1
        float sampleRate = static_cast<float>(getSampleRate());

        // variables and constants
        float const a0 = 2.5f; // precalculated coeffs
        float const a1 = -1.5f; // for HF compensation
        float w = sineCurrentFrequency / sampleRate;
        
        myPhasor.frequency(sineCurrentFrequency, sampleRate);
        
        float n = (0.5f - w);
        float scaling = 13.0f *n *n*n*n; //calculate scating
        //float DC = 0.376f - w*0.752f; // DC compensation
        float norm = 1.0f - 2.0f*w; // calculate normalization
       

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    //Phasor phasor(0, currentSampleRate);

    juce::HeapBlock<float> b(buffer.getNumSamples()); // allocate array
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

        float phase = myPhasor.process();//get phasor value
        //Ocilator 1 (reference)
        float feedback1= phase+(osc1_history*scaling); //feedback from last output

        float saw1 = feedback1 - std::floor((feedback1));//wrap around using floor

        float osc1 = sin7(saw1); //calculate sinewave 

        float out1 = (osc1 + osc1_history) * 0.5f; //final output with naive integrator
        osc1_history = out1; //store history

        //Ocilator 2 (phase offset)
        float phase2_raw = phase + pw; //add pulse width offset
        if(phase2_raw >= 1.0f) phase2_raw -= 1.0f; //wrap



        float feedback2= phase2_raw+(osc2_history*scaling); //feedback from last output
        float saw2 = feedback2 - std::floor((feedback2));//wrap around using floor
        float osc2 = sin7(saw2); //calculate sinewave
        float out2 = (osc2 + osc2_history) * 0.5f; //final output with naive integrator
        osc2_history = out2; //store history

        // pulse wave out 
        float raw_pulse = out1-out2; //raw pulse wave

        // High-Frequency Compensation Filter
        float filtered_pulse = (a0 * raw_pulse) + (a1 * filter_history);

        filter_history = raw_pulse; //store filter history

      b[sample] = filtered_pulse * norm;
        //b[sample] = std::sin(myPhasor() * 2.0f * juce::MathConstants<float>::pi) * 0.1f; //for testing to see if audio works
        
    }

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] = b[sample] * gain;
        }
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


     params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {"pw",1}, "pw", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
   
    return {params .begin(), params.end()};
}