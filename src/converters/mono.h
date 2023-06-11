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

    int m_note;
    int m_keys_pressed;
    bool m_gate;

    uint8_t m_portamento_time = PORTAMENTO_TIME;
    int m_portamento_start = -1, m_portamento_stop = -1;
    float m_portamento_current_freq = 0.0f;

    void m_push_note(uint8_t note);
    void m_pop_note(uint8_t note);
    int m_find_note(uint8_t note);
    void m_debug();
};

#endif