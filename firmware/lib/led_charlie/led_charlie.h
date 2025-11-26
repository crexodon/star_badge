#ifndef LED_CHARLIE_H
#define LED_CHARLIE_H
#include <stdint.h>

void charlie_init(void);
void charlie_off(void);
void charlie_test(void);
void charlie_set_led(uint8_t led_num, uint8_t state);
void charlie_set_brightness(uint8_t);
void charlie_single(uint8_t led_num, uint8_t state);
uint8_t charlie_get_brightness(void);

void charlie_enable_multiplex(uint32_t *bitmask);
void charlie_update_multiplex_pattern(uint32_t *bitmask);
void charlie_set_fast_pwm_mode(uint8_t enable);

#endif /* LED_CHARLIE_H */