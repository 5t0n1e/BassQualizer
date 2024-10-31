# BassQualizer Docs

## Table of Contents

- [Introduction](#introduction)
- [Installation](#installation)
- [Usage](#usage)
- [Important Functions](#important-functions)

## Introduction

read README.md

## Installation

1. Make a plugin basic project with projucer with the same name as the plugin.
2. Go to the parent directory of the project. and clone this repository.
    - This will clone the files into the project directory.
3. Open the project in the projucer
    1. Add the myLookAndFeel.h and myLookAndFeel.cpp files to the project.
    2. Add the dsp module to the project.
4. Build the project in you're desired way (depending on operating system).
5. Open the plugin executable and route audio to it using you're desired way.
6. You can also use the plugin in a DAW with the vst.
7. Enjoy the plugin!.

## Usage

- The plugin has 4 filters:
    1. Lowcut
    2. Highcut
    3. Peaking
    4. Reverb
- Each filter has its own set of parameters that can be adjusted with the knobs.
- The filters can be turned on and off with the power buttons.

## Important Functions

- `void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override`
    - This function processes the audio signal.
    - It applies the filters to the audio signal.
    - The audio signal is passed in the buffer.
- `void prepareToPlay (double sampleRate, int samplesPerBlock) override`
    - This function prepares the plugin to play audio.
    - It sets the sample rate and the number of samples per block.
    - Every filter is prepared to play audio.
- `void updateFilters()`
    - This function updates the parameters of the filters.
    - It is called when the parameters are changed.
    - The parameters are updated with the values of the knobs.
- `ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts)`
    - This function gets the values of the knobs.
    - It returns the values in a struct.
    - The struct is used to update the parameters of the filters.
- `void updatePeakFilter(const ChainSettings& chainSettings)`
    - This function updates the parameters of the peaking filter.
- `void updateLowCutFilters(const ChainSettings& chainSettings)`
    - This function updates the parameters of the lowcut filter.
- `void updateHighCutFilters(const ChainSettings& chainSettings)`
    - This function updates the parameters of the highcut filter.
- `void updateReverbFilters(const ChainSettings& chainSettings)`
    - These functions update the parameters of the filters.
    - They are called when the parameters are changed.
    - The parameters are updated with the values of the knobs.
