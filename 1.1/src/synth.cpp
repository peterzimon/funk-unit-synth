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

    m_reset_note_history();
    m_reset_chord_notes();

    for (int i = 0; i < VOICES; i++) {
        m_notes_played[i] = -1;
        m_chord_notes[i] = -1;
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
    switch (mode) {

        // Mono and fat mode uses the same converter, the diff is only the
        // number of voices played
        case MONO:
            m_ui.chord_on = false;
            m_converter = &m_mono;

            // Reset all voices to 0V
            for (int voice = 0; voice < VOICES; voice++) {
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), 0);
            }
            break;

        case FAT_MONO:
            m_ui.chord_on = false;
            m_converter = &m_mono;
            m_voices = FAT_MONO_VOICES;

            // Reset all voices to 0V
            for (int voice = 0; voice < VOICES; voice++) {
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), 0);
            }
            break;

        case PARA:
            m_ui.chord_on = false;
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
        set_kb_tracking(m_ui.switches[mux_switch::KB_TRACKING]);

        if (m_ui.synth_mode != settings.mode) {
            set_mode(m_ui.synth_mode);
        }
    }

    // Process synth
    m_read_midi();

    if (ENABLE_CHORD_MEMORY) {
        m_set_chord();
    }

    m_update_envelope();
    m_apply_mods();
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE ON event
 * was fired.
*/
void Synth::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (channel != MIDI_CHANNEL) return;

    if (note < LOWEST_MIDI_NOTE) return;

    // When chord is ON, then fill the paraphonic player with the number of
    // notes. If a new note is played, turn off the previous note.
    if (ENABLE_CHORD_MEMORY && m_ui.chord_on) {
        chord_off();
        m_converter->reset();
        active_chord_base_note = note;
        m_converter->note_on(channel, note, velocity);
        for (int i = 1; i < m_no_of_chord_notes; i++) {
            int diff = m_chord_notes[i] - m_chord_notes[0];
            uint8_t chord_note = note + diff;
            m_converter->note_on(channel, chord_note, velocity);
        }
        m_update_dcos();
    } else {
        m_converter->note_on(channel, note, velocity);
        m_update_dcos();

        // Save played notes to history
        if (!m_no_of_played_notes) {
            m_reset_note_history();
        }
        if (m_history_records < VOICES && m_record_history) {
            m_note_history[m_no_of_played_notes] = note;
            m_history_records++;
        }

        m_notes_played[m_no_of_played_notes] = note;
        m_increase_no_of_played_notes();
    }

    // Velocity
    m_last_velocity = m_converter->get_main_velocity();
    m_update_filter_mod(m_last_velocity);     // Only update KB tracking output on note on

    // Pitch bend
    m_last_midi_pitch_bend = 0;
}

/**
 * Callback function that the MidiParser (parent) class calls if a NOTE OFF
 * event was fired.
*/
void Synth::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    if (channel != MIDI_CHANNEL) return;

    if (note < LOWEST_MIDI_NOTE) return;
    if (ENABLE_CHORD_MEMORY && m_ui.chord_on) {
        if (m_no_of_played_notes) {
            m_no_of_played_notes = 0;
        }
        m_converter->note_off(channel, note, velocity);

        for (int i = 1; i < m_no_of_chord_notes; i++) {
            // if (m_chord_notes[i] == -1) break;
            int diff = m_chord_notes[i] - m_chord_notes[0];
            uint8_t chord_note = note + diff;
            m_converter->note_off(channel, chord_note, velocity);
        }
        m_update_dcos();
    } else {
        m_converter->note_off(channel, note, velocity);
        m_update_dcos();
        m_remove_played_note(note);
        m_decrease_no_of_played_notes();
    }
}

void Synth::chord_off() {
    if (active_chord_base_note != -1) {
        m_converter->note_off(1, active_chord_base_note, 127);

        for (int i = 1; i < m_no_of_chord_notes; i++) {
            int diff = m_chord_notes[i] - m_chord_notes[0];
            uint8_t chord_note = active_chord_base_note + diff;
            m_converter->note_off(1, chord_note, 127);
        }

        active_chord_base_note = -1;
    }
}

void Synth::cc(uint8_t channel, uint8_t data1, uint8_t data2) {
    if (channel != MIDI_CHANNEL) return;

    if (data1 == 1) { // CC value 1 = modwheel
        m_modwheel = data2;
    }
    m_update_filter_mod(m_last_velocity);
}

