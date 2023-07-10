# Shmøergh Funk Live One — 6 voice paraphonic synthesizer

Shmøergh Funk Live One is a 6 voice (mostly) digitally controlled paraphonic
analog synthesizer. Its purposefully built with limitations so it's great for
live performances with quick changes in settings without presets or menu diving. It
has a lot of simple but effective ways to modulate the filter cutoff which makes
it great for funk performances. Because of its simplicity it's also cool to get
started with synthesis.

**Status:** WORK IN PROGRESS

**Latest version:** 1.1

## Features

- 6 voices (DCOs) controlled by a Raspberry Pi Pico
- Sawtooth, square, narrow pulse and fixed rate pulse with modulated square waveforms
- Analog signal path
- External filter connection with selectable various modulation sources (envelope, velocity, analog LFO, modwheel, keyboard tracking). Steiner-Parker
filter schematics and PCB is included
- 12bit digital ADSR with simplified envelope controls
- Built-in analog VCA with optional external VCA connections
- Monophonic, 3-voice unison with detune and 6-voice paraphonic modes
- MIDI input
- Eurorack compatible PCB sizing

Note: schematics does not include power supply

## References

- [Polykit DCO design](https://github.com/polykit/pico-dco)
- [The design of the Roland Juno synthesizes](https://blog.thea.codes/the-design-of-the-juno-dco/)
- [Steiner-Parker Filter](https://yusynth.net/Modular/EN/STEINERVCF/index-v2.html)

## License & info

Version: 1.1.0
Author: Peter Zimon
Copyright: 2023
Licence: MIT

