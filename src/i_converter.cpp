#include "i_converter.h"
#include <utils.h>
#include <math.h>

/**
 * Constructor
*/
IConverter::IConverter() {
    // Init calibration values
    for (int i = 0; i < settings.voices; i++) {
        for (int j = 0; j < OCTAVES; j++) {
            m_cal[i][j] = 0;
        }
    }
    
    /**
     * Set _cal calibration values here. How? Read the header comment in cvgate.h
     * 
     * _cal[VOICE][Cx] = RELATIVE ADJUSTMENT VALUE
     *        ^     ^              ^ 
     *        ^     ^              Use negative values to decrease the CV output
     *        ^     C note on given octave. Should output xV (C1 -> 1V, C2 -> 2V etc.) [0-10]
     *        CV output [0-3]
    */

    // Voice A
    m_cal[0][1] = 0;
    m_cal[0][2] = 0;
    m_cal[0][3] = 0;
    m_cal[0][4] = 0;
    m_cal[0][5] = 0;
    m_cal[0][6] = 0;
    m_cal[0][7] = 0;
    m_cal[0][8] = 0;
    m_cal[0][8] = 0;
    m_cal[0][9] = 0;
    m_cal[0][10] = 0;

    // Voice B
    m_cal[1][1] = 0;
    m_cal[1][2] = 0;
    m_cal[1][3] = 0;
    m_cal[1][4] = 0;
    m_cal[1][5] = 0;
    m_cal[1][6] = 0;
    m_cal[1][7] = 0;
    m_cal[1][8] = 0;
    m_cal[1][9] = 0;
    m_cal[1][10] = 0;

    // Voice C
    m_cal[2][1] = 0;
    m_cal[2][2] = 0;
    m_cal[2][3] = 0;
    m_cal[2][4] = 0;
    m_cal[2][5] = 0;
    m_cal[2][6] = 0;
    m_cal[2][7] = 0;
    m_cal[2][8] = 0;
    m_cal[2][9] = 0;
    m_cal[2][10] = 0;

    // Voice D
    m_cal[3][1] = 0;
    m_cal[3][2] = 0;
    m_cal[3][3] = 0;
    m_cal[3][4] = 0;
    m_cal[3][5] = 0;
    m_cal[3][6] = 0;
    m_cal[3][7] = 0;
    m_cal[3][8] = 0;
    m_cal[3][9] = 0;
    m_cal[3][10] = 0;

    m_pitch_bend_cv = 0;
}

/**
 * Returns the calibrated CV value for the given note.
 * The output of the DAC is amplified by the circuit which results in a pretty 
 * accurate amplification of 4095mV (max output voltage of DAC) to 10V. Since 
 * the device covers up to 10 octaves we have to map OCTAVES*12  notes 
 * to 0-4095.
*/
uint16_t IConverter::cv_for_note(uint8_t note, int voice) {
    uint8_t t_note = note;
    uint8_t notes = OCTAVES * 12;
    if (t_note > LOWEST_MIDI_NOTE + notes) {
        t_note = LOWEST_MIDI_NOTE + notes;
    }
    uint8_t octave = floor(t_note / 12) + 1;
    uint16_t volt_per_octave = (uint16_t) (MAX_NOTE_VOLTAGE / OCTAVES);
    uint16_t raw_cv_lo = volt_per_octave * (octave - 1);
    uint16_t raw_cv_hi = volt_per_octave * octave;

    uint16_t raw_cv = (uint16_t)Utils::map(t_note, LOWEST_MIDI_NOTE, (LOWEST_MIDI_NOTE + notes), 0, MAX_NOTE_VOLTAGE);
    uint16_t cv = (uint16_t)Utils::map(raw_cv, raw_cv_lo, raw_cv_hi, (raw_cv_lo + m_cal[voice][octave - 1]), (raw_cv_hi + m_cal[voice][octave]));

    return cv + m_pitch_bend_cv;
}

/**
 * Pitch bend value can be between 0 and 0x3fff with 0x2000 meaning no bend. 
 * Pitch bend CV calculation:
 * 1. shift bend value to -0x2000 and 0x2000
 * 2. get max bend CV value (2 semitones)
 * 3. calculate actual bend CV value with the bend vs. max bend ratio
*/
void IConverter::update_pitch_bend(uint16_t bend) {
    int16_t shifted_bend = bend - PITCH_BEND_CENTER;
    uint8_t max_bend_cv = (uint8_t) (MAX_NOTE_VOLTAGE / (OCTAVES * 12) * 2);
    m_pitch_bend_cv = shifted_bend * max_bend_cv / PITCH_BEND_CENTER; // BEND / MAX_BEND_VALUE[8192]] = BENDCV / MAX_BEND_CV
}