#ifndef PIN_H_
#define PIN_H_

#include <stdint.h>
#include <stdbool.h>

typedef int8_t pin_num_t;

//Functions
int32_t pin_reset(pin_num_t pin);
int32_t pin_pullup(pin_num_t pin, bool enable);
int32_t pin_input(pin_num_t pin, bool enable);
int32_t pin_output(pin_num_t pin, bool enable);
int32_t pin_set_level(pin_num_t pin, uint32_t level);
int32_t pin_get_level(pin_num_t pin);
uint64_t pin_get_in_reg(void);
uint64_t pin_get_out_reg(void);

#endif