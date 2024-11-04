/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(BassQualizerAudioProcessor& p) : audioProcessor(p), leftChannelFifo(&p.leftChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());

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

    juce::AudioBuffer<float> tempIncomingBuffer;

    while( leftChannelFifo->getNumCompleteBuffersAvailable() > 0 )
    {
        if( leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    const auto fftBounds = getAnalysisArea().toFloat();
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();

    const auto binWidth = audioProcessor.getSampleRate() / (double)fftSize;

    while( leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        if( leftChannelFFTDataGenerator.getFFTData(fftData) )
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }



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
        //repaint();

    }

    repaint();
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    //auto responseArea = getLocalBounds();
    auto responseArea = getAnalysisArea();

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
        if (!monoChain.isBypassed<ChainPositions::highCut>())
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


    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.setColour(Colours::blue);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

g.drawRect(getRenderArea());

}


juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    bounds.removeFromBottom(2);
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
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
    highcutBypassButtonAttachment(audioProcessor.apvts, "highCutBypass", highcutBypassButton),
    reverbRoomSizeAttachment(audioProcessor.apvts, "reverbRoomSize", reverbRoomSizeSlider),
    reverbDampingAttachment(audioProcessor.apvts, "reverbDamping", reverbDampingSlider),
    reverbWidthAttachment(audioProcessor.apvts, "reverbWidth", reverbWidthSlider),
    reverbDryLevelAttachment(audioProcessor.apvts, "reverbDryLevel", reverbDryLevelSlider),
    reverbWetLevelAttachment(audioProcessor.apvts, "reverbWetLevel", reverbWetLevelSlider),
    reverbBypassButtonAttachment(audioProcessor.apvts, "reverbBypass", reverbBypassButton)

{
    peakFreqSlider.setLookAndFeel(&lookAndFeelV1);
    peakGainSlider.setLookAndFeel(&lookAndFeelV3);
    peakqualitySlider.setLookAndFeel(&lookAndFeelV3);
    lowcutFreqSlider.setLookAndFeel(&lookAndFeelV1);
    highcutFreqSlider.setLookAndFeel(&lookAndFeelV1);
    lowcutSlopeSlider.setLookAndFeel(&lookAndFeelV3);
    highcutSlopeSlider.setLookAndFeel(&lookAndFeelV3);
    reverbRoomSizeSlider.setLookAndFeel(&lookAndFeelV1);
    reverbDampingSlider.setLookAndFeel(&lookAndFeelV1);
    reverbWetLevelSlider.setLookAndFeel(&lookAndFeelV1);
    reverbDryLevelSlider.setLookAndFeel(&lookAndFeelV1);
    reverbWidthSlider.setLookAndFeel(&lookAndFeelV1);

    lowcutBypassButton.setLookAndFeel(&lookAndFeelV1);
    peakBypassButton.setLookAndFeel(&lookAndFeelV1);
    highcutBypassButton.setLookAndFeel(&lookAndFeelV1);
    reverbBypassButton.setLookAndFeel(&lookAndFeelV1);

    // Initialize labels
    lowcutLabel.setText("Low Cut", juce::dontSendNotification);
    highcutLabel.setText("High Cut", juce::dontSendNotification);
    peakLabel.setText("Peak", juce::dontSendNotification);
    reverbLabel.setText("Reverb", juce::dontSendNotification);

    // Add labels to the editor
    addAndMakeVisible(lowcutLabel);
    addAndMakeVisible(highcutLabel);
    addAndMakeVisible(peakLabel);
    addAndMakeVisible(reverbLabel);

    for( auto* comp : getComps()){
        addAndMakeVisible(comp);
    }

    setSize (1200, 800);
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
    reverbRoomSizeSlider.setLookAndFeel(nullptr);
    reverbDampingSlider.setLookAndFeel(nullptr);
    reverbWetLevelSlider.setLookAndFeel(nullptr);
    reverbDryLevelSlider.setLookAndFeel(nullptr);
    reverbWidthSlider.setLookAndFeel(nullptr);

    lowcutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    reverbBypassButton.setLookAndFeel(nullptr);
}

//==============================================================================
void BassQualizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    g.fillAll (juce::Colours::black);
}

void BassQualizerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    responseCurveComponent.setBounds(responseArea);

    auto topArea = bounds.removeFromTop(bounds.getHeight() * 0.5);

    auto lowCutArea = topArea.removeFromLeft(topArea.getWidth() * 0.33);
    auto highCutArea = topArea.removeFromRight(topArea.getWidth() * 0.5);
    auto peakArea = topArea;

    lowcutLabel.setBounds(lowCutArea.removeFromTop(25));
    lowcutLabel.setJustificationType(juce::Justification::centred);
    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowcutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowcutSlopeSlider.setBounds(lowCutArea);

    highcutLabel.setBounds(highCutArea.removeFromTop(25));
    highcutLabel.setJustificationType(juce::Justification::centred);
    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highcutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highcutSlopeSlider.setBounds(highCutArea);

    peakLabel.setBounds(peakArea.removeFromTop(25));
    peakLabel.setJustificationType(juce::Justification::centred);
    peakBypassButton.setBounds(peakArea.removeFromTop(25));
    peakFreqSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * 0.33));
    peakGainSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * 0.5));
    peakqualitySlider.setBounds(peakArea);

    auto bottomArea = bounds;
    bottomArea.removeFromTop(50);

    reverbLabel.setBounds(bottomArea.removeFromTop(25));
    reverbLabel.setJustificationType(juce::Justification::centred);
    reverbBypassButton.setBounds(bottomArea.removeFromTop(25));

    auto borderSpacing = 100;
    bottomArea.reduce(borderSpacing, 0);

    // Calculate the total width of all sliders and the spacing
    int numSliders = 5;
    int totalSpacing = borderSpacing * 2;
    int totalWidth = bottomArea.getWidth() - totalSpacing;
    auto reverbSliderWidth = totalWidth / numSliders;
    auto reverbSliderHeight = 120;

    // Calculate the starting X position to center the sliders
    int startX = (bottomArea.getWidth() - totalWidth) / 2;

    // Adjust the bottomArea to start from the calculated X position
    bottomArea = bottomArea.withTrimmedLeft(startX).withTrimmedRight(startX);

    reverbRoomSizeSlider.setBounds(bottomArea.removeFromLeft(reverbSliderWidth).removeFromTop(reverbSliderHeight));
    reverbDampingSlider.setBounds(bottomArea.removeFromLeft(reverbSliderWidth).removeFromTop(reverbSliderHeight));
    reverbWetLevelSlider.setBounds(bottomArea.removeFromLeft(reverbSliderWidth).removeFromTop(reverbSliderHeight));
    reverbDryLevelSlider.setBounds(bottomArea.removeFromLeft(reverbSliderWidth).removeFromTop(reverbSliderHeight));
    reverbWidthSlider.setBounds(bottomArea.removeFromLeft(reverbSliderWidth).removeFromTop(reverbSliderHeight));
    reverbFreezeModeButton.setBounds(bottomArea);
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
        &reverbRoomSizeSlider,
        &reverbDampingSlider,
        &reverbWetLevelSlider,
        &reverbDryLevelSlider,
        &reverbWidthSlider,

        &responseCurveComponent,

        &lowcutBypassButton,
        &peakBypassButton,
        &highcutBypassButton,
        &reverbBypassButton
    };
}

