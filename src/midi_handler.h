#ifndef _MIDI_HANDLER_H
#define _MIDI_HANDLER_H

#include "settings.h"
#include "ui.h"
#include "math.h"

#include <mcp48x2.h>
#include <utils.h>
#include <midi_parser.h>
#include <ringbuffer.h>

#include "i_converter.h"
#include "./converters/para.h"

#define MIDI_BUFFER_SIZE 32

class MidiHandler: public MidiParser {
public:
    static MidiHandler& get_instance() {
        static MidiHandler instance;
        return instance;
    }

    DISALLOW_COPY_AND_ASSIGN(MidiHandler);

    void init();
    // void attach(MCP48X2 *dac);
    void process();

    void set_mode();
    
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    void pitch_bend(uint8_t channel, uint16_t bend);

protected:
    MidiHandler() = default;

private:
    IConverter *m_converter;
    Para m_para;

    int m_gates[MAX_VOICES];
    uint16_t m_cvs[MAX_VOICES];

    bool m_pitch_bend_dirty;
    int16_t m_pitch_bend_cv;

    uint8_t m_buffer_var[MIDI_BUFFER_SIZE];
    RingBuffer m_input_buffer;

    MCP48X2 *m_dac_1;
    MCP48X2 *m_dac_2;
    UI &m_ui = UI::get_instance();

    void m_read_midi();
    bool m_any_gate_on();
    void m_update_output(void);
};

#endif