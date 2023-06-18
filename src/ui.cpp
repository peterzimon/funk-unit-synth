#include "ui.h"

void UI::init() {
    adc_init();
    adc_gpio_init(ADC_RING_LEN_PIN);
    adc_gpio_init(ADC_SYNTH_MODE_PIN);
    adc_select_input(ADC_RING_LEN_CHANNEL);

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

    m_mux_step = 0;
}

void UI::init_scan() {
    for (int i = 0; i < NO_OF_SWITCHES; i++) {
        gpio_put(MUX_BINARY_PIN_A, i & (1 << 0));
        gpio_put(MUX_BINARY_PIN_B, i & (1 << 1));
        gpio_put(MUX_BINARY_PIN_C, i & (1 << 2));
        bool sw_read = !gpio_get(MUX_BINARY_INPUT);
        switches[static_cast<mux_switch>(i)] = false;
    }

    adc_select_input(ADC_RING_LEN_CHANNEL);
    release_long = adc_read();

    adc_select_input(ADC_SYNTH_MODE_CHANNEL);
    release_long = adc_read();
}

void UI::scan() {

    // Read switches (muxed)
    if (m_scan_cycle < SCAN_CYCLE) {
        m_scan_cycle++;
        updated = false;
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

    // Read ADC
    adc_select_input(ADC_RING_LEN_CHANNEL);
    release_long = Utils::map(adc_read(), 0, 4096, RELEASE_LONG_MIN, RELEASE_LONG_MAX);

    adc_select_input(ADC_SYNTH_MODE_CHANNEL);
    synth_mode = static_cast<device_mode>(Utils::map(adc_read(), 0, 4096, 0, NO_OF_MODES));

    updated = true;
    debug();
}

void UI::debug() {
    printf("Binary inputs:\n");
    for (int i = 0; i < NO_OF_SWITCHES; i++) {
        printf("%d: %d\n", i, (int)switches[static_cast<mux_switch>(i)]);
    }
    printf("Ring length: %lu\n", release_long);
    printf("Synth mode: %d\n", synth_mode);
    printf("\n");
}