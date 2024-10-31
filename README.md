# Equalizer - C++ - VST

This is a simple equalizer plugin for VST3. It has a couple of filters that can be used to manipulate the audio signal. We also made a visual representation of the filters in the frequency domain and the audio signal the plugin is processing. 

## Lowcut
The lowcut filter is a highpass filter that removes the low frequencies from the audio signal. The cutoff frequency can be adjusted with the knobs. The slope of the filter can also be adjusted with the knobs.

## Highcut
The highcut filter is a lowpass filter that removes the high frequencies from the audio signal. The cutoff frequency can be adjusted with the knobs. The slope of the filter can also be adjusted with the knobs.

## Peaking
The peaking filter is a bandpass filter that boosts or attenuates a certain frequency range. The center frequency, gain and bandwidth can be adjusted with the knobs.

## Reverb
The reverb filter is a simple reverb effect that can be added to the audio signal. The reverb time and dry/wet mix can be adjusted with the knobs.
