#include "para.h"
#include "../settings.h"

/**
 * Resets the whole shabang
*/
void Para::reset() {
    for (int i = 0; i < MAX_VOICES; i++) {
        m_lru[i] = m_notes[i] = m_cvs[i] = -1;
        m_voices[i] = 0;
    }
}

/**
 * Handling NOTE ON event. MIDI events are called in the MidiHandler class.
 * 1. Check if note is already playing
 * 2. Find new note index
 * 3. Set voice for new note
 * 4. Loop through voices and fill all non-playing with one of the existing
 *    notes. If less notes is played than the number of voices then the notes
 *    are equally distributed amongst the voices.
*/
void Para::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == (int)note) return;
    }

    int new_note_index = m_find_new_note_index(note);
    if (new_note_index == -1) return; // Shit happened

    m_notes[new_note_index] = note;
    m_voices[new_note_index] = note;

    m_update_unused_voices();
}

/**
 * Handling NOTE OFF event. MIDI events are called in the MidiHandler class.
 * 1. Find released note's index
 * 2. Remove note from notes array
 * 3. Maintain LRU
 * 4. Updated unused voices similarly as in NOTE ON
*/
void Para::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {

    int note_off_index = m_find_note_index(note);
    if (note_off_index == -1) return; // The note is not played

    m_notes[note_off_index] = -1;

    m_lru_remove_note_index(note_off_index);
    m_update_unused_voices();
}

/**
 * Return the CV and gate values to MidiHandler (this is called in MidiHandler)
*/
void Para::get_cv_gate(uint16_t *cv, int *gate) {
    // printf("\r\nVoices: [");
    for (int voice = 0; voice < settings.voices; voice++) {
        // printf("%d, ", m_voices[voice]);
        if (m_voices[voice] != -1) {
            cv[voice] = cv_for_note(m_voices[voice], voice); // Parent class method
        }
        gate[voice] = (m_notes[voice] != -1) ? 1 : 0;
    }
    // printf("]");
}

/**
 * Return whatever is needed for DCO. I guess it's the frequency and the
 * amplitude value
*/
void Para::get_freq_amp() {
    
}


/** ----------------------------------------------------------------------------
 * PRIVATE
*/

/**
 * Since the converter is optimised for paraphonic use  all voices have to be 
 * used all the time. For this we need to find filler notes where the 
 * corresponding note = -1.
*/
void Para::m_update_unused_voices() {
    m_filler_note = -1;
    bool all_notes_released = true;
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == -1) {
            m_find_filler_note();
            if (m_filler_note != -1) {
                m_voices[i] = m_filler_note;
            }
        } else {
            all_notes_released = false;
        }
    }

    // Do a full reset if all notes are released
    if (all_notes_released) {
        for (int i = 0; i < settings.voices; i++) {
            m_lru[i] = m_notes[i] = m_cvs[i] = -1;
        }
    }
}

/**
 * Finds a NEW note index for an incoming note.
*/
int Para::m_find_new_note_index(uint8_t note) {
    
    // Check if there's any empty note slot (empty: where note == -1)
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == -1) {
            m_notes[i] = note;

            // Maintain LRU
            m_lru_add_note_index(i);
            return i;
        }
    }

    // If there is no empty note slot then take the least recently used, which
    // always the first item in the LRU stack
    int least_recently_used_voice = m_lru[0];
    
    // This case we need to shift the LRU to the left and put the new slot to 
    // the end of the stack (ie. it becomes most recently used). So:
    // [0, 2, 1, 3] becomes [2, 1, 3, 0]
    m_lru_shift_left(1);
    m_lru[settings.voices - 1] = least_recently_used_voice;

    return least_recently_used_voice;
}

/**
 * Finds filler notes for unused voices (unused: where corresponding note = -1).
 * It distributes played notes amongst empty voices equally so if e.g. only 
 * two note are held but there are 4 voices then each note would be played by
 * two voices. If only 1 note is held then the same note would be played by all 
 * voices (fallback to unison mode).
*/
void Para::m_find_filler_note() {
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] != -1 && m_notes[i] != m_filler_note) {
            m_filler_note = m_notes[i];
            break;
        }
    }
}

/**
 * Finds the note index of a played note
*/
int Para::m_find_note_index(uint8_t note) {
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == note) {
            return i;
        }
    }
    return -1;
}

/**
 * Adds a new note index to the LRU
*/
void Para::m_lru_add_note_index(int note_index) {
    for (int i = 0; i < settings.voices; i++) {
        if (m_lru[i] == -1) {
            m_lru[i] = note_index;
            return;
        }
    }
}

/**
 * Remove note index from LRU
 * 
 * Because the LRU logic relies on the position of the indexes in the LRU stack
 * it's not enough to simply overwrite the given note index with -1. The stack
 * also has to be shifted left and the empty slots must be -1 to be consistent
 * with notes and voices arrays.
 * 
 * Example #1:
 *      LRU: [1, 3, 0, -1]
 *      -> remove "3" and shift from "0" to left. Call left_shift function with 
 *         shift_index = 2.
 *      i: 1 -> condition false (i !>= index)
 *      i: 2 -> [1, 0, 0, -1]
 *      i: 3 -> [1, 0, 2, -1]
 * 
 * Example #2:
 *      LRU: [1, 3, 0, 2]
 *      -> remove "3" and shift from "0" to left. Call left_shift function with 
 *         shift_index = 2.
 *      i: 1 -> condition false (i !>= index)
 *      i: 2 -> [1, 0, 0, 2]
 *      i: 3 -> [1, 0, 2, 2]
 * 
 *      -> last has to be -1 to make space for new note
 *      [1, 0, 2, -1]
*/
void Para::m_lru_remove_note_index(int note_index) {
    
    // Get the LRU index of the released note
    int lru_index = m_lru_find_note_index(note_index);

    // Left shift LRU after the index of the released note
    m_lru_shift_left(lru_index + 1);

    // Last LRU index has to be -1 when a note is removed
    m_lru[settings.voices - 1] = -1;
}

/**
 * Finds the note index in the LRU and returns the LRU index for it
*/
int Para::m_lru_find_note_index(int note_index) {
    for (int i = 0; i < settings.voices; i++) {
        if (m_lru[i] == note_index) {
            return i;
        }
    }
    return -1;
}

/**
 * Shifts the LRU values to the left from a given (array) index.
*/
void Para::m_lru_shift_left(int from_index) {
    for (int i = 1; i < settings.voices; i++) {
        // [7, 8, 9, 10]
        // i: 1 -> [8, 8, 9, 10]
        // i: 2 -> [8, 9, 9, 10]
        // i: 3 -> [8, 9, 10, 10]
        if (i >= from_index) {
            m_lru[i - 1] = m_lru[i];
        }
    }
}

// -----------------------------------------------------------------------------

/**
 * Shifts the LRU values to the right
 * Unused, just kept it if I need to use it later
*/
void Para::m_lru_shift_right() {
    for (int i = settings.voices - 1; i >= 0; i--) {
        // [7, 8, 9, 10]
        // i: 3 -> [7, 8, 9, 9]
        // i: 2 -> [7, 8, 8, 9]
        // i: 1 -> [7, 7, 8, 9]
        // i: 0 -> condition is false
        if (i > 0) {
            m_lru[i] = m_lru[i - 1];
        }
    }
}