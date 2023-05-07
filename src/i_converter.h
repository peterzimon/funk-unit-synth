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

class IConverter {
    public:
        IConverter();

        virtual void reset(void) { }
        virtual void note_off(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void note_on(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void mod_wheel(uint8_t channel, uint8_t value) { }
        virtual void get_freq_amp() { }
        
        void update_pitch_bend(uint16_t bend);
        uint16_t freq_for_note();
        uint16_t amp_for_note();

    private:
};

#endif