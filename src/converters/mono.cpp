#include "mono.h"

void Mono::reset() {
    m_gate = false;

    // Set the first note of the note stack to -1. This indicates that there's
    // no incoming note (all notes released). The new notes always push all the
    // other notes in the stack to the right but only until [stack size - 1]
    // is reached. This way -1 never gets deleted from the stack so we always
    // know when all notes are released.
    m_note_stack[0] = -1;
}

void Mono::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    set_dirty(true);
    m_push_note(note);

    m_note = note;
    m_gate = true;

    if (settings.portamento) {
        if (m_portamento_start == -1) {
            m_portamento_start = note;
            m_portamento_current_freq = 0.0f;
        } else {
            m_portamento_stop = note;
        }
    }

    m_debug();
}

void Mono::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    set_dirty(true);

    bool last_note = false;
    if (m_note_stack[1] == -1) {
        m_note = m_note_stack[0];
        last_note = true;
    }
    m_pop_note(note);
    if (!last_note && m_note_stack[0] != -1) m_note = m_note_stack[0];
    m_gate = (m_note_stack[0] != -1);

    if (m_note_stack[0] == -1) {
        m_portamento_start = note;
    } else {
        m_portamento_stop = m_note;
    }

    m_debug();
}

float Mono::get_freq(uint8_t voice) {
    if (m_note == -1) return 0;

    if (settings.portamento && m_portamento_start != -1 && m_portamento_stop != -1) {
        float freq1 = pow(2, (m_portamento_start - 69) / 12.0f) * BASE_NOTE;
        float freq2 = pow(2, (m_portamento_stop - 69) / 12.0f) * BASE_NOTE;

        if (m_portamento_current_freq == 0) {
            m_portamento_current_freq = freq1;
        } else {

            // Next note is higher than current
            if (freq1 < freq2) {
                m_portamento_current_freq += 1.0f / (m_portamento_time + 1);
                if (m_portamento_current_freq > freq2) {
                    m_portamento_current_freq = freq2;
                    set_dirty(false);
                }

            // Next note is lower than current
            } else {
                m_portamento_current_freq -= 1.0f / (m_portamento_time + 1);
                if (m_portamento_current_freq < freq2) {
                    m_portamento_current_freq = freq2;
                    set_dirty(false);
                }
            }
        }

        return m_portamento_current_freq;
    }

    return frequency_from_midi_note(m_note);
}

bool Mono::get_gate() {
    return m_gate;
}

void Mono::m_push_note(uint8_t note) {

    // Check if note exists in the stack. If it does, do nothing unless the last
    // note is -1 (gate is off)
    if (m_find_note(note)) {
        return;
    }

    // Use two temp variables to push notes in the stack to the right. The last
    // note played is always in note_stack[0].
    int i = 0;
    int temp = m_note_stack[0], temp2;

    // We have to keep a 0 in the last stack position to be able to zero out
    // all notes when popping them out of the stack on note off events. That's
    // the reason we loop only until NOTE_STACK_SIZE - 1.
    while (temp && (i < NOTE_STACK_SIZE - 1)) {
         // Right shift stack
        temp2 = temp;
        temp = m_note_stack[i + 1];
        if (i < (NOTE_STACK_SIZE - 1)) {
            m_note_stack[i + 1] = temp2;
        }
        i++;
    }

    // Put the freshest note to the first position in the stack
    m_note_stack[0] = note;
}

void Mono::m_pop_note(uint8_t note) {
    bool shift = false;

    // Find the released note and remove from the stack while pushing
    // all other notes to the left. The last element of the stack is
    // always 0.
    for (int i = 0; i < (NOTE_STACK_SIZE - 1); i++) {
        if (m_note_stack[i] == note) {
            shift = true;
        }
        if (shift) {
            m_note_stack[i] = m_note_stack[i + 1];
        }
    }
}

int Mono::m_find_note(uint8_t note) {
    for (int i = 0; i < NOTE_STACK_SIZE; i++) {
        if (m_note_stack[i] == note) return i;
    }
    return 0;
}

void Mono::m_debug() {
    // return;
    printf("Note: %d\n", m_note);
    printf("Note stack:\n");
    for (int i = 0; i < VOICES; i++) {
        printf("%d: %d \n", i, m_note_stack[i]);
    }
    printf("Gate: %d\n", m_gate);
    printf("---\r\n\n");
}