#ifndef _MONO_H
#define _MONO_H

#include <stdio.h>
#include "math.h"

#include <utils.h>

#include "../settings.h"
#include "../i_converter.h"

#define NOTE_STACK_SIZE 25

class Mono: public IConverter {
public:

    void reset(void);
    void note_on(uint8_t channel, uint8_t note, uint8_t velocity);
    void note_off(uint8_t channel, uint8_t note, uint8_t velocity);
    float get_freq(uint8_t voice);
    bool get_gate();
private:
    int m_note_stack[NOTE_STACK_SIZE];
    bool m_note_playing;

    int m_note;
    int m_keys_pressed;
    bool m_gate;

    void m_push_note(uint8_t note);
    void m_pop_note(uint8_t note);
    int m_find_note(uint8_t note);
    void m_debug();
};

#endif