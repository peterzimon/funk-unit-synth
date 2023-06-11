/**
 * Interface class for different modes (mono, para etc.)
 * @TODO: this whole shabang
*/

#ifndef _I_CONVERTER
#define _I_CONVERTER

#include <inttypes.h>
#include "settings.h"

#define MAX_PITCH_BEND      0x3fff
#define PITCH_BEND_CENTER   0x2000
#define BASE_NOTE           440.0f

class IConverter {
    public:
        IConverter();

        virtual void reset(void) { }
        virtual void note_off(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void note_on(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void mod_wheel(uint8_t channel, uint8_t value) { }
        virtual float get_freq(uint8_t voice) { return 0; }
        virtual bool get_gate() { return false; }

        float frequency_from_midi_note(int note);
        void update_pitch_bend(uint16_t bend);

    private:

};

#endif