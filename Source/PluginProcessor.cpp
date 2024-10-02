/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BassQualizerAudioProcessor::BassQualizerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

BassQualizerAudioProcessor::~BassQualizerAudioProcessor() {
}

//==============================================================================
const juce::String BassQualizerAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool BassQualizerAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool BassQualizerAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool BassQualizerAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double BassQualizerAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int BassQualizerAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int BassQualizerAudioProcessor::getCurrentProgram() {
    return 0;
}

void BassQualizerAudioProcessor::setCurrentProgram(int index) {
}

const juce::String BassQualizerAudioProcessor::getProgramName(int index) {
    return {};
}

void BassQualizerAudioProcessor::changeProgramName(int index, const juce::String &newName) {
}

//==============================================================================
void BassQualizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::peak>().coefficients = *peakCoefficients;
}

void BassQualizerAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BassQualizerAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
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
#endif

void BassQualizerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    *leftChain.get<ChainPositions::peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::peak>().coefficients = *peakCoefficients;

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool BassQualizerAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *BassQualizerAudioProcessor::createEditor() {
    return new juce::GenericAudioProcessorEditor(*this);
    //return new BassQualizerAudioProcessorEditor(*this);
}

//==============================================================================
void BassQualizerAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BassQualizerAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("lowCutFreq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("highCutFreq")->load();
    settings.peakFreq = apvts.getRawParameterValue("peakFreq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("peakGainInDb")->load();
    settings.peakQuality = apvts.getRawParameterValue("peakQuality")->load();
    settings.lowCutSlope = apvts.getRawParameterValue("lowCutSlope")->load();
    settings.highCutSlope = apvts.getRawParameterValue("highCutSlope")->load();

    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout BassQualizerAudioProcessor::createParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("lowCutFreq", "Low Cut Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
                                                           20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("highCutFreq", "High Cut Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
                                                           20000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("peakFreq", "Peak Freq",
                                                           juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
                                                           750.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("peakGainInDb", "Peak Gain", -24.0f, 24.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("peakQuality", "Peak Quality", 0.1f, 10.0f, 1.0f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("lowCutSlope", "Low Cut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("highCutSlope", "High Cut Slope", stringArray, 0));


    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter() {
    return new BassQualizerAudioProcessor();
}
