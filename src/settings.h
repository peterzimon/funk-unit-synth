#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <inttypes.h>
#include <pico/stdio.h>
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/pio.h"

// GLOBAL
#define VOICES              6
#define PARA_STACK_VOICES   false
#define DEFAULT_FREQ        220.0
// #define AMP_COMP            0.75    // Use this to fine tune DCO amplification
#define MAX_FREQ            5000.0      // This depends on the integrator's RC constant.
                                        // Rint = 200kohm, Cint = 1nF
                                        // fmax = 1/RC = 5kHz
                                        // Change this value if you change any of
                                        // the above components in the circuit.


// DAC
#define DAC_SPI_PORT        spi0
#define GP_DAC_SCK          6
#define GP_DAC_MOSI         7
#define GP_DAC_1_CS         5
#define GP_DAC_2_CS         10

// MIDI
#define LOWEST_MIDI_NOTE    0x00
#define OCTAVES             10
#define MAX_PITCH_BEND      0x3fff
#define PITCH_BEND_CENTER   0x2000
#define MAX_PB_SEMINOTES    2

#define MIDI_OCTAVE_SHIFT   0 // Not implemented

#define MIDI_UART_INSTANCE  uart1
#define GP_MIDI_RX          9
#define MIDI_BAUDRATE       31250

// CV
#define MAX_NOTE_VOLTAGE    4095


enum device_mode {
    MONO,
    PARA
};

struct Settings
{
    device_mode mode;

    // Solo is a submode of paraphonic mode. If it's off then the all played
    // voices will be on until the first voice is pressed again (the first
    // voice controls the gate to the envelope). When solo is _on_ then non-
    // first voices will turn off when their respective key is released.
    bool solo = false;
    
    const uint8_t midi_channel = 0;
    const uint8_t voices = 6;

    const uint8_t reset_pins[VOICES] = {10, 11, 12, 13, 14, 15};
    const uint8_t amp_pins[VOICES] = {16, 17, 18, 19, 20, 21};
    const uint8_t voice_to_pio[VOICES] = {0, 0, 0, 0, 1, 1};
    const uint8_t voice_to_sm[VOICES] = {0, 1, 2, 3, 0, 1};

    // Two PIOs with 6 state machines (4 and 2) are used to set the frequency of the DCOs
    const PIO pio[2] = {pio0, pio1};
};

extern Settings settings;

#endif