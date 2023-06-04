#ifndef _MIDI_HANDLER_H
#define _MIDI_HANDLER_H

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
#define ENVELOPE_DAC_SIZE 4096

#define ATTACK_SHORT            10000          // us
#define ATTACK_LONG             1000000        // us
#define DECAY_SHORT             10000          // us
#define DECAY_LONG              15000000       // us
#define SUSTAIN_ON              3200           // mV, between 0 and 4095
#define SUSTAIN_OFF             0              // mV, between 0 and 4095
#define RELEASE_SHORT           10000          // us
#define RELEASE_LONG            15000000       // us

const uint16_t DIV_COUNTER = 1250;

class Synth: public MidiParser {
public:
    static Synth& get_instance(int adsrDacSize) {
        static Synth instance(adsrDacSize);
        return instance;
    }

    DISALLOW_COPY_AND_ASSIGN(Synth);

    void init(device_mode default_mode);
    void init_dcos();
    void process();

    void set_mode(device_mode mode);
    
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    void pitch_bend(uint8_t channel, uint16_t bend);
    
    void set_adsr(bool soft, bool hold, bool ring);


protected:
    Synth(int adsrDacSize);
//     Synth(int adsrDacSize) = default;

private:
    IConverter *m_converter;
    Mono m_mono;
    Para m_para;

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
    void m_update_gate();
    void m_update_envelope();

};

#endif