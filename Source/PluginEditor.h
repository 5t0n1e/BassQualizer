/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


struct CustomRotarySlider : juce::Slider
{
  CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                      juce::Slider::TextEntryBoxPosition::NoTextBox)
  {
    
  }
};

//==============================================================================
/**
*/
class BassQualizerAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener, 
juce::Timer
{
public:
    BassQualizerAudioProcessorEditor (BassQualizerAudioProcessor&);
    ~BassQualizerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BassQualizerAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{false};

    CustomRotarySlider peakFreqSlider,
    peakGainSlider,
    peakqualitySlider,
    lowcutFreqSlider,
    highcutFreqSlider,
    lowcutSlopeSlider,
    highcutSlopeSlider;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakqualitySliderAttachment,
    lowcutFreqSliderAttachment,
    highcutFreqSliderAttachment,
    lowcutSlopeSliderAttachment,
    highcutSlopeSliderAttachment;

  

    std::vector<juce::Component*> getComps();

    MonoChain monoChain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassQualizerAudioProcessorEditor)
};
