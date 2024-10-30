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

    juce::ToggleButton lowcutBypassButton, peakBypassButton, highcutBypassButton;

    using ButtonAttachment = APVTS::ButtonAttachment;

    ButtonAttachment lowcutBypassButtonAttachment, peakBypassButtonAttachment, highcutBypassButtonAttachment;

    // Reverb sliders
    juce::Slider reverbRoomSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbWetLevelSlider;
    juce::Slider reverbDryLevelSlider;
    juce::Slider reverbWidthSlider;
    juce::ToggleButton reverbFreezeModeButton;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbRoomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbDryLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbFreezeModeAttachment;

    // PluginEditor.h
    juce::ToggleButton reverbBypassButton;
    ButtonAttachment reverbBypassButtonAttachment;

    myLookAndFeelV1 lookAndFeelV1; // Create instances of your custom look and feel classes
    myLookAndFeelV3 lookAndFeelV3;
    

    std::vector<juce::Component *> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassQualizerAudioProcessorEditor)
};
