/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(BassQualizerAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}


void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if(parametersChanged.compareAndSetBool(false, true))
    {
        DBG("Params changed");
        // update the mono chain
        auto chainSettings = getChainSettings(audioProcessor.apvts);

        monoChain.setBypassed<ChainPositions::lowCut>(chainSettings.lowCutBypassed);
        monoChain.setBypassed<ChainPositions::peak>(chainSettings.peakBypassed);
        monoChain.setBypassed<ChainPositions::highCut>(chainSettings.highCutBypassed);

        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::peak>().coefficients, peakCoefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

        updateCutFilter(monoChain.get<ChainPositions::lowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::highCut>(), highCutCoefficients, chainSettings.highCutSlope);
        // signal a repaint
        repaint();

    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto responseArea = getLocalBounds();

    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::lowCut>();
    auto& peak = monoChain.get<ChainPositions::peak>();
    auto& highcut = monoChain.get<ChainPositions::highCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for( int i = 0; i < w; ++i){
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if(! monoChain.isBypassed<ChainPositions::peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(! monoChain.isBypassed<ChainPositions::lowCut>()) {
            if(!lowcut.isBypassed<0>())
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!lowcut.isBypassed<1>())
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!lowcut.isBypassed<2>())
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!lowcut.isBypassed<3>())
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        }
        if (!monoChain.isBypassed<ChainPositions::lowCut>())
        {
            if(!highcut.isBypassed<0>())
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!highcut.isBypassed<1>())
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!highcut.isBypassed<2>())
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

            if(!highcut.isBypassed<3>())
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i){
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));


}


//==============================================================================
BassQualizerAudioProcessorEditor::BassQualizerAudioProcessorEditor (BassQualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    responseCurveComponent(audioProcessor),
    peakFreqSliderAttachment(audioProcessor.apvts, "peakFreq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "peakGainInDb", peakGainSlider),
    peakqualitySliderAttachment(audioProcessor.apvts, "peakQuality", peakqualitySlider),
    lowcutFreqSliderAttachment(audioProcessor.apvts, "lowCutFreq", lowcutFreqSlider),
    highcutFreqSliderAttachment(audioProcessor.apvts, "highCutFreq", highcutFreqSlider),
    lowcutSlopeSliderAttachment(audioProcessor.apvts, "lowCutSlope", lowcutSlopeSlider),
    highcutSlopeSliderAttachment(audioProcessor.apvts, "highCutSlope", highcutSlopeSlider),
    lowcutBypassButtonAttachment(audioProcessor.apvts, "lowCutBypass", lowcutBypassButton),
    peakBypassButtonAttachment(audioProcessor.apvts, "peakBypass", peakBypassButton),
    highcutBypassButtonAttachment(audioProcessor.apvts, "highCutBypass", highcutBypassButton)

{
    peakFreqSlider.setLookAndFeel(&lookAndFeelV1);
    peakGainSlider.setLookAndFeel(&lookAndFeelV3);
    peakqualitySlider.setLookAndFeel(&lookAndFeelV3);
    lowcutFreqSlider.setLookAndFeel(&lookAndFeelV1);
    highcutFreqSlider.setLookAndFeel(&lookAndFeelV1);
    lowcutSlopeSlider.setLookAndFeel(&lookAndFeelV3);
    highcutSlopeSlider.setLookAndFeel(&lookAndFeelV3);

    for( auto* comp : getComps()){
        addAndMakeVisible(comp);
    }

    setSize (600, 400);
}

BassQualizerAudioProcessorEditor::~BassQualizerAudioProcessorEditor()
{
    peakFreqSlider.setLookAndFeel(nullptr);
    peakGainSlider.setLookAndFeel(nullptr);
    peakqualitySlider.setLookAndFeel(nullptr);
    lowcutFreqSlider.setLookAndFeel(nullptr);
    highcutFreqSlider.setLookAndFeel(nullptr);
    lowcutSlopeSlider.setLookAndFeel(nullptr);
    highcutSlopeSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void BassQualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    g.fillAll (juce::Colours::black);
}

void BassQualizerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    responseCurveComponent.setBounds(responseArea);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowcutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowcutSlopeSlider.setBounds(lowCutArea);

    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highcutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highcutSlopeSlider.setBounds(highCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(25));
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakqualitySlider.setBounds(bounds);
}


std::vector<juce::Component*> BassQualizerAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakqualitySlider,
        &lowcutFreqSlider,
        &highcutFreqSlider,
        &lowcutSlopeSlider,
        &highcutSlopeSlider,
        &responseCurveComponent,
        &lowcutBypassButton,
        &peakBypassButton,
        &highcutBypassButton
    };
}

