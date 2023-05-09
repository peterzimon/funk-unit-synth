#include "para_time_based.h"
#include "../settings.h"

/**
 * Resets everything
*/
void ParaTimeBased::reset() {
    m_no_of_distinct_notes = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
        m_notes[i] = m_distinct_notes[i] = -1;
        m_voice_millis[i] = 0;
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
void ParaTimeBased::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {

    // Return if note is already playing
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == (int)note) return;
    }

    // If the note is not playing find a voice for the new note
    int new_note_index = m_find_voice();
    m_voice_millis[new_note_index] = Utils::millis();
    m_notes[new_note_index] = (int)note;
    
    m_distinct_notes[m_no_of_distinct_notes] = (int)note;
    m_no_of_distinct_notes++;

    m_update_voices();
}

/**
 * Handling NOTE OFF event. MIDI events are called in the MidiHandler class.
 * 1. Reset all voices to 0 where note matches releasedd note
 * 2. Updated unused voices similarly as in NOTE ON
*/
void ParaTimeBased::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    for (int i = 0; i < settings.voices; i++) {
        if (m_notes[i] == (int)note) {
            m_notes[i] = -1;
            m_voice_millis[i] = 0;
        }
    }
    
    m_update_voices();
}

/**
 * Return whatever is needed for DCO. I guess it's the frequency and the
 * amplitude value. This is called in midi handler.
*/
void ParaTimeBased::get_freq_amp() {

}


/** ----------------------------------------------------------------------------
 * PRIVATE
*/

/**
 * Finds the next voice for most recently played note
*/
int ParaTimeBased::m_find_voice() {
    uint32_t oldest_millis = Utils::millis();
    int oldest_voice = 0;

    for (int i = 0; i < settings.voices; i++) {
        
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
 * two note are held but there are 4 voices then each note would be played by
 * two voices. If only 1 note is held then the same note would be played by all 
 * voices (fallback to unison mode).
*/
void ParaTimeBased::m_update_voices() {
    
    // Collect distinct notes
    int distinct_notes[MAX_VOICES];
    int no_of_distinct_notes = 0;
    for (int i = 0; i < settings.voices; i++) {
        if (m_voice_millis[i] != 0) {
            distinct_notes[no_of_distinct_notes] = m_notes[i];
            no_of_distinct_notes++;
        }
    }
    
    // Distribute distinct notes amongst unused voices
    int next_distinct_note_index = 0;
    for (int i = 0; i < settings.voices; i++) {
        if (m_voice_millis[i] == 0) {
            m_notes[i] = distinct_notes[next_distinct_note_index];
            next_distinct_note_index++;
            if (next_distinct_note_index >= no_of_distinct_notes) next_distinct_note_index = 0;
        }
    }
}