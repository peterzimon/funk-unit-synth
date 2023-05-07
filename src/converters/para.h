#ifndef _PARA_H
#define _PARA_H

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
    void get_cv_gate(uint16_t *cv, int *gate);

private:
    int m_notes[MAX_VOICES];
    int m_voices[MAX_VOICES];
    int m_cvs[MAX_VOICES];
    int m_filler_note;
    int m_lru[MAX_VOICES];

    void m_update_unused_voices();
    int m_find_new_note_index(uint8_t note);
    void m_find_filler_note();
    int m_find_note_index(uint8_t note);

    void m_lru_add_note_index(int note_index);
    void m_lru_remove_note_index(int note_index);
    int m_lru_find_note_index(int note_index);
    void m_lru_shift_left(int from_index);
    void m_lru_shift_right();
};

#endif