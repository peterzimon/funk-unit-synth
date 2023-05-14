#include <stdio.h>
#include <math.h>
#include "synth.h"

void Synth::init(device_mode default_mode) {
    
    // MIDI init
    uart_init(MIDI_UART_INSTANCE, MIDI_BAUDRATE);
    gpio_set_function(GP_MIDI_RX, GPIO_FUNC_UART);
    m_input_buffer.init(m_buffer_var, MIDI_BUFFER_SIZE);

    for (int i = 0; i < VOICES; i++) {
        // PWM init
        gpio_set_function(settings.amp_pins[i], GPIO_FUNC_PWM);
        m_amp_pwm_slices[i] = pwm_gpio_to_slice_num(settings.amp_pins[i]);
        pwm_set_wrap(m_amp_pwm_slices[i], DIV_COUNTER);
        pwm_set_enabled(m_amp_pwm_slices[i], true);

        // Gate(s)
        // TODO:
    }

    set_mode(default_mode);
}

void Synth::init_dcos() {
    for (int i = 0; i < VOICES; i++) {
        m_set_frequency(settings.pio[settings.voice_to_pio[i]], settings.voice_to_sm[i], DEFAULT_FREQ);
    }
}

/**
 * Each mode has its own converter that implements the same interface. Here we
 * set up a pointer to the converter of the selected mode, then reset all voices
 * and gates.
*/
void Synth::set_mode(device_mode mode) {
    switch (mode) {
        case MONO:
            m_converter = &m_mono;
            break;
        case PARA:
            m_converter = &m_para;
            break;
    }

    m_converter->reset();
    m_update_dcos();
}

/**
 * The process function that's called in the main loop
*/
void Synth::process() {
    m_read_midi();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE ON event 
 * was fired.
*/
void Synth::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    m_converter->note_on(channel, note, velocity);
    m_update_dcos();
    m_update_gate();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE OFF 
 * event was fired.
*/
void Synth::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    m_converter->note_off(channel, note, velocity);
    m_update_dcos();
    m_update_gate();
}

/**
 * Callback function that the MidiParser (parent) class calls if a PITCH BEND
 * event was fired.
*/
void Synth::pitch_bend(uint8_t channel, uint16_t bend) {
    m_converter->update_pitch_bend(bend);
    m_update_dcos();
    m_update_gate();
}

/** ----------------------------------------------------------------------------
 * PRIVATE
*/

/**
 * Reads incoming MIDI messages via MidiParser parent class. This function is 
 * called for infinity from the main loop.
*/
void Synth::m_read_midi() {
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

void Synth::m_set_frequency(PIO pio, uint sm, float freq) {
    uint32_t clk_div = clock_get_hz(clk_sys) / 2 / freq;
    if (freq == 0) clk_div = 0;
    pio_sm_put(pio, sm, clk_div);
    pio_sm_exec(pio, sm, pio_encode_pull(false, false));
    pio_sm_exec(pio, sm, pio_encode_out(pio_y, 32));
}

/**
 * Updates DCOs
*/
void Synth::m_update_dcos(void) {

    // Temporarily return until I figure out how to handle paraphony
    // return;

    for (int voice = 0; voice < VOICES; voice++) {
        float freq = m_converter->get_freq(voice);
        m_set_frequency(settings.pio[settings.voice_to_pio[voice]], settings.voice_to_sm[voice], freq);

        float amp = 0;
        if (freq != 0) {
            amp = (int)(DIV_COUNTER * (freq * 0.00025f - 1 / (100 * freq)));
        }
        pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), amp);
    }
}

void Synth::m_update_gate() {
    // GATE VALUE: m_converter->get_gate();
}