#include <stdio.h>
#include <math.h>
#include "synth.h"

// Constructor with
Synth::Synth(): m_adsr(ENVELOPE_DAC_SIZE) {}

void Synth::init(device_mode default_mode) {

    // DAC init
    sleep_ms(500);
    m_dac.init(DAC_SPI_PORT, GP_DAC_CS, GP_DAC_SCK, GP_DAC_MOSI);

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

    }

    set_mode(default_mode);

    // Set default ADSR (soft, hold, ring)
    set_adsr(false, true, false);
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

    printf("NEW MODE! %d\n", static_cast<int>(mode));

    switch (mode) {

        // Mono and fat mode uses the same converter, the diff is only the
        // number of voices played
        case MONO:
            m_converter = &m_mono;

            // Reset all voices to 0V
            for (int voice = 0; voice < VOICES; voice++) {
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), 0);
            }
            break;

        case FAT_MONO:
            m_converter = &m_mono;
            m_voices = FAT_MONO_VOICES;

            // Reset all voices to 0V
            for (int voice = 0; voice < VOICES; voice++) {
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), 0);
            }
            break;

        case PARA:
            m_converter = &m_para;
            m_voices = VOICES;
            m_converter->set_dirty(true);
            break;
    }

    settings.mode = mode;
    m_converter->reset();
    m_update_dcos();
}

void Synth::set_kb_tracking(bool kb_tracking) {
    settings.kb_tracking = kb_tracking;
    m_reset_filter_mod();
}

void Synth::set_velo_tracking(bool velo_tracking) {
    settings.velo_tracking = velo_tracking;
    m_reset_filter_mod();
}

/**
 * Called in the main loop
*/
void Synth::process() {
    // Update settings from switches and pots
    if (m_ui.updated) {
        set_adsr(m_ui.switches[mux_switch::SOFT], m_ui.switches[mux_switch::HOLD], m_ui.switches[mux_switch::RING]);
        set_portamento(m_ui.switches[mux_switch::PORTAMENTO]);
        set_detune(m_ui.switches[mux_switch::DETUNE]);
        set_solo(m_ui.switches[mux_switch::SOLO_CHORD]);
        set_velo_tracking(m_ui.switches[mux_switch::WAH_VELOCITY]);

        if (m_ui.synth_mode != settings.mode) {
            set_mode(m_ui.synth_mode);
        }
    }

    // Process synth
    m_read_midi();
    m_update_envelope();
    m_apply_mods();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE ON event
 * was fired.
*/
void Synth::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (note < LOWEST_MIDI_NOTE) return;
    m_converter->note_on(channel, note, velocity);
    m_update_dcos();
    m_update_filter_mod(m_converter->get_main_velocity());     // Only update KB tracking output on note on
    m_last_midi_pitch_bend = 0;
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE OFF
 * event was fired.
*/
void Synth::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (note < LOWEST_MIDI_NOTE) return;
    m_converter->note_off(channel, note, velocity);
    m_update_dcos();
}

/**
 * Callback function that the MidiParser (parent) class calls if a PITCH BEND
 * event was fired.
*/
void Synth::pitch_bend(uint8_t channel, uint16_t bend) {
    // m_converter->update_pitch_bend(bend);
    // m_update_dcos();

    m_midi_pitch_bend = bend;
    m_pitch_bend_dirty = (m_midi_pitch_bend != m_last_midi_pitch_bend);
}

