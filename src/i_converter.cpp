#include "i_converter.h"
#include <utils.h>
#include <math.h>

/**
 * Constructor
*/
IConverter::IConverter() {
    // Empty
}

/**
 * TBD
*/
void IConverter::update_pitch_bend(uint16_t bend) {

}

float IConverter::frequency_from_midi_note(int note) {
    return pow(2, (note - 69) / 12.0f) * BASE_NOTE;
}