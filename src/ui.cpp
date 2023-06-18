#include "ui.h"

void UI::init() {
    // Binary MUX GPIO setup
    gpio_init(MUX_BINARY_PIN_A);
    gpio_init(MUX_BINARY_PIN_B);
    gpio_init(MUX_BINARY_PIN_C);
    gpio_init(MUX_BINARY_INPUT);

    gpio_set_dir(MUX_BINARY_PIN_A, GPIO_OUT);
    gpio_set_dir(MUX_BINARY_PIN_B, GPIO_OUT);
    gpio_set_dir(MUX_BINARY_PIN_C, GPIO_OUT);
    gpio_set_dir(MUX_BINARY_INPUT, GPIO_IN);

    gpio_pull_down(MUX_BINARY_INPUT);

    reset();
}

void UI::reset() {
    m_mux_step = 0;
    for (int i = 0; i < NO_OF_SWITCHES; i++) {
        switches[static_cast<mux_switch>(i)] = false;
    }
}

void UI::scan() {
    gpio_put(MUX_BINARY_PIN_A, m_mux_step & (1 << 0));
    gpio_put(MUX_BINARY_PIN_B, m_mux_step & (1 << 1));
    gpio_put(MUX_BINARY_PIN_C, m_mux_step & (1 << 2));

    switches[static_cast<mux_switch>(m_mux_step)] = gpio_get(MUX_BINARY_INPUT);

    m_mux_step++;
    m_mux_step &= 0x7; // Reset to 0 after 8 steps
}