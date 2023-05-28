#ifndef _PARA_H
#define _PARA_H

/**
 * Paraphonic converter
 * 
 * To be documented...
 */

#include <stdio.h>
#include "math.h"

#include <utils.h>

#include "../settings.h"
#include "../i_converter.h"

class Para: public IConverter {
public:    
    void reset(void);
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    float get_freq(uint8_t voice);
    bool get_gate();

private:
    int m_notes[VOICES];
    uint32_t m_voice_millis[VOICES];
    bool m_reset;

    int m_find_voice();
    bool m_last_note_playing(int note);
    void m_distribute_notes();
    void m_debug();
};

#endif