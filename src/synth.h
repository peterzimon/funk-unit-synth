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

#include "i_converter.h"
#include "./converters/para.h"
#include "./converters/para_time_based.h"

#define MIDI_BUFFER_SIZE 32
const uint16_t DIV_COUNTER = 1250;

class Synth: public MidiParser {
public:
    static Synth& get_instance() {
        static Synth instance;
        return instance;
    }

    DISALLOW_COPY_AND_ASSIGN(Synth);

    void init();
    void process();

    void set_mode();
    
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    void pitch_bend(uint8_t channel, uint16_t bend);


protected:
    Synth() = default;

private:
    IConverter *m_converter;
    Para m_para;
    ParaTimeBased m_para_time_based;

    uint8_t m_amp_pwm_slices[MAX_VOICES];

    bool m_pitch_bend_dirty;
    int16_t m_pitch_bend_cv;

    uint8_t m_buffer_var[MIDI_BUFFER_SIZE];
    RingBuffer m_input_buffer;

    UI &m_ui = UI::get_instance();

    void m_read_midi();
    void m_set_frequency(PIO pio, uint sm, float freq);
    void m_update_dcos(void);
};

#endif