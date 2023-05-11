#ifndef _PARA_TIME_BASED_H
#define _PARA_TIME_BASED_H

/**
 * Paraphonic converter
 * 
 * To be documented...
 */

#include "../settings.h"
#include <mcp48x2.h>
#include "../ui.h"
#include "math.h"
#include <utils.h>
#include <midi_parser.h>
#include <ringbuffer.h>
#include "../i_converter.h"

class Para: public IConverter {
public:
    void init();
    
    void reset(void);
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    float get_freq(uint8_t voice);
    bool get_gate();

private:
    int m_notes[VOICES];
    uint32_t m_voice_millis[VOICES];
    int m_filler_note;
    bool m_reset;

    int m_find_voice();
    void m_distribute_notes();
    void m_debug();
};

#endif