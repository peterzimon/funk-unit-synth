/*
 * shmoergh-funk-live for Raspberry Pi Pico
 *
 * @version     1.0.0
 * @author      Peter Zimon — peter.zimon@gmail.com
 * @copyright   2023
 * @licence     MIT
 * 
 *          ---------------------------------------------------
 *          |       USE ONLY USB POWER WHEN FLASHING!!!!      |
 *          ---------------------------------------------------
 * 
 * Classes
 * -------
 *      Synth: MidiParser 
 *          - parses incoming MIDI data
 *          - updates DCO frequencies and amp levels
 * 
 *      IConverter
 *          - interface for different converters (modes). Converter
 *            implementations are in ./converters
 * 
 *      UI
 *          - handles the interface, LEDs and stuff
 *
 * @TODO:
 * 
 * ATM theoretically the synth reads MIDI and sets the frequency of 6 (PIO) pins
 * and the voltage of 6 (PWM) pins according to the incoming notes. The played 
 * notes are distributed equally amongst unused voices. Next step: test this!
 */

/*
 * C++ headers
 */
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cstring>

/*
 * Pico headers
 */
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "frequency.pio.h"

/* 
 * Custom libraries
 */
#include <utils.h>

/*
 * Project headers
 */
#include "settings.h"
#include "ui.h"
#include "synth.h"

/**
 * Classes
*/
Settings settings;
UI &ui = UI::get_instance();
Synth &synth = Synth::get_instance();

int main() {
    stdio_init_all();

    settings.mode = PARA;

    sleep_ms(1000);
    ui.init();

    synth.init();

    // Init PIOs: they must be initialised here in main.cpp
    uint offset[2];
    offset[0] = pio_add_program(settings.pio[0], &frequency_program);
    offset[1] = pio_add_program(settings.pio[1], &frequency_program);
    for (int i = 0; i < settings.voices; i++) {
        init_sm_pin(settings.pio[settings.voice_to_pio[i]], 
                    settings.voice_to_sm[i], 
                    offset[settings.voice_to_pio[i]], 
                    settings.reset_pins[i]);
        pio_sm_set_enabled(settings.pio[settings.voice_to_pio[i]], settings.voice_to_sm[i], true);
    }
    
    while (1) {
        ui.update();
        synth.process();
    }

    return 0;
}
