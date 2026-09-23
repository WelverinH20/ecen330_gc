#include <stdio.h>
#include "soc/reg_base.h" // DR_REG_GPIO_BASE, DR_REG_IO_MUX_BASE
#include "driver/rtc_io.h" // rtc_gpio_*
#include "pin.h"

// GPIO Matrix Registers
#define GPIO_OUT_REG          (DR_REG_GPIO_BASE + 0x04)
#define GPIO_OUT_W1TS_REG     (DR_REG_GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC_REG     (DR_REG_GPIO_BASE + 0x0C)
#define GPIO_OUT1_REG         (DR_REG_GPIO_BASE + 0x10)
#define GPIO_OUT1_W1TS_REG    (DR_REG_GPIO_BASE + 0x14)
#define GPIO_OUT1_W1TC_REG    (DR_REG_GPIO_BASE + 0x18)

#define GPIO_ENABLE_REG       (DR_REG_GPIO_BASE + 0x20)
#define GPIO_ENABLE_W1TS_REG  (DR_REG_GPIO_BASE + 0x24)
#define GPIO_ENABLE_W1TC_REG  (DR_REG_GPIO_BASE + 0x28)
#define GPIO_ENABLE1_REG      (DR_REG_GPIO_BASE + 0x2C)
#define GPIO_ENABLE1_W1TS_REG (DR_REG_GPIO_BASE + 0x30)
#define GPIO_ENABLE1_W1TC_REG (DR_REG_GPIO_BASE + 0x34)

#define GPIO_IN_REG           (DR_REG_GPIO_BASE + 0x3C)
#define GPIO_IN1_REG          (DR_REG_GPIO_BASE + 0x40)

#define GPIO_PIN0_REG         (DR_REG_GPIO_BASE + 0x88)
#define GPIO_FUNC0_OUT_SEL_CFG_REG (DR_REG_GPIO_BASE + 0x530)

// Gives byte offset of IO_MUX Configuration Register from DR_REG_IO_MUX_BASE
static const uint8_t PIN_MUX_REG_OFFSET[] = {
    0x44, 0x88, 0x40, 0x84, 0x48, 0x6c, 0x60, 0x64, // pin  0- 7
    0x68, 0x54, 0x58, 0x5c, 0x34, 0x38, 0x30, 0x3c, // pin  8-15
    0x4c, 0x50, 0x70, 0x74, 0x78, 0x7c, 0x80, 0x8c, // pin 16-23
    0x90, 0x24, 0x28, 0x2c, 0xFF, 0xFF, 0xFF, 0xFF, // pin 24-31
    0x1c, 0x20, 0x14, 0x18, 0x04, 0x08, 0x0c, 0x10, // pin 32-39
};

// IO MUX Register Macro
#define IO_MUX_REG(n)         (DR_REG_IO_MUX_BASE + PIN_MUX_REG_OFFSET[n])

// IO MUX Register Fields & Shift Bits
#define FUN_WPD               7
#define FUN_WPU               8
#define FUN_IE                9
#define FUN_DRV_S             10
#define FUN_DRV_M             (0x3 << FUN_DRV_S)
#define MCU_SEL_S             12
#define MCU_SEL_M             (0x7 << MCU_SEL_S)
#define GPIO_PIN_PAD_DRIVER   2

#define REG(r) (*(volatile uint32_t *)(r))
#define REG_BITS 32

// Register Bit Manipulation Macros
#define REG_SET_BIT(r,b)      (REG(r) |= (1U << (b)))
#define REG_CLR_BIT(r,b)      (REG(r) &= ~(1U << (b)))
#define REG_GET_BIT(r,b)      ((REG(r) >> (b)) & 1U)

int32_t pin_reset(pin_num_t pin)
{
    if (pin >= 40 || PIN_MUX_REG_OFFSET[pin] == 0xFF) return -1;

    if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
        rtc_gpio_deinit(pin);
        rtc_gpio_pullup_en(pin);
        rtc_gpio_pulldown_dis(pin);
    }

    REG(GPIO_PIN0_REG + (pin * 4)) = 0;
    REG(GPIO_FUNC0_OUT_SEL_CFG_REG + (pin * 4)) = 0x100;

    uint32_t io_mux_val = REG(IO_MUX_REG(pin));
    io_mux_val &= ~(MCU_SEL_M | FUN_DRV_M);
    io_mux_val |= (2 << MCU_SEL_S) | (2 << FUN_DRV_S) | (1U << FUN_WPU);
    REG(IO_MUX_REG(pin)) = io_mux_val;

    return pin_set_level(pin, 0);
}

