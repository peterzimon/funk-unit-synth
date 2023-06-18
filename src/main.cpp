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
 * - paraphonic logic works!! YAY! clean up debug/printf calls once all done
 * - test DCOs
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
#include "hardware/clocks.h"
#include "frequency.pio.h"

/*
 * Custom libraries
 */
#include <utils.h>

/*
 * Project headers
 */
#include "ui.h"
#include "synth.h"
#include "settings.h"

/**
 * Classes
*/
Settings settings;
Synth &synth = Synth::get_instance();
UI &ui = UI::get_instance();

PIO pio = pio0;
uint sm = pio_claim_unused_sm(pio, true);

int main() {
    stdio_init_all();

    printf("\n\n--- SHMØERGH FUNK LIVE ONE ---\r\n\n");

    // Init PIOs: they must be initialised here in main.cpp
    uint offset[2];
    offset[0] = pio_add_program(settings.pio[0], &frequency_program);
    offset[1] = pio_add_program(settings.pio[1], &frequency_program);

    for (int i = 0; i < VOICES; i++) {
        init_sm_pin(settings.pio[settings.voice_to_pio[i]],
                    settings.voice_to_sm[i],
                    offset[settings.voice_to_pio[i]],
                    settings.reset_pins[i]);
        pio_sm_set_enabled(settings.pio[settings.voice_to_pio[i]], settings.voice_to_sm[i], true);
    }

    // Initialise UI
    ui.init();

    // Initialise synth
    synth.init(PARA);
    synth.init_dcos();

    // Test -------------------------------
    // synth.set_adsr(false, true, false);
    // synth.set_solo(false);
    // synth.set_detune(true);
    // synth.set_portamento(false);
    // synth.set_kb_tracking(true);
    // synth.set_velo_tracking(true);
    // ------------------------------------

    // Main update loop
    while (1) {
        ui.scan();
        synth.process();
    }

    return 0;
}
