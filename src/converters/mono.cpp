#include "mono.h"

void Mono::reset() {
    m_gate = false;

    // Set the first note of the note stack to -1. This indicates that there's
    // no incoming note (all notes released). The new notes always push all the
    // other notes in the stack to the right but only until [stack size - 1]
    // is reached. This way -1 never gets deleted from the stack so we always
    // know when all notes are released.
    m_note_stack[0] = -1;

    m_debug();
}

void Mono::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    // Do nothing if it's the same note that's already been playing
    if (m_note == note) {
        return;
    }

    m_push_note(note);
    m_note = note;
    m_note_playing = (m_note_stack[0] != -1);

    m_debug();
}

void Mono::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    bool last_note = false;
    if (m_note_stack[1] == -1) {
        m_note = m_note_stack[0];
        last_note = true;
    }
    m_pop_note(note);
    if (!last_note) m_note = m_note_stack[0];
    m_note_playing = (m_note_stack[0] != -1);

    m_debug();
}

float Mono::get_freq(uint8_t voice) {
    if (m_note == -1) return 0;
    return frequency_from_midi_note(m_note);
}

bool Mono::get_gate() {
    return m_gate;
}

void Mono::m_push_note(uint8_t note) {

    // Check if note exists in the stack. If it does, do nothing.
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
    printf("Notes / voices:\n");
    for (int i = 0; i < VOICES; i++) {
        printf("%d: %d %d \n", i, m_note, m_note_stack[i]);
    }
    printf("Gate: %d\n", m_gate);
    printf("---\r\n\n");
}