#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    addAndMakeVisible(gainSlider);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "Gain", gainSlider);


    addAndMakeVisible(frequencySlider);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    frequencySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    frequencySlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "sineCurrentFrequency", frequencySlider);

    
    addAndMakeVisible(pulseWidthSlider);
    pulseWidthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    pulseWidthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pulseWidthSlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    pulseWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "pw", pulseWidthSlider);

}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::black);

    // g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);

}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    gainSlider.setBounds(50, 100, 100, 100);
    frequencySlider.setBounds(150, 100, 100, 100);
    pulseWidthSlider.setBounds(250, 100, 100, 100);
}

