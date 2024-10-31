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
            highcutSlopeSlider,
            reverbRoomSizeSlider,
            reverbDampingSlider,
            reverbWetLevelSlider,
            reverbDryLevelSlider,
            reverbWidthSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
            peakGainSliderAttachment,
            peakqualitySliderAttachment,
            lowcutFreqSliderAttachment,
            highcutFreqSliderAttachment,
            lowcutSlopeSliderAttachment,
            highcutSlopeSliderAttachment,
            reverbRoomSizeAttachment,
            reverbDampingAttachment,
            reverbWetLevelAttachment,
            reverbDryLevelAttachment,
            reverbWidthAttachment;

    juce::ToggleButton lowcutBypassButton, peakBypassButton, highcutBypassButton, reverbBypassButton,
            reverbFreezeModeButton;

    using ButtonAttachment = APVTS::ButtonAttachment;

    ButtonAttachment lowcutBypassButtonAttachment,
            peakBypassButtonAttachment,
            highcutBypassButtonAttachment,
            reverbBypassButtonAttachment;;

    // Custom look and feel
    myLookAndFeelV1 lookAndFeelV1;
    myLookAndFeelV3 lookAndFeelV3;

    // Labels
    using Label = juce::Label;
    Label lowcutLabel,
            highcutLabel,
            peakLabel,
            reverbLabel;

    std::vector<juce::Component *> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassQualizerAudioProcessorEditor)
};
