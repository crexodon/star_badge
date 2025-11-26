#ifndef ANIMATIONS_H
#define ANIMATIONS_H
#include <stdint.h>

void anim_sparkle_init(uint8_t num_leds_on, uint8_t speed);
uint32_t* anim_sparkle_update(void);

#endif /* ANIMATIONS_H */