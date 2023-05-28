#include "para.h"

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
 * There are two modes depending on the value of (bool) PARA_STACK_VOICES. If 
 * it's `true` then on the first note all the voices will play the same note and
 * all subsequent notes will be distribute evenly amongst the voices.
 * 
 * If it's `false` then voices will take notes as they are played, if a single
 * note is played then just a single voice is on, all others are off. If two
 * notes, then two and so on. This is possible because all the DCOs have an amp
 * compensation and it's possible to shut them down without dedicated VCAs.
 * 
 * In both cases all the played notes are kept even if the keys are released,
 * which is needed to be able to ring them if the (ADSR) release is not zero. 
 * Ie. if we shut them all down on key up then there wouldn't be any note played
 * during the release phase.
 * 
 * Here's an example how it works with PARA_STACK_VOICES = false
 * 
 * HOLD FIRST NOTE
 * notes/voices = {note1, 0, 0, 0, 0, 0}
 * 
 * HOLD SECOND NOTE
 * notes/voices = {note1, note2, 0, 0, 0, 0}
 * 
 * HOLD THIRD NOTE
 * notes/voices = {note1, note2, note3, 0, 0, 0}
 * 
 * RELEASE SECOND NOTE
 * notes/voices = {note1, note2, note3, 0, 0, 0} -> YES, keep all notes playing!
 * 
 * RELEASE THIRD NOTE
 * notes/voices = {note1, note2, note3, 0, 0, 0} -> YES, keep all notes playing!
 * 
 * RELEASE ALL NOTES
 * notes/voices = {note1, note2, note3, 0, 0, 0} -> YES, keep all notes playing 
 *                                                  so that it keeps ringing!
 * 
 * HOLD FIRST NOTE
 * notes/voices = {note1, 0, 0, 0, 0, 0} -> reset all notes except 1st
 * 
 * With PARA_STACK_VOICES = true the difference is that instead of 0's the
 * played notes are equally distributed.
 * 
 * ---
 * 
 * Additionally there's another submode of paraphonic mode depeneding on the 
 * value of 'settings.solo'. When it's on, then instead of keep playing the 
 * notes on releasing non-first notes, it'll shut off. This is useful when 
 * playing solos so there's no stuck notes, however this also means that only
 * the last note will be kept and thus played through to the envelope.
 * 
 * ---
 * 
 * For this though 6 voices may be too many! Maybe 5 voices would be better
 * because if a wrong note is held it's not removed from the played notes until
 * a new note claims its place. Needs to be tested!
 */
void Para::note_on(uint8_t channel, uint8_t note, uint8_t velocity) {

    if (m_reset) {
        if (PARA_STACK_VOICES) {
            for (int i = 0; i < VOICES; i++) {
                m_notes[i] = (int)note;
            }
        } else {
            m_notes[0] = (int)note;
            for (int i = 1; i < VOICES; i++) {
                m_notes[i] = -1;
            }
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

    if (PARA_STACK_VOICES) {
        m_distribute_notes();
    }

    m_debug();
}

/**
 * Handling NOTE OFF event. MIDI events are called in the MidiHandler class.
*/
void Para::note_off(uint8_t channel, uint8_t note, uint8_t velocity) {
    for (int i = 0; i < VOICES; i++) {
        if (m_notes[i] == (int)note) {
            m_voice_millis[i] = 0;
            if (settings.solo && i != 0) {
                m_notes[i] = -1;
            }
        }
    }

    m_reset = true;
    for (int i = 0; i < VOICES; i++) {
        if (m_voice_millis[i] != 0) {
            m_reset = false;
            break;
        }
    }

    m_debug();
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
void Para::m_distribute_notes() {
    
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