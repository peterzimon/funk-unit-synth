#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <math.h>
#include <utils.h>
#include <map>
#include <pico/stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "settings.h"

// The UI doesn't have to be scanned in every processor cycle. To save processor
// time define the number cycles for which the UI should not be scanned.
#define SCAN_CYCLE 1000

class UI {
public:
    static UI& get_instance() {
        static UI instance;
        return instance;
    }

    DISALLOW_COPY_AND_ASSIGN(UI);

    std::map<mux_switch, bool> switches;

    void init();
    void reset();
    void scan();

protected:
    UI() = default;

private:
    uint16_t m_mux_step = 0;
    int m_scan_cycle = 0;

    void debug();
};

#endif