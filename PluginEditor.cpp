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
    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "Gain", gainSlider);


    addAndMakeVisible(frequencySlider);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    frequencySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    frequencySlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "currentFrequency_in_midi", frequencySlider);

    
    addAndMakeVisible(pulseWidthSlider);
    pulseWidthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    pulseWidthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pulseWidthSlider.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    pulseWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.apvts, "pw", pulseWidthSlider);

    addAndMakeVisible(pluckButton);
    pluckButton.onClick = [this]
    {
        processorRef.karp.pluck();
    };



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

    auto bounds = getLocalBounds();
     auto area = getLocalBounds();
  auto height = 40;
  
    
    gainSlider.setBounds(area.removeFromTop(height));
    frequencySlider.setBounds(area.removeFromTop(height));
    pulseWidthSlider.setBounds(area.removeFromTop(height));

    pluckButton.setBounds(20, 20, 100, 30);
}

