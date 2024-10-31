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

    reverb.prepare(spec);
    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();
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

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    reverb.process(leftContext);
    reverb.process(rightContext);
}

//==============================================================================
bool BassQualizerAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *BassQualizerAudioProcessor::createEditor() {
   // return new juce::GenericAudioProcessorEditor(*this);
    return new BassQualizerAudioProcessorEditor(*this);
}

//==============================================================================
void BassQualizerAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void BassQualizerAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateFilters();
    }
    
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("lowCutFreq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("highCutFreq")->load();
    settings.peakFreq = apvts.getRawParameterValue("peakFreq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("peakGainInDb")->load();
    settings.peakQuality = apvts.getRawParameterValue("peakQuality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("lowCutSlope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("highCutSlope")->load());
    settings.lowCutBypassed = apvts.getRawParameterValue("lowCutBypass")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("peakBypass")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("highCutBypass")->load() > 0.5f;
    settings.reverbRoomSize = apvts.getRawParameterValue("reverbRoomSize")->load();
    settings.reverbDamping = apvts.getRawParameterValue("reverbDamping")->load();
    settings.reverbWetLevel = apvts.getRawParameterValue("reverbWetLevel")->load();
    settings.reverbDryLevel = apvts.getRawParameterValue("reverbDryLevel")->load();
    settings.reverbWidth = apvts.getRawParameterValue("reverbWidth")->load();
    settings.reverbBypassed = apvts.getRawParameterValue("reverbBypass")->load() > 0.5f;
    settings.reverbFreezeMode = apvts.getRawParameterValue("reverbFreezeMode")->load() > 0.5f;

    return settings;
}

Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate) 
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void BassQualizerAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings) {
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::peak>(chainSettings.peakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::peak>().coefficients, *peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::peak>().coefficients, *peakCoefficients);
}

void updateCoefficients(Coefficients &old, const Coefficients &replacements) {
    *old = *replacements;
}

void BassQualizerAudioProcessor::updateLowCutFilter(const ChainSettings &chainSettings) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto &leftLowCut = leftChain.get<ChainPositions::lowCut>();
    auto &rightLowCut = rightChain.get<ChainPositions::lowCut>();

    leftChain.setBypassed<ChainPositions::lowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::lowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void BassQualizerAudioProcessor::updateHighCutFilter(const ChainSettings &chainSettings) {
    auto highCutCoefficientsHigh = makeHighCutFilter(chainSettings, getSampleRate());
    auto &leftHighCut = leftChain.get<ChainPositions::highCut>();
    auto &rightHighCut = rightChain.get<ChainPositions::highCut>();

    leftChain.setBypassed<ChainPositions::highCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::highCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highCutCoefficientsHigh, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficientsHigh, chainSettings.highCutSlope);
}

void BassQualizerAudioProcessor::updateReverbFilter(const ChainSettings &chainSettings) {
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = chainSettings.reverbRoomSize;
    reverbParams.damping = chainSettings.reverbDamping;
    reverbParams.wetLevel = chainSettings.reverbWetLevel;
    reverbParams.dryLevel = chainSettings.reverbDryLevel;
    reverbParams.width = chainSettings.reverbWidth;
    reverbParams.freezeMode = chainSettings.reverbFreezeMode;

    reverb.setParameters(reverbParams);
    reverb.setEnabled(!chainSettings.reverbBypassed);
}


void BassQualizerAudioProcessor::updateFilters() {
    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);
    updateLowCutFilter(chainSettings);
    updateHighCutFilter(chainSettings);
    updateReverbFilter(chainSettings);
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

    layout.add(std::make_unique<juce::AudioParameterBool>("lowCutBypass", "Low Cut Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("peakBypass", "Peak Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("highCutBypass", "High Cut Bypass", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbRoomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbDamping", "Damping", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbWetLevel", "Wet Level", 0.0f, 1.0f, 0.33f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbDryLevel", "Dry Level", 0.0f, 1.0f, 0.4f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("reverbWidth", "Width", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("reverbFreezeMode", "Freeze Mode", false));

    layout.add(std::make_unique<juce::AudioParameterBool>("reverbBypass", "Reverb Bypass", true));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter() {
    return new BassQualizerAudioProcessor();
}
