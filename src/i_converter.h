/**
 * Interface class for different modes (mono, para, poly etc.)
 * 
 * Calibration
 * -----------
 * Since all electrical components have slightly different characteristics, the 
 * final output voltage may differ from voice to voice (CV+Gate pairs). Follow the
 * process below to calibrate each voice's CV output:
 * 
 * 0. According to the V/octave standard, all C's should output a rounded voltage
 *    value. C1 (ie. the 2nd octave C, MIDI note 12) should generate 1V, C2 should
 *    2V and so on. Midipoly can output 10 octaves so the first 10 octave on your
 *    MIDI keyboard should output 1, 2 ... 10 volts respectively. 
 * 1. Set Midipoly to Unison mode and make sure the mod wheel is turned all the way
 *    down. This way all the CV outputs will be the same. Use a precision multimeter
 *    and measure the output voltage of CV A. Note that when you don't press a key
 *    the output voltage is not precisely 0V due to imperfection and leakage current. 
 *    This is something you can't calibrate.
 * 2. Press C1 on your MIDI keyboard. You should read around 1V on the multimeter.
 *    If it's higher than 1V then you'll need to set an adjustment value for the 
 *    1st voice's 1st octave C. In order to do this, open cvgate.cpp and set the
 *    _cal[0][1] to some negative value. Setting it to -1 will case a cca. 2mV
 *    drop in the output. So _cal[0][1] sets the first voice ("0" -> first voice)
 *    first octave ("1" that's supposed to be 1V).
 * 3. Upload the firmware and check the output voltage again. If it's 1-2mV close 
 *    then you can go to the next octave. Do this until all the octaves (0-10) 
 *    show the appropriate voltage on the given voice.
 * 4. If a voice is done, you can move on to the next voice. Calibrate all 4 voices 
 *    and you're done.
*/

#ifndef _I_CONVERTER
#define _I_CONVERTER

#include <inttypes.h>
#include "settings.h"

#define MAX_PITCH_BEND      0x3fff
#define PITCH_BEND_CENTER   0x2000

class IConverter {
    public:
        IConverter();

        virtual void reset(void) { }
        virtual void note_off(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void note_on(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void mod_wheel(uint8_t channel, uint8_t value) { }
        virtual void get_cv_gate(uint16_t *cv, int *gate) { }
        void update_pitch_bend(uint16_t bend);
        uint16_t cv_for_note(uint8_t note, int voice);

    private:
        int8_t m_cal[MAX_VOICES][OCTAVES + 1]; // Calibration data
        int16_t m_pitch_bend_cv;
};

#endif