/**
 * Callback function that the MidiParser (parent) class calls if a PITCH BEND
 * event was fired.
*/
void Synth::pitch_bend(uint8_t channel, uint16_t bend) {
    if (channel != MIDI_CHANNEL) return;

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
        m_decay = m_ui.decay_long * 100000;
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
        m_decay = m_ui.decay_long * 100000;
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
            amp = m_converter->amp_for_frequency(freq); // (int)(DIV_COUNTER * freq / MAX_FREQ);

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
                // amps[voice] = (int)(DIV_COUNTER * freqs[voice] / MAX_FREQ);
                amps[voice] = m_converter->amp_for_frequency(freqs[voice]);
            }
            break;

        case PARA:
            for (int voice = 0; voice < VOICES; voice++) {
                freqs[voice] = m_converter->get_freq(voice);
                amps[voice] = m_converter->amp_for_frequency(freqs[voice]); //(int)(DIV_COUNTER * freqs[voice] / MAX_FREQ);
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

            int amp = m_converter->amp_for_frequency(freq); // (int)(DIV_COUNTER * freq / MAX_FREQ);

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
                // pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), (int)(DIV_COUNTER * freqs[voice] / MAX_FREQ));
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), m_converter->amp_for_frequency(freqs[voice]));
            }
        }
        break;

    case PARA:
        if (m_pitch_bend_dirty) {
            for (int voice = 0; voice < VOICES; voice++) {
                float freq = m_converter->get_freq(voice);
                uint16_t amp = m_converter->amp_for_frequency(freq);
                m_last_midi_pitch_bend = m_midi_pitch_bend;
                freq = m_pitch_bend_freq(freq, m_midi_pitch_bend);
                m_set_frequency(settings.pio[settings.voice_to_pio[voice]], settings.voice_to_sm[voice], freq);
                // pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), (int)(DIV_COUNTER * freq / MAX_FREQ));
                pwm_set_chan_level(m_amp_pwm_slices[voice], pwm_gpio_to_channel(settings.amp_pins[voice]), amp);
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
    int kb_mv = 0;

    if (settings.kb_tracking) {
        int freq = (int)m_converter->get_freq(0);
        if (freq < KB_TRACK_MIN_FREQ) {
            freq = KB_TRACK_MIN_FREQ;
        } else if (freq > KB_TRACK_MAX_FREQ) {
            freq = KB_TRACK_MAX_FREQ;
        }
        kb_mv = Utils::map(freq, KB_TRACK_MIN_FREQ, KB_TRACK_MAX_FREQ, 0, FILTER_MOD_DAC_SIZE - 1);
        kb_mv = (int)((float)kb_mv * KB_TRACK_FACTOR);
    }

    if (settings.velo_tracking) {
        kb_mv += velocity * VELO_FACTOR;
    }

    int modwheel_mv = Utils::map(m_modwheel, 0, 127, 0, FILTER_MOD_DAC_SIZE - 1);
    kb_mv += modwheel_mv;

    if (kb_mv >= FILTER_MOD_DAC_SIZE) {
        kb_mv = FILTER_MOD_DAC_SIZE - 1;
    }

    m_dac.config(MCP48X2_CHANNEL_B, MCP48X2_GAIN_X2, 1);
    m_dac.write(kb_mv);
}

void Synth::m_reset_filter_mod() {
    if (!settings.kb_tracking && !settings.velo_tracking) {
        m_dac.config(MCP48X2_CHANNEL_B, MCP48X2_GAIN_X2, 1);
        m_dac.write(Utils::map(m_modwheel, 0, 127, 0, FILTER_MOD_DAC_SIZE - 1));
    }
}

float Synth::m_pitch_bend_freq(float freq, uint16_t pitch_bend) {
    return freq - (freq * ((0x2000 - pitch_bend) / (67000.0f / PITCH_BEND_SEMITONES)));
}

void Synth::m_increase_no_of_played_notes() {
    if (m_no_of_played_notes < VOICES) {
        m_no_of_played_notes++;
    }
}

void Synth::m_decrease_no_of_played_notes() {
    if (m_no_of_played_notes > 0) {
        m_no_of_played_notes--;
    }
}

void Synth::m_remove_played_note(uint8_t note) {
    // Assume these notes are played: [4, 19, 7, 12, -1, -1]
    int shift_from = 0;

    // Let's remove note 7
    for (int i = 0; i < VOICES; i++) {
        if (m_notes_played[i] == note) {
            m_notes_played[i] = -1;
            shift_from = i; // 2
            break;
        }
    }

    // Shift all the notes with higher index in the notes array
    if (shift_from) { // 2
        for (int i = shift_from; i < VOICES - 1; i++) { // From 2 -> 6
            m_notes_played[i + 1] = m_notes_played[i];

            // 2: [4, 19, 7, 12, -1, -1]    -> [4, 19, 12, 12, -1, -1]
            // 3: [4, 19, 12, 12, -1, -1]   -> [4, 19, 12, -1, -1, -1]
            // 4: [4, 19, 12, -1, -1, -1]   -> [4, 19, 12, -1, -1, -1]
            // 5: [4, 19, 12, -1, -1, -1]   -> [4, 19, 12, -1, -1, -1]
        }
        m_notes_played[VOICES - 1] = -1; // Last note is always -1 if there was at least one shift
    }
}

void Synth::m_reset_note_history() {
    for (int i = 0; i < VOICES; i++) {
        m_note_history[i] = -1;
    }
    m_history_records = 0;
}

void Synth::m_reset_chord_notes() {
    for (int i = 0; i < VOICES; i++) {
        m_chord_notes[i] = -1;
    }
}

void Synth::m_set_chord() {
    if (settings.mode != PARA && m_ui.chord_on) {
        m_ui.chord_on = false;
        return;
    }

    if (m_ui.chord_on) {
        if (!m_chord_set) {
            m_reset_chord_notes();
            m_no_of_chord_notes = 0;
            if (!m_no_of_played_notes) {

                // Read notes from history
                for (int i = 0; i < VOICES; i++) {
                    if (m_note_history[i] != -1) {
                        m_chord_notes[m_no_of_chord_notes] = m_note_history[i];
                        m_no_of_chord_notes++;
                        m_chord_set = true;
                        if (m_no_of_chord_notes >= VOICES) {
                            break;
                        }
                    }
                }

                // Turn off chord if there's no notes to build a chord from
                if (!m_chord_set) {
                    m_ui.chord_on = false;
                }
            } else {
                for (int i = 0; i < VOICES; i++) {
                    if (m_notes_played[i] != -1) {
                        m_chord_notes[m_no_of_chord_notes] = m_notes_played[i];
                        m_no_of_chord_notes++;
                    }
                }
                active_chord_base_note = m_chord_notes[0];
                m_chord_set = true;
            }

            if (m_chord_set) {
                m_record_history = false;
            }
        }
    } else {
        chord_off();
        if (m_chord_set && m_ui.reset_chord) {
            m_reset_chord_notes();
            m_ui.reset_chord = false;
            m_chord_set = false;
            m_record_history = true;
        }
    }
}