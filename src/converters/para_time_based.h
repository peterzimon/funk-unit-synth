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

class ParaTimeBased: public IConverter {
public:
    void init();
    
    void reset(void);
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    float get_freq(uint8_t voice);

private:
    int m_notes[MAX_VOICES];
    int m_distinct_notes[MAX_VOICES];
    uint8_t m_no_of_distinct_notes;
    uint32_t m_voice_millis[MAX_VOICES];
    int m_filler_note;

    int m_find_voice();
    void m_update_voices();
    void m_find_filler_note();
};

#endif