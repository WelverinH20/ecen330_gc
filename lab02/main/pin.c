#include "pin.h"

// Hardware Register Macro
#define REG(r) (*((volatile uint32_t *)(uintptr_t)(r)))

// ESP32 Hardware Register Base Addresses
#define DR_REG_GPIO_BASE        0x3FF44000
#define DR_REG_IO_MUX_BASE      0x3FF49000

// GPIO Register Map
#define GPIO_OUT_REG            (DR_REG_GPIO_BASE + 0x0004)
#define GPIO_OUT_W1TS_REG       (DR_REG_GPIO_BASE + 0x0008)
#define GPIO_OUT_W1TC_REG       (DR_REG_GPIO_BASE + 0x000C)

#define GPIO_OUT1_REG           (DR_REG_GPIO_BASE + 0x0010)
#define GPIO_OUT1_W1TS_REG      (DR_REG_GPIO_BASE + 0x0014)
#define GPIO_OUT1_W1TC_REG      (DR_REG_GPIO_BASE + 0x0018)

#define GPIO_ENABLE_W1TS_REG    (DR_REG_GPIO_BASE + 0x0024)
#define GPIO_ENABLE_W1TC_REG    (DR_REG_GPIO_BASE + 0x0028)

#define GPIO_ENABLE1_W1TS_REG   (DR_REG_GPIO_BASE + 0x0030)
#define GPIO_ENABLE1_W1TC_REG   (DR_REG_GPIO_BASE + 0x0034)

#define GPIO_IN_REG             (DR_REG_GPIO_BASE + 0x003C)
#define GPIO_IN1_REG            (DR_REG_GPIO_BASE + 0x0040)

#define GPIO_PIN0_REG           (DR_REG_GPIO_BASE + 0x0088)
#define GPIO_FUNC0_OUT_SEL_CFG  (DR_REG_GPIO_BASE + 0x0530)

// IO_MUX Register Mapping Table for ESP32 GPIOs 0 to 39
static const uint16_t io_mux_offsets[40] = {
    0x44, 0x88, 0x40, 0x84, 0x48, 0x6C, 0x60, 0x64, // GPIO 0-7
    0x68, 0x68, 0x6C, 0x70, 0x34, 0x38, 0x3C, 0x30, // GPIO 8-15
    0x4C, 0x50, 0x74, 0x78, 0x7C, 0x80, 0x8C, 0x90, // GPIO 16-23
    0x00, 0x24, 0x28, 0x2C, 0x00, 0x00, 0x00, 0x00, // GPIO 24-31
    0x1C, 0x20, 0x14, 0x18, 0x04, 0x08, 0x0C, 0x10  // GPIO 32-39
};

// Validate valid ESP32 GPIO pins (0-39, excluding flash pins 6-11)
static inline bool is_valid_pin(pin_num_t pin) {
    return (pin >= 0 && pin < 40 && !(pin >= 6 && pin <= 11));
}

int32_t pin_reset(pin_num_t pin) {
    if (!is_valid_pin(pin)) return -1;

    // 1. Reset pin configuration register to 0
    REG(GPIO_PIN0_REG + (pin * 4)) = 0;

    // 2. Map output pin function to simple GPIO output mode (0x100)
    REG(GPIO_FUNC0_OUT_SEL_CFG + (pin * 4)) = 0x100;

    // 3. Reset IO_MUX register to default state (MCU_SEL=2, FUN_DRV=2) -> Base: 0x2800
    if (io_mux_offsets[pin] != 0) {
        REG(DR_REG_IO_MUX_BASE + io_mux_offsets[pin]) = 0x2800;
    }

    // 4. Disable output driver
    pin_output(pin, false);

    // 5. Disable internal pull-up
    pin_pullup(pin, false);

    // 6. Reset output level register to 0
    pin_set_level(pin, 0);

    return 0;
}

int32_t pin_pullup(pin_num_t pin, bool enable) {
    if (!is_valid_pin(pin) || pin >= 34) return -1; // Pins 34-39 are input-only (no pull-ups)

    uint32_t mux_addr = DR_REG_IO_MUX_BASE + io_mux_offsets[pin];
    if (mux_addr == DR_REG_IO_MUX_BASE) return -1;

    if (enable) {
        REG(mux_addr) |= (1U << 8);  // Set Bit 8 (FUN_WPU - Pull-Up Enable)
    } else {
        REG(mux_addr) &= ~(1U << 8); // Clear Bit 8
    }

    return 0;
}

int32_t pin_input(pin_num_t pin, bool enable) {
    if (!is_valid_pin(pin)) return -1;

    uint32_t mux_addr = DR_REG_IO_MUX_BASE + io_mux_offsets[pin];
    if (mux_addr == DR_REG_IO_MUX_BASE) return -1;

    if (enable) {
        REG(mux_addr) |= (1U << 9);  // Set Bit 9 (FUN_IE - Input Enable)
    } else {
        REG(mux_addr) &= ~(1U << 9); // Clear Bit 9
    }

    return 0;
}

int32_t pin_output(pin_num_t pin, bool enable) {
    if (!is_valid_pin(pin) || pin >= 34) return -1; // Pins 34-39 cannot act as outputs

    if (pin < 32) {
        if (enable) {
            REG(GPIO_ENABLE_W1TS_REG) = (1U << pin);
        } else {
            REG(GPIO_ENABLE_W1TC_REG) = (1U << pin);
        }
    } else {
        uint32_t bit = pin - 32;
        if (enable) {
            REG(GPIO_ENABLE1_W1TS_REG) = (1U << bit);
        } else {
            REG(GPIO_ENABLE1_W1TC_REG) = (1U << bit);
        }
    }

    return 0;
}

int32_t pin_set_level(pin_num_t pin, uint32_t level) {
    if (!is_valid_pin(pin) || pin >= 34) return -1;

    if (pin < 32) {
        if (level) {
            REG(GPIO_OUT_W1TS_REG) = (1U << pin);
        } else {
            REG(GPIO_OUT_W1TC_REG) = (1U << pin);
        }
    } else {
        uint32_t bit = pin - 32;
        if (level) {
            REG(GPIO_OUT1_W1TS_REG) = (1U << bit);
        } else {
            REG(GPIO_OUT1_W1TC_REG) = (1U << bit);
        }
    }

    return 0;
}

int32_t pin_get_level(pin_num_t pin) {
    if (!is_valid_pin(pin)) return -1;

    if (pin < 32) {
        return (REG(GPIO_IN_REG) >> pin) & 1U;
    } else {
        return (REG(GPIO_IN1_REG) >> (pin - 32)) & 1U;
    }
}

uint64_t pin_get_in_reg(void) {
    uint64_t low = REG(GPIO_IN_REG);
    uint64_t high = REG(GPIO_IN1_REG) & 0xFFULL;
    return low | (high << 32);
}

uint64_t pin_get_out_reg(void) {
    uint64_t low = REG(GPIO_OUT_REG);
    uint64_t high = REG(GPIO_OUT1_REG) & 0xFFULL;
    return low | (high << 32);
}