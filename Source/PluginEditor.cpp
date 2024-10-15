/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BassQualizerAudioProcessorEditor::BassQualizerAudioProcessorEditor (BassQualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqSliderAttachment(audioProcessor.apvts, "peakFreq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "peakGainInDb", peakGainSlider),
    peakqualitySliderAttachment(audioProcessor.apvts, "peakQuality", peakqualitySlider),
    lowcutFreqSliderAttachment(audioProcessor.apvts, "lowCutFreq", lowcutFreqSlider),
    highcutFreqSliderAttachment(audioProcessor.apvts, "highCutFreq", highcutFreqSlider),
    lowcutSlopeSliderAttachment(audioProcessor.apvts, "lowCutSlope", lowcutSlopeSlider),
    highcutSlopeSliderAttachment(audioProcessor.apvts, "highCutSlope", highcutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for( auto* comp : getComps()){
        addAndMakeVisible(comp);
    }
    setSize (600, 400);
}

BassQualizerAudioProcessorEditor::~BassQualizerAudioProcessorEditor()
{
}

//==============================================================================
void BassQualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void BassQualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowcutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowcutSlopeSlider.setBounds(lowCutArea);

    highcutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highcutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakqualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> BassQualizerAudioProcessorEditor::getComps()
{
    return {&peakFreqSlider, &peakGainSlider, &peakqualitySlider, &lowcutFreqSlider, &highcutFreqSlider, &lowcutSlopeSlider, &highcutSlopeSlider};
}