/**
 * Handling all the states that the ADSR can get to based on the Soft, Hold and
 * Ring switches.
*/
void Synth::set_adsr(bool soft, bool hold, bool ring) {
    int shr = (soft << 2) | (hold << 1) | ring;

    switch (shr) {
    case 0b001:
        m_attack = ATTACK_SHORT;
        m_decay = DECAY_LONG;
        m_sustain = SUSTAIN_OFF;
        m_release = RELEASE_SHORT;
        break;
    case 0b010:
        m_attack = ATTACK_SHORT;
        m_decay = DECAY_SHORT;
        m_sustain = SUSTAIN_ON;
        m_release = RELEASE_SHORT;
        break;
    case 0b011:
        m_attack = ATTACK_SHORT;
        m_decay = DECAY_SHORT;
        m_sustain = SUSTAIN_ON;
        m_release = m_ui.release_long * 100000;
        break;
    case 0b100:
        m_attack = ATTACK_LONG;
        m_decay = DECAY_SHORT;
        m_sustain = SUSTAIN_OFF;
        m_release = RELEASE_SHORT;
        break;
    case 0b101:
        m_attack = ATTACK_LONG;
        m_decay = DECAY_LONG;
        m_sustain = SUSTAIN_OFF;
        m_release = RELEASE_SHORT;
        break;
    case 0b110:
        m_attack = ATTACK_LONG;
        m_decay = DECAY_SHORT;
        m_sustain = SUSTAIN_ON;
        m_release = RELEASE_SHORT;
        break;
    case 0b111:
        m_attack = ATTACK_LONG;
        m_decay = DECAY_SHORT;
        m_sustain = SUSTAIN_ON;
        m_release = m_ui.release_long * 100000;
        break;
    default:
        m_attack = ATTACK_SHORT;
        m_decay = DECAY_MID;
        m_sustain = SUSTAIN_OFF;
        m_release = RELEASE_SHORT;
        break;
    }

    m_adsr.set_attack(m_attack);
    m_adsr.set_decay(m_decay);
    m_adsr.set_sustain(m_sustain);
    m_adsr.set_release(m_release);
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
    if (m_converter->is_dirty()) {
        // Sometimes the delay in setting the frequency of the PIOs can cause them
        // to be out of phase by 180deg. This causes phase cancellation with voices
        // playing the same note (e.g. in mono mode). To minimise the chances of
        // this, first calculate the frequencies and amplitudes (which require time)
        // and set the frequency and amp only after then, in a separate loop.
        float freq;
        int amp;
        float freqs[VOICES];
        uint16_t amps[VOICES];

        switch (settings.mode)
        {
        case MONO:
            freq = m_converter->get_freq(0);
            amp = (int)(DIV_COUNTER * freq / MAX_FREQ);

            m_set_frequency(settings.pio[settings.voice_to_pio[0]], settings.voice_to_sm[0], freq);
            pwm_set_chan_level(m_amp_pwm_slices[0], pwm_gpio_to_channel(settings.amp_pins[0]), amp);
            return;
            break;

        case FAT_MONO:
            freq = m_converter->get_freq(0);

            freqs[0] = freqs[1] = freqs[2] = freq;
            if (settings.detune) {
                freqs[1] = freq * DETUNE_FACTOR;
                freqs[2] = freq * (1.0 - (DETUNE_FACTOR - 1.0));
            }
            for (int voice = 0; voice < FAT_MONO_VOICES; voice++) {
                amps[voice] = (int)(DIV_COUNTER * freqs[voice] / MAX_FREQ);
            }
            break;

        case PARA:
            for (int voice = 0; voice < VOICES; voice++) {
                freqs[voice] = m_converter->get_freq(voice);
                amps[voice] = (int)(DIV_COUNTER * freqs[voice] / MAX_FREQ);
            }
            break;
        }

        for (int voice = 0; voice < m_voices; voice++) {
            m_set_frequency(settings.pio[settings.voice_to_pio[voice]], settings.voice_to_sm[voice], freqs[voice]);
            pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), amps[voice]);
        }
    }
}

