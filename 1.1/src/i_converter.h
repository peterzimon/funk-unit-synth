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

// const uint16_t DIV_COUNTER = 1250;

class IConverter {
    public:
        IConverter();

        virtual void reset(void) { }
        virtual void note_off(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void note_on(uint8_t channel, uint8_t note, uint8_t velocity) { }
        virtual void mod_wheel(uint8_t channel, uint8_t value) { }
        virtual float get_freq(uint8_t voice) { return 0; }
        virtual bool get_gate() { return false; }

        // TODO: implement this. Should return true if it should update the output
        virtual bool is_dirty() { return m_dirty; }
        virtual void set_dirty(bool dirty) { m_dirty = dirty; }

        virtual void set_main_velocity(uint8_t main_velocity) { m_main_velocity = main_velocity; }
        virtual uint8_t get_main_velocity() { return m_main_velocity; }

        float frequency_from_midi_note(int note);
        uint16_t amp_for_frequency(float freq);
        void update_pitch_bend(uint16_t bend);


    private:
        bool m_dirty = false;
        uint8_t m_main_velocity = 0;
};

#endif