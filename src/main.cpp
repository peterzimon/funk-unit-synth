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
 *      MidiHandler: MidiParser 
 *          - parses incoming MIDI data
 *          - selects cv calculator class based on selected mode
 *          - passes on MIDI message to cv calculator
 *          - receives calculated CV and gate values
 *          - updates DAC to new CV values
 *          - updates gates
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
 * - make sure MIDI handling works as intended
 * - add basic DCO handling for a single voice
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

/* 
 * Custom libraries
 */
#include <utils.h>
#include <mcp48x2.h>

/*
 * Project headers
 */
#include "settings.h"
#include "ui.h"
#include "synth.h"

Settings settings;
// MCP48X2 dac_1;
// MCP48X2 dac_2;

UI &ui = UI::get_instance();
Synth &synth = Synth::get_instance();

int main() {
    stdio_init_all();

    settings.mode = PARA;

    sleep_ms(1000);
    ui.init();
    // dac_1.init(DAC_SPI_PORT, GP_DAC_1_CS, GP_DAC_SCK, GP_DAC_MOSI); // DAC 1 used for ADSR
    // dac_2.init(DAC_SPI_PORT, GP_DAC_2_CS, GP_DAC_SCK, GP_DAC_MOSI); // Voices 3 & 4
    
    // midi_handler.attach(&dac_1);
    // midi_handler.attach(&dac_2);

    synth.init();
    
    while (1) {
        ui.update();
        synth.process();
    }

    return 0;
}
