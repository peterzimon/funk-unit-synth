#include <stdio.h>
#include <math.h>
#include "midi_handler.h"

/**
 * Initialises buffer which holds incoming MIDI messages. Set mode according to 
 * settings.
*/
void MidiHandler::init(void) {
    
    // 
    uart_init(MIDI_UART_INSTANCE, MIDI_BAUDRATE);
    gpio_set_function(GP_MIDI_RX, GPIO_FUNC_UART);

    // Software init
    m_input_buffer.init(m_buffer_var, MIDI_BUFFER_SIZE);

    int hardware_gate_gp;
    for (int i = 0; i < settings.voices; i++) {
        m_cvs[i] = 0;
        m_gates[i] = 0;

        // Gates
        hardware_gate_gp = settings.gate_gps[i];
        if (hardware_gate_gp != -1) {
            gpio_init(settings.gate_gps[i]);
            gpio_set_dir(settings.gate_gps[i], GPIO_OUT);
            gpio_pull_down(settings.gate_gps[i]);
            gpio_put(settings.gate_gps[i], 0);
        }
    }
    set_mode();
}

/**
 * Each mode has its own converter that implements the same interface. Here we
 * set up a pointer to the converter of the selected mode, then reset all voices
 * and gates.
*/
void MidiHandler::set_mode(void) {
    switch (settings.mode) {
        case PARA:
            m_converter = &m_para;
            break;
    }

    m_converter->reset();

    for (int i = 0; i < settings.voices; i++) {
        m_cvs[i] = m_gates[i] = 0;
    }

    m_update_output();
}

/**
 * The process function that's called in the main loop
*/
void MidiHandler::process() {
    m_read_midi();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE ON event 
 * was fired.
*/
void MidiHandler::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    m_converter->note_on(channel, note, velocity);
    m_converter->get_cv_gate(m_cvs, m_gates);
    m_update_output();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE OFF 
 * event was fired.
*/
void MidiHandler::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    m_converter->note_off(channel, note, velocity);
    m_converter->get_cv_gate(m_cvs, m_gates);
    m_update_output();
}

/**
 * Callback function that the MidiParser (parent) class calls if a PITCH BEND
 * event was fired.
*/
void MidiHandler::pitch_bend(uint8_t channel, uint16_t bend) {
    m_converter->update_pitch_bend(bend);
    m_converter->get_cv_gate(m_cvs, m_gates);
    m_update_output();
}

/** ----------------------------------------------------------------------------
 * PRIVATE
*/

/**
 * Reads incoming MIDI messages via MidiParser parent class. This function is 
 * called for infinity from the main loop.
*/
void MidiHandler::m_read_midi() {
    if (!uart_is_readable(MIDI_UART_INSTANCE)) return;
    
    uint8_t data = uart_getc(MIDI_UART_INSTANCE);
    m_input_buffer.write_byte(data);

    while (!m_input_buffer.is_empty()) {
        uint8_t byte = 0;
        m_input_buffer.read_byte(byte);
        
        // Parent class call which will eventually call this class's midi 
        // message methods such as note_on, note_off etc.
        this->parse_byte(byte);
    }
}

bool MidiHandler::m_any_gate_on() {
    for (size_t i = 0; i < settings.voices; i++) {
        if (m_gates[i]) return true;
    }
    return false;
}

/**
 * Updates the control voltages, gates and the UI
*/
void MidiHandler::m_update_output(void) {
    int cv;
    MCP48X2 *dac;
    mcp48x2_channel channel;

    for (int i = 0; i < MAX_VOICES; i++) {
        cv = m_cvs[i];
        cv = cv < 0 ? 0 : cv;

        // printf("%d, ", cv);

        // // This could be nicer
        switch (i)
        {
        case 0:
            dac = m_dac_1;
            channel = MCP48X2_CHANNEL_A;
            break;
        case 1:
            dac = m_dac_1;
            channel = MCP48X2_CHANNEL_B;
            break;
        case 2:
            dac = m_dac_2;
            channel = MCP48X2_CHANNEL_A;
            break;
        case 3:
            dac = m_dac_2;
            channel = MCP48X2_CHANNEL_B;
            break;
        }

        if (cv != -1) {
            dac->config(channel, MCP48X2_GAIN_X2, 1);   // Set channel (set_channel function doesn't work)
            dac->write(cv);
        }

        switch (settings.mode)
        {
        case PARA:
            if (m_any_gate_on()) {
                gpio_put(settings.gate_gps[i], 1);
            } else {
                gpio_put(settings.gate_gps[i], 0);
            }
            break;
        case POLY:
            if (settings.gate_gps[i] != -1) {
                gpio_put(settings.gate_gps[i], m_gates[i]);
            }
            break;
        }

        // No need to update UI since it's just a LED which is driven by the 
        // gate output.
    }
    // printf("`\r\n");
}