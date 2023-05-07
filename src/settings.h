#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <inttypes.h>
#include <pico/stdio.h>
#include "hardware/spi.h"
#include "hardware/uart.h"

// GLOBAL
#define MAX_VOICES          4

// DAC
#define DAC_SPI_PORT        spi0
#define GP_DAC_SCK          6
#define GP_DAC_MOSI         7
#define GP_DAC_1_CS         5
#define GP_DAC_2_CS         10

// NUMBER OF VOICES
#define GP_VOICES_SWITCH    15

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
    uint8_t midi_channel = 0;
    uint8_t voices = 3;

    int gate_gps[MAX_VOICES] = {
        21, -1, -1, -1
    };
};

extern Settings settings;

#endif