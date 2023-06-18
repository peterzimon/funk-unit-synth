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

    gpio_pull_up(MUX_BINARY_INPUT);

    reset();
}

void UI::reset() {
    m_mux_step = 0;
    for (int i = 0; i < NO_OF_SWITCHES; i++) {
        switches[static_cast<mux_switch>(i)] = false;
    }

    gpio_put(MUX_BINARY_PIN_A, 0);
    gpio_put(MUX_BINARY_PIN_B, 0);
    gpio_put(MUX_BINARY_PIN_C, 0);

    bool value = gpio_get(MUX_BINARY_INPUT);
}

void UI::scan() {
    if (m_scan_cycle < SCAN_CYCLE) {
        m_scan_cycle++;
        return;
    }

    bool value = !gpio_get(MUX_BINARY_INPUT);
    mux_switch current_switch = static_cast<mux_switch>(m_mux_step);
    switches[current_switch] = value;

    m_mux_step++;

    gpio_put(MUX_BINARY_PIN_A, m_mux_step & (1 << 0));
    gpio_put(MUX_BINARY_PIN_B, m_mux_step & (1 << 1));
    gpio_put(MUX_BINARY_PIN_C, m_mux_step & (1 << 2));

    m_mux_step &= 0x7; // Reset to 0 after 8 steps

    m_scan_cycle = 0;
    // debug();
}

void UI::debug() {
    printf("Binary inputs:\n");
    for (int i = 0; i < NO_OF_SWITCHES; i++) {
        printf("%d: %d\n", i, (int)switches[static_cast<mux_switch>(i)]);
    }
    printf("\n");
}