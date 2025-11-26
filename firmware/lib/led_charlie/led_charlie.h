#ifndef LED_CHARLIE_H
#define LED_CHARLIE_H
#include <stdint.h>

void charlie_init(void);
void charlie_off(void);
void charlie_test(void);
void charlie_single(uint8_t led_num, uint8_t state);
void charlie_set_brightness(uint8_t);
uint8_t charlie_get_brightness(void);

#endif /* LED_CHARLIE_H */