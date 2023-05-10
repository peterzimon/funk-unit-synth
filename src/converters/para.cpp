#include "para.h"
#include "../settings.h"

/**
 * Resets everything
*/
void Para::reset() {
    for (int i = 0; i < VOICES; i++) {
        m_notes[i] = -1;
        m_voice_millis[i] = 0;
    }

    m_reset = true;
    m_debug();
}

/**
 * If it's the first note after releasing all notes (or at the very beginning), 
 * then all voices will play the same note. After this, all new notes will be 
 * equally distributed amongst the unused voices. Once all voices are used then
 * the least recently used will be used for any new note.
 * 
 * This logic allows using a single gate (the first voice's gate!) to trigger
 * the ADSR.
*/
void Para::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {

    if (m_reset) {
        for (int i = 0; i < VOICES; i++) {
            m_notes[i] = (int)note;
        }
        m_voice_millis[0] = Utils::millis();
        m_reset = false;
        m_debug();
        return;
    }

    // Return if note is already playing
    for (int i = 0; i < VOICES; i++) {
        if (m_notes[i] == (int)note) return;
    }

    // If the note is not playing find a voice for the new note
    int new_note_index = m_find_voice();
    m_voice_millis[new_note_index] = Utils::millis();
    m_notes[new_note_index] = (int)note;

    m_update_voices();
}

/**
 * Handling NOTE OFF event. MIDI events are called in the MidiHandler class.
*/
void Para::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    for (int i = 0; i < VOICES; i++) {
        if (m_notes[i] == (int)note) {
            m_voice_millis[i] = 0;
        }
    }

    m_reset = true;
    for (int i = 0; i < VOICES; i++) {
        if (m_voice_millis[i] != 0) {
            m_reset = false;
            return;
        }
    }
}

/**
 * Return the frequency of a voice. Return 0Hz if no note is set for the 
 * given voice. 
 * 
 * TODO: 0 AS DEFAULT MAY NOT WORK BECAUSE OF DIVISION BY 0 IN THE PIO!! 
 * TO BE TESTED!!
*/
float Para::get_freq(uint8_t voice) {
    if (m_notes[voice] == -1) return 0;
    return frequency_from_midi_note(m_notes[voice]);
}

/**
 * Actually when the reset is set is exactly when the gate is released.
*/
bool Para::get_gate() {
    return !m_reset;
}


/** ----------------------------------------------------------------------------
 * PRIVATE
*/

/**
 * Finds the next voice for most recently played note
*/
int Para::m_find_voice() {
    uint32_t oldest_millis = Utils::millis();
    int oldest_voice = 0;

    for (int i = 0; i < VOICES; i++) {
        
        // Return if there's a free voice
        if (m_voice_millis[i] == 0) return i;
        
        // Find the least recently used voice
        if (m_voice_millis[i] < oldest_millis) {
            oldest_millis = m_voice_millis[i];
            oldest_voice = i;
        }
    }

    return oldest_voice;
}

/**
 * Distributes played notes amongst unused voices equally so if e.g. only 
 * two note are held but there are 6 voices then each note would be played by
 * 3 voices. If only 1 note is held then the same note would be played by all 
 * voices (fallback to unison mode).
*/
void Para::m_update_voices() {
    
    // Collect distinct notes
    int distinct_notes[VOICES];
    int no_of_distinct_notes = 0;
    for (int i = 0; i < VOICES; i++) {
        if (m_voice_millis[i] != 0) {
            distinct_notes[no_of_distinct_notes] = m_notes[i];
            no_of_distinct_notes++;
        }
    }
    
    // Distribute distinct notes amongst unused voices
    if (no_of_distinct_notes > 0) {
        int next_distinct_note_index = 0;
        for (int i = 0; i < VOICES; i++) {
            if (m_voice_millis[i] == 0) {
                m_notes[i] = distinct_notes[next_distinct_note_index];
                next_distinct_note_index++;
                if (next_distinct_note_index >= no_of_distinct_notes) next_distinct_note_index = 0;
            }
        }
    }

    m_debug();
}

void Para::m_debug() {
    printf("Notes / voices:\n");
    for (int i = 0; i < VOICES; i++) {
        printf("%d: %d %d\n", i, m_notes[i], m_voice_millis[i]);
    }
    printf("---\r\n\n");
}