#include "ui.h"

void UI::init() {

    // Hardware
    gpio_init(GP_VOICES_SWITCH);
    gpio_set_dir(GP_VOICES_SWITCH, GPIO_IN);
    gpio_pull_down(GP_VOICES_SWITCH);

    // Software
    m_scan_counter = SCAN_CYCLE; // Make sure the UI is scanned at start
}

void UI::update() {
    if (m_scan_counter == SCAN_CYCLE) {

        // Set number of voices
        if (gpio_get(GP_VOICES_SWITCH)) {
            settings.voices = 4;
        } else {
            settings.voices = 3;
        }

        m_scan_counter = 0;
    }
    m_scan_counter++;
}