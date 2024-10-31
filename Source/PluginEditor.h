/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "myLookAndFeel.h"

struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                        juce::Slider::TextEntryBoxPosition::NoTextBox) {
    }
};

struct ResponseCurveComponent : juce::Component,
                                juce::AudioProcessorParameter::Listener,
                                juce::Timer {
    ResponseCurveComponent(BassQualizerAudioProcessor &);

    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {
    }

    void timerCallback() override;

    void paint(juce::Graphics &g) override;

private:
    BassQualizerAudioProcessor &audioProcessor;
    juce::Atomic<bool> parametersChanged{false};

    MonoChain monoChain;

    juce::Image background;

    juce::Rectangle<int> getRenderArea();

    juce::Rectangle<int> getAnalysisArea();
};

//==============================================================================
/**
*/
class BassQualizerAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    BassQualizerAudioProcessorEditor(BassQualizerAudioProcessor &);

    ~BassQualizerAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &g) override;

    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BassQualizerAudioProcessor &audioProcessor;

    CustomRotarySlider peakFreqSlider,
            peakGainSlider,
            peakqualitySlider,
            lowcutFreqSlider,
            highcutFreqSlider,
            lowcutSlopeSlider,
            highcutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
            peakGainSliderAttachment,
            peakqualitySliderAttachment,
            lowcutFreqSliderAttachment,
            highcutFreqSliderAttachment,
            lowcutSlopeSliderAttachment,
            highcutSlopeSliderAttachment;

    myLookAndFeelV1 lookAndFeelV1; // Create instances of your custom look and feel classes
    myLookAndFeelV3 lookAndFeelV3;

    std::vector<juce::Component *> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassQualizerAudioProcessorEditor)
};