int32_t pin_pullup(pin_num_t pin, bool enable)
{
    if (pin >= 40 || PIN_MUX_REG_OFFSET[pin] == 0xFF) return -1;

    if (rtc_gpio_is_valid_gpio(pin)) {
        if (enable) return rtc_gpio_pullup_en(pin);
        else return rtc_gpio_pullup_dis(pin);
    }

    if (enable) REG_SET_BIT(IO_MUX_REG(pin), FUN_WPU);
    else        REG_CLR_BIT(IO_MUX_REG(pin), FUN_WPU);
    return 0;
}

int32_t pin_pulldown(pin_num_t pin, bool enable)
{
    if (pin >= 40 || PIN_MUX_REG_OFFSET[pin] == 0xFF) return -1;

    if (rtc_gpio_is_valid_gpio(pin)) {
        if (enable) return rtc_gpio_pulldown_en(pin);
        else return rtc_gpio_pulldown_dis(pin);
    }

    if (enable) REG_SET_BIT(IO_MUX_REG(pin), FUN_WPD);
    else        REG_CLR_BIT(IO_MUX_REG(pin), FUN_WPD);
    return 0;
}

int32_t pin_input(pin_num_t pin, bool enable)
{
    if (pin >= 40 || PIN_MUX_REG_OFFSET[pin] == 0xFF) return -1;

    if (enable) REG_SET_BIT(IO_MUX_REG(pin), FUN_IE);
    else        REG_CLR_BIT(IO_MUX_REG(pin), FUN_IE);
    return 0;
}

int32_t pin_output(pin_num_t pin, bool enable)
{
    if (pin >= 40) return -1;

    if (pin < REG_BITS) {
        if (enable) REG(GPIO_ENABLE_W1TS_REG) = (1U << pin);
        else        REG(GPIO_ENABLE_W1TC_REG) = (1U << pin);
    } else {
        if (enable) REG(GPIO_ENABLE1_W1TS_REG) = (1U << (pin - REG_BITS));
        else        REG(GPIO_ENABLE1_W1TC_REG) = (1U << (pin - REG_BITS));
    }
    return 0;
}

int32_t pin_odrain(pin_num_t pin, bool enable)
{
    if (pin >= 40) return -1;

    uint32_t pin_reg = GPIO_PIN0_REG + (pin * 4);
    if (enable) REG_SET_BIT(pin_reg, GPIO_PIN_PAD_DRIVER);
    else        REG_CLR_BIT(pin_reg, GPIO_PIN_PAD_DRIVER);
    return 0;
}

int32_t pin_set_level(pin_num_t pin, int32_t level)
{
    if (pin >= 40) return -1;

    if (pin < REG_BITS) {
        if (level) REG(GPIO_OUT_W1TS_REG) = (1U << pin);
        else       REG(GPIO_OUT_W1TC_REG) = (1U << pin);
    } else {
        if (level) REG(GPIO_OUT1_W1TS_REG) = (1U << (pin - REG_BITS));
        else       REG(GPIO_OUT1_W1TC_REG) = (1U << (pin - REG_BITS));
    }
    return 0;
}

int32_t pin_get_level(pin_num_t pin)
{
    if (pin >= 40) return -1;

    if (pin < REG_BITS) {
        return REG_GET_BIT(GPIO_IN_REG, pin);
    } else {
        return REG_GET_BIT(GPIO_IN1_REG, pin - REG_BITS);
    }
}

uint64_t pin_get_in_reg(void)
{
    uint64_t low = REG(GPIO_IN_REG);
    uint64_t high = REG(GPIO_IN1_REG);
    return low | (high << REG_BITS);
}

uint64_t pin_get_out_reg(void)
{
    uint64_t low = REG(GPIO_OUT_REG);
    uint64_t high = REG(GPIO_OUT1_REG);
    return low | (high << REG_BITS);
}