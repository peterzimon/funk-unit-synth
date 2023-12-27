#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <inttypes.h>
#include <pico/stdio.h>
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/pio.h"

// GLOBAL
#define VOICES              6
#define FAT_MONO_VOICES     3
#define PARA_STACK_VOICES   false
#define DEFAULT_FREQ        220.0
#define MAX_FREQ            5000.0      // This depends on the integrator's RC constant.
                                        // Rint = 200kohm, Cint = 1nF
                                        // fmax = 1/RC = 5kHz
                                        // Change this value if you change any of
                                        // the above components in the circuit.
#define LOWEST_MIDI_NOTE    21          // A0. This is needed otherwise the DCO can
                                        // get in a stuck state when a too low note
                                        // is played.

#define PORTAMENTO_TIME     10
#define DETUNE_FACTOR       1.02f       // Only available in FAT mode, should be > 1

#define ENVELOPE_DAC_SIZE   4096
#define FILTER_MOD_DAC_SIZE 4096

#define VELO_FACTOR         24          // Velocity is used to mod the filter (if it's
                                        // turned on), with the formula:
                                        // Vmod = VELOCITY * VELO_FACTOR [mV]
                                        // (0 < VELOCITY < 127)
                                        // Since the max output is 4096mV and there
                                        // should be some space for other mod sources
                                        // it's maximised in about 3V (when velo = 127)

// ADSR (all time values are in us)
#define ATTACK_SHORT        3500
#define ATTACK_LONG         210000
#define DECAY_SHORT         500000
#define DECAY_MID           1500000
#define DECAY_LONG          23000000
#define DECAY_LONG_MIN      75          // * 100000 = min decay long
#define DECAY_LONG_MAX      230         // * 100000 = max decay long
#define SUSTAIN_ON          3600            // mV, between 0 and 4095
#define SUSTAIN_OFF         0               // mV, between 0 and 4095
#define RELEASE_SHORT       500000
#define RELEASE_LONG_MIN    50              // * 100000 = min release in us
#define RELEASE_LONG_MAX    300             // * 100000 = max release in us

// Keyboard tracking. There's a min and max frequency in between keyboard is
// tracked and changes the cutoff. Below it, tracking is off, above it it's
// fully open. The factor sets how much tracking should open cutoff. This is
// set so that at 1760Hz it sets the DAC to it's max value (4096/1760 = 2.32).
#define KB_TRACK_MIN_FREQ   55
#define KB_TRACK_MAX_FREQ   1760
#define KB_TRACK_FACTOR     2.3

// DAC
#define DAC_SPI_PORT        spi0
#define GP_DAC_SCK          6
#define GP_DAC_MOSI         7
#define GP_DAC_CS           5

// MIDI
#define MIDI_CHANNEL                4
#define OCTAVES                     10
#define MIDI_UART_INSTANCE          uart1
#define GP_MIDI_RX                  9
#define MIDI_BAUDRATE               31250
#define MIDI_OCTAVE_SHIFT           0 // Not implemented
#define PITCH_BEND_SEMITONES        1 // Min. 1

// UI
#define MUX_BINARY_PIN_A            4
#define MUX_BINARY_PIN_B            3
#define MUX_BINARY_PIN_C            2
#define MUX_BINARY_INPUT            8
#define ADC_RING_LEN_PIN            27
#define ADC_RING_LEN_CHANNEL        1
#define ADC_SYNTH_MODE_PIN          26
#define ADC_SYNTH_MODE_CHANNEL      0
#define ENABLE_CHORD_MEMORY         false
#define BTN_CHORD                   22
#define LED_CHORD                   28

// Switches in the order how they're connected to the MUX
#define NO_OF_SWITCHES              8
enum mux_switch {
    SOFT,
    HOLD,
    RING,
    PORTAMENTO,
    DETUNE,
    SOLO_CHORD,
    KB_TRACKING,
    WAH_VELOCITY
};

#define NO_OF_MODES 3
enum device_mode {
    MONO,
    FAT_MONO,
    PARA
};

// PWM division counter
const uint16_t DIV_COUNTER = 1250;

struct Settings
{
    device_mode mode;

    // Solo is a submode of paraphonic mode. If it's off then the all played
    // voices will be on until the first voice is pressed again (the first
    // voice controls the gate to the envelope). When solo is _on_ then non-
    // first voices will turn off when their respective key is released.
    bool solo = false;
    bool portamento = false;
    bool detune = false;
    bool kb_tracking = false;
    bool velo_tracking = false;

    const uint8_t midi_channel = 0;
    const uint8_t voices = 6;

    const uint8_t reset_pins[VOICES] = {10, 11, 12, 13, 14, 15};
    const uint8_t amp_pins[VOICES] = {21, 20, 19, 18, 17, 16};
    const uint8_t voice_to_pio[VOICES] = {0, 0, 0, 0, 1, 1};
    const uint8_t voice_to_sm[VOICES] = {0, 1, 2, 3, 0, 1};

    // Two PIOs with 6 state machines (4 and 2) are used to set the frequency of the DCOs
    const PIO pio[2] = {pio0, pio1};
};

extern Settings settings;

#endif