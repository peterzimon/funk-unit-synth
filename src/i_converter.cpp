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
 * Return frequency for a given MIDI note
*/
uint16_t IConverter::freq_for_note() {
    return 0;
}

/**
 * Return frequency for a given MIDI note
*/
uint16_t IConverter::amp_for_note() {
    return 0;
}

/**
 * TBD
*/
void IConverter::update_pitch_bend(uint16_t bend) {
    
}