void Synth::m_apply_mods() {
    switch (settings.mode)
    {
    case MONO:
        if (settings.portamento && m_converter->is_dirty() || m_pitch_bend_dirty) {
            float freq = m_converter->get_freq(0);

            m_last_midi_pitch_bend = m_midi_pitch_bend;
            freq = m_pitch_bend_freq(freq, m_midi_pitch_bend);

            int amp = (int)(DIV_COUNTER * freq / MAX_FREQ);

            m_set_frequency(settings.pio[settings.voice_to_pio[0]], settings.voice_to_sm[0], freq);
            pwm_set_chan_level(m_amp_pwm_slices[0], pwm_gpio_to_channel(settings.amp_pins[0]), amp);
        }
        break;

    case FAT_MONO:
        if (settings.portamento && m_converter->is_dirty() || m_pitch_bend_dirty) {
            float freq = m_converter->get_freq(0);

            m_last_midi_pitch_bend = m_midi_pitch_bend;
            freq = m_pitch_bend_freq(freq, m_midi_pitch_bend);

            float freqs[VOICES];
            freqs[0] = freqs[1] = freqs[2] = freq;
            if (settings.detune) {
                freqs[1] = freq * DETUNE_FACTOR;
                freqs[2] = freq * (1.0 - (DETUNE_FACTOR - 1.0));
            }

            for (int voice = 0; voice < FAT_MONO_VOICES; voice++) {
                m_set_frequency(settings.pio[settings.voice_to_pio[voice]], settings.voice_to_sm[voice], freqs[voice]);
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), (int)(DIV_COUNTER * freqs[voice] / MAX_FREQ));
            }
        }
        break;

    case PARA:
        if (m_pitch_bend_dirty) {
            for (int voice = 0; voice < VOICES; voice++) {
                float freq = m_converter->get_freq(voice);
                m_last_midi_pitch_bend = m_midi_pitch_bend;
                freq = m_pitch_bend_freq(freq, m_midi_pitch_bend);
                m_set_frequency(settings.pio[settings.voice_to_pio[voice]], settings.voice_to_sm[voice], freq);
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), (int)(DIV_COUNTER * freq / MAX_FREQ));
            }
        }
        break;

    default:
        break;
    }
}

void Synth::m_update_envelope() {

    // Trigger ADSR only if the gate is on and it's not already on
    if (m_converter->get_gate() && !m_adsr.is_on()) {
        m_adsr.note_on();
    } else if (!m_converter->get_gate() && m_adsr.is_on()) {
        m_adsr.note_off();
    }

    // Set DAC channel A. TODO: update MCP48X2 library to be able to set
    // channel with its own method
    m_dac.config(MCP48X2_CHANNEL_A, MCP48X2_GAIN_X2, 1);
    m_dac.write(m_adsr.envelope());
}

void Synth::m_update_filter_mod(uint8_t velocity) {
    if (!settings.kb_tracking && !settings.velo_tracking) return;

    int kb_mv = 0;

    if (settings.kb_tracking) {
        int freq = (int)m_converter->get_freq(0);
        if (freq < KB_TRACK_MIN_FREQ) {
            freq = KB_TRACK_MIN_FREQ;
        } else if (freq > KB_TRACK_MAX_FREQ) {
            freq = KB_TRACK_MAX_FREQ;
        }
        kb_mv = Utils::map(freq, KB_TRACK_MIN_FREQ, KB_TRACK_MAX_FREQ, 0, FILTER_MOD_DAC_SIZE);
    }

    if (settings.velo_tracking) {
        kb_mv += velocity * VELO_FACTOR;
    }

    m_dac.config(MCP48X2_CHANNEL_B, MCP48X2_GAIN_X2, 1);
    m_dac.write(kb_mv);
}

void Synth::m_reset_filter_mod() {
    if (!settings.kb_tracking && !settings.velo_tracking) {
        m_dac.config(MCP48X2_CHANNEL_B, MCP48X2_GAIN_X2, 1);
        m_dac.write(0);
    }
}

float Synth::m_pitch_bend_freq(float freq, uint16_t pitch_bend) {
    return freq - (freq * ((0x2000 - pitch_bend) / (67000.0f / PITCH_BEND_SEMITONES)));
}