#ifndef _SYNTH_H
#define _SYNTH_H

#include "settings.h"
#include "ui.h"
#include "math.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include <utils.h>
#include <midi_parser.h>
#include <ringbuffer.h>
#include <adsr.h>
#include <mcp48x2.h>

#include "i_converter.h"
#include "./converters/para.h"
#include "./converters/mono.h"

#define MIDI_BUFFER_SIZE 32

const uint16_t DIV_COUNTER = 1250;

class Synth: public MidiParser {
public:
    static Synth& get_instance() {
        static Synth instance;
        return instance;
    }

    DISALLOW_COPY_AND_ASSIGN(Synth);

    void init(device_mode default_mode);
    void init_dcos();
    void process();

    void set_mode(device_mode mode);
    void set_solo(bool solo) { settings.solo = solo; }
    void set_detune(bool detune) { settings.detune = detune; }
    void set_portamento(bool portamento) { settings.portamento = portamento; }

    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    void pitch_bend(uint8_t channel, uint16_t bend);

    void set_adsr(bool soft, bool hold, bool ring);

protected:
    Synth();

private:
    IConverter *m_converter;
    Mono m_mono;
    Para m_para;
    int m_voices = VOICES;

    uint64_t m_attack;
    uint64_t m_decay;
    int m_sustain;
    uint64_t m_release;

    ADSR m_adsr;
    MCP48X2 m_dac;

    uint8_t m_amp_pwm_slices[VOICES];

    bool m_pitch_bend_dirty;
    int16_t m_pitch_bend_cv;

    uint8_t m_buffer_var[MIDI_BUFFER_SIZE];
    RingBuffer m_input_buffer;

    UI &m_ui = UI::get_instance();

    void m_read_midi();
    void m_set_frequency(PIO pio, uint sm, float freq);
    void m_update_dcos(void);
    void m_update_envelope();

    void m_update_kb_tracking();
    void m_apply_mods();
};

#endif