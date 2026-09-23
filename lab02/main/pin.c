#include <stdint.h>
#include <stdbool.h>
#include "pin.h"
#include "soc/soc.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_periph.h"

//Reg Access Macro
#define REG(r) (*((volatile uint32_t *)(r)))

/* This Function resets the pin to 0*/
