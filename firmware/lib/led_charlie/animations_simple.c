#include <ch32v00x.h>
#include <stdlib.h>
#include "animations_simple.h"

#define TWINKLE_NUM_FRAMES      10
#define TWINKLE_BITMASK_SIZE    2       // For 42 LEDs

static const uint32_t twinkle_frames[TWINKLE_NUM_FRAMES][TWINKLE_BITMASK_SIZE] = {
    // Frame 0: LEDs 0, 5, 12, 18, 25, 31, 37, 40
    {0x84041021, 0x00000121},
    
    // Frame 1: LEDs 2, 7, 11, 16, 22, 28, 35, 41
    {0x10410084, 0x00000288},
    
    // Frame 2: LEDs 1, 8, 14, 19, 24, 30, 36, 39
    {0x41088102, 0x00000190},
    
    // Frame 3: LEDs 3, 9, 15, 20, 26, 32, 38
    {0x04109208, 0x00000141},
    
    // Frame 4: LEDs 4, 10, 13, 21, 27, 33, 39, 41
    {0x08202414, 0x00000282},
    
    // Frame 5: LEDs 6, 11, 17, 23, 29, 34, 40
    {0x20820840, 0x00000104},
    
    // Frame 6: LEDs 1, 7, 14, 20, 26, 32, 37, 41
    {0x04104082, 0x00000222},
    
    // Frame 7: LEDs 3, 9, 16, 22, 28, 35, 38
    {0x10410208, 0x00000148},
    
    // Frame 8: LEDs 5, 12, 18, 24, 30, 36, 39
    {0x41041020, 0x00000190},
    
    // Frame 9: LEDs 2, 8, 15, 21, 27, 33, 40
    {0x08208104, 0x00000102}
};

static uint8_t current_frame = 0;

void twinkle_init(void){
    srand(TIM2->CNT);
    
    current_frame = rand() % TWINKLE_NUM_FRAMES;
}

uint32_t* twinkle_next_frame(void){
    current_frame = rand() % TWINKLE_NUM_FRAMES;
    
    /* Return pointer to that frame's pattern */
    return (uint32_t*)twinkle_frames[current_frame];
}