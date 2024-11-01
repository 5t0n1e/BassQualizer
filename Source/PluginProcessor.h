/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

template<typename T>
struct Fifo {
    void prepare(int numChannels, int numSamples) {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float> >,
                      "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>")
                ;
        for (auto &buffer: buffers) {
            buffer.setSize(numChannels,
                           numSamples,
                           false, //clear everything?
                           true, //including the extra space?
                           true); //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements) {
        static_assert(std::is_same_v<T, std::vector<float> >,
                      "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto &buffer: buffers) {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T &t) {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0) {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T &t) {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0) {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const {
        return fifo.getNumReady();
    }

private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{Capacity};
};


enum Channel {
    Right, Left
};

template<typename BlockType>
struct SingleChannelSampleFifo {
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch) {
        prepared.set(false);
    }

    void update(const BlockType &buffer) {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto *channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize) {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1, //channel
                             bufferSize, //num samples
                             false, //keepExistingContent
                             true, //clear extra space
                             true); //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }

    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType &buf) { return audioBufferFifo.pull(buf); }

private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample) {
        if (fifoIndex == bufferToFill.getNumSamples()) {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings {
    float peakFreq{0},
            peakGainInDecibels{0},
            peakQuality{1.f},
            lowCutFreq{0}, highCutFreq{0},
            reverbRoomSize{0.5f},
            reverbDamping{0.5f},
            reverbWetLevel{0.33f},
            reverbDryLevel{0.4f},
            reverbWidth{1.0f};

    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};

    bool lowCutBypassed{false},
            peakBypassed{false},
            highCutBypassed{false},
            reverbBypassed{false},
            reverbFreezeMode{false};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;


enum ChainPositions {
    lowCut,
    peak,
    highCut,
    reverb
};


using Coefficients = Filter::CoefficientsPtr;

void updateCoefficients(Coefficients &old, const Coefficients &replacements);

Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate);

template<typename ChainType, typename CoefficientType>

void updateCutFilter(ChainType &chain, const CoefficientType &cutCoefficients, Slope cutSlope) {
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch (cutSlope) {
        case Slope_12:
            *chain.template get<0>().coefficients = *cutCoefficients[0];
            chain.template setBypassed<0>(false);
            break;
        case Slope_24:
            *chain.template get<0>().coefficients = *cutCoefficients[0];
            chain.template setBypassed<0>(false);
            *chain.template get<1>().coefficients = *cutCoefficients[1];
            chain.template setBypassed<1>(false);
            break;
        case Slope_36:
            *chain.template get<0>().coefficients = *cutCoefficients[0];
            chain.template setBypassed<0>(false);
            *chain.template get<1>().coefficients = *cutCoefficients[1];
            chain.template setBypassed<1>(false);
            *chain.template get<2>().coefficients = *cutCoefficients[2];
            chain.template setBypassed<2>(false);
            break;
        case Slope_48:
            *chain.template get<0>().coefficients = *cutCoefficients[0];
            chain.template setBypassed<0>(false);
            *chain.template get<1>().coefficients = *cutCoefficients[1];
            chain.template setBypassed<1>(false);
            *chain.template get<2>().coefficients = *cutCoefficients[2];
            chain.template setBypassed<2>(false);
            *chain.template get<3>().coefficients = *cutCoefficients[3];
            chain.template setBypassed<3>(false);
            break;
        default: break;
    }
}

inline auto makeLowCutFilter(const ChainSettings &chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
        sampleRate,
        2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings &chainSettings, double sampleRate) {
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
        sampleRate,
        2 * (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class BassQualizerAudioProcessor : public juce::AudioProcessor {
public:
    //==============================================================================
    BassQualizerAudioProcessor();

    ~BassQualizerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;

    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;

    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;

    void setStateInformation(const void *data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::dsp::Reverb reverb;

    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameters()};

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{Channel::Left};
    SingleChannelSampleFifo<BlockType> rightChannelFifo{Channel::Right};

private:
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings &chainSettings);

    void updateFilters();

    void updateLowCutFilter(const ChainSettings &chainSettings);

    void updateHighCutFilter(const ChainSettings &chainSettings);

    void updateReverbFilter(const ChainSettings &chainSettings);

    juce::dsp::Oscillator<float> osc;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassQualizerAudioProcessor)
};
