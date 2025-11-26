#include <ch32v00x.h>
#include "animations.h"
#include <stdlib.h>  // For rand()

#define ANIM_NUM_LEDS       42      // Total number of LEDs in matrix
#define ANIM_BITMASK_SIZE   2       // (42 + 31) / 32 = 2

static uint32_t anim_pattern[ANIM_BITMASK_SIZE] = {0};
static uint32_t anim_frame_counter = 0;

// Helper Functions
static void anim_set_led(uint8_t led_num, uint8_t state){
    if (led_num >= ANIM_NUM_LEDS) return;
    
    uint8_t index = led_num / 32;
    uint8_t bit = led_num % 32;
    
    if (state) {
        anim_pattern[index] |= (1UL << bit);
    } else {
        anim_pattern[index] &= ~(1UL << bit);
    }
}

static uint8_t anim_get_led(uint8_t led_num){
    if (led_num >= ANIM_NUM_LEDS) return 0;
    
    uint8_t index = led_num / 32;
    uint8_t bit = led_num % 32;
    
    return (anim_pattern[index] & (1UL << bit)) != 0;
}

static void anim_clear_all(void){
    for (int i = 0; i < ANIM_BITMASK_SIZE; i++) {
        anim_pattern[i] = 0;
    }
}

static void anim_set_all(void){
    for (int i = 0; i < ANIM_BITMASK_SIZE; i++) {
        anim_pattern[i] = 0xFFFFFFFF;
    }
    
    /* Clear any bits beyond ANIM_MAX_LEDS */
    uint8_t remainder = ANIM_NUM_LEDS % 32;
    if (remainder > 0) {
        anim_pattern[ANIM_BITMASK_SIZE - 1] = (1UL << remainder) - 1;
    }
}

static uint8_t anim_count_leds_on(void){
    uint8_t count = 0;
    
    for (uint8_t i = 0; i < ANIM_NUM_LEDS; i++) {
        if (anim_get_led(i)) {
            count++;
        }
    }
    
    return count;
}

static uint8_t anim_random(uint8_t max){
    return (uint8_t)(rand() % max);
}

// Sparkle Animation
typedef struct {
    uint8_t target_count;      // Target number of LEDs to have on
    uint8_t change_probability; // Probability (0-255) of changing an LED each frame
    uint32_t last_update_frame;
} SparkleState;

static SparkleState sparkle_state = {0};

void anim_sparkle_init(uint8_t num_leds_on, uint8_t speed){
    anim_clear_all();
    
    sparkle_state.target_count = num_leds_on;
    if (sparkle_state.target_count > ANIM_NUM_LEDS) {
        sparkle_state.target_count = ANIM_NUM_LEDS;
    }
    
    sparkle_state.change_probability = speed;
    sparkle_state.last_update_frame = 0;
    
    // Beginn with random LEDs
    for (uint8_t i = 0; i < sparkle_state.target_count; i++) {
        uint8_t led = anim_random(ANIM_NUM_LEDS);
        anim_set_led(led, 1);
    }
}

// Update every frame
// returns bitpattern
uint32_t* anim_sparkle_update(void){
    anim_frame_counter++;
    
    uint8_t current_count = anim_count_leds_on();
    
    // probability change
    uint8_t do_change = (anim_random(256) < sparkle_state.change_probability);
    
    if (do_change){
        if (current_count < sparkle_state.target_count){ // find and turn on a off led
            uint8_t attempts = 0;
            while (attempts < 10) {
                uint8_t led = anim_random(ANIM_NUM_LEDS);
                if (!anim_get_led(led)) {
                    anim_set_led(led, 1);
                    break;
                }
                attempts++;
            }
        }
        else if (current_count > sparkle_state.target_count){ // find and turn off a on led
            uint8_t attempts = 0;
            while (attempts < 10) {
                uint8_t led = anim_random(ANIM_NUM_LEDS);
                if (anim_get_led(led)) {
                    anim_set_led(led, 0);
                    break;
                }
                attempts++;
            }
        }
        else{
            if (anim_random(2)) {  // 50% chance to swap for twinkle effect
                /* Turn off a random on LED */
                uint8_t attempts = 0;
                while (attempts < 10) {
                    uint8_t led = anim_random(ANIM_NUM_LEDS);
                    if (anim_get_led(led)) {
                        anim_set_led(led, 0);
                        break;
                    }
                    attempts++;
                }
                
                attempts = 0; // turn off random led
                while (attempts < 10) {
                    uint8_t led = anim_random(ANIM_NUM_LEDS);
                    if (!anim_get_led(led)) {
                        anim_set_led(led, 1);
                        break;
                    }
                    attempts++;
                }
            }
        }
    }
    
    return anim_pattern;
}

/* ===================================================================
 * ANIMATION: IMPULSE (Expanding wave effect)
 * =================================================================== */

/* Impulse animation state */
// typedef struct {
//     uint8_t center_led;         // Starting LED for the impulse
//     uint8_t current_radius;     // Current expansion radius
//     uint8_t max_radius;         // Maximum radius (when all LEDs are on)
//     uint8_t growth_rate;        // How fast the impulse grows (LEDs per frame)
//     uint8_t state;              // 0=expanding, 1=holding, 2=done
//     uint32_t hold_frames;       // How long to hold at full expansion
//     uint32_t hold_counter;      // Counter for hold state
// } ImpulseState;

// static ImpulseState impulse_state = {0};

// /**
//  * @brief Initialize impulse animation
//  * @param start_led Starting LED (center of impulse, 0-41)
//  * @param speed Growth rate (1=slow, 5=medium, 10=fast)
//  * @param hold_time How long to hold at full expansion in frames (0=no hold)
//  * 
//  * Creates an expanding wave effect starting from start_led and propagating
//  * outward until all LEDs are lit.
//  */
// void anim_impulse_init(uint8_t start_led, uint8_t speed, uint32_t hold_time)
// {
//     anim_clear_all();
    
//     if (start_led >= ANIM_NUM_LEDS) {
//         start_led = 0;
//     }
    
//     impulse_state.center_led = start_led;
//     impulse_state.current_radius = 0;
//     impulse_state.max_radius = ANIM_NUM_LEDS;  // Will expand to all LEDs
//     impulse_state.growth_rate = speed;
//     impulse_state.state = 0;  // Expanding
//     impulse_state.hold_frames = hold_time;
//     impulse_state.hold_counter = 0;
    
//     /* Start with just the center LED */
//     anim_set_led(start_led, 1);
// }

// /**
//  * @brief Update impulse animation (call this every frame)
//  * @return Pointer to pattern bitmask
//  * 
//  * Returns the current state of the impulse animation.
//  * Returns NULL when animation is complete.
//  */
// uint32_t* anim_impulse_update(void)
// {
//     anim_frame_counter++;
    
//     if (impulse_state.state == 0)  // Expanding
//     {
//         /* Expand the radius */
//         impulse_state.current_radius += impulse_state.growth_rate;
        
//         /* Calculate "distance" from center and light up nearby LEDs */
//         /* For simplicity, we use LED index distance */
//         for (uint8_t led = 0; led < ANIM_NUM_LEDS; led++) {
//             /* Calculate circular distance (wraps around) */
//             int16_t dist1 = abs((int16_t)led - (int16_t)impulse_state.center_led);
//             int16_t dist2 = ANIM_NUM_LEDS - dist1;  // Distance going the other way
//             uint8_t distance = (dist1 < dist2) ? dist1 : dist2;
            
//             if (distance <= impulse_state.current_radius) {
//                 anim_set_led(led, 1);
//             }
//         }
        
//         /* Check if we've reached maximum expansion */
//         if (impulse_state.current_radius >= impulse_state.max_radius) {
//             /* All LEDs should be on now */
//             anim_set_all();
            
//             if (impulse_state.hold_frames > 0) {
//                 impulse_state.state = 1;  // Go to hold state
//                 impulse_state.hold_counter = 0;
//             } else {
//                 impulse_state.state = 2;  // Done
//             }
//         }
//     }
//     else if (impulse_state.state == 1)  // Holding
//     {
//         impulse_state.hold_counter++;
        
//         if (impulse_state.hold_counter >= impulse_state.hold_frames) {
//             impulse_state.state = 2;  // Done
//         }
//     }
//     else  // Done (state == 2)
//     {
//         return NULL;  // Animation complete
//     }
    
//     return anim_pattern;
// }

// /**
//  * @brief Check if impulse animation is complete
//  * @return 1 if complete, 0 if still running
//  */
// uint8_t anim_impulse_is_complete(void)
// {
//     return (impulse_state.state == 2);
// }

// /**
//  * @brief Reset impulse animation to start again
//  */
// void anim_impulse_reset(void)
// {
//     anim_impulse_init(impulse_state.center_led, 
//                      impulse_state.growth_rate, 
//                      impulse_state.hold_frames);
// }

// /* ===================================================================
//  * ANIMATION: IMPULSE REVERSE (Collapsing effect)
//  * =================================================================== */

// /**
//  * @brief Initialize reverse impulse animation (collapse to center)
//  * @param end_led Target LED (center point, 0-41)
//  * @param speed Collapse rate (1=slow, 5=medium, 10=fast)
//  * 
//  * Starts with all LEDs on and collapses inward to a single LED.
//  */
// void anim_impulse_reverse_init(uint8_t end_led, uint8_t speed)
// {
//     anim_set_all();
    
//     if (end_led >= ANIM_NUM_LEDS) {
//         end_led = 0;
//     }
    
//     impulse_state.center_led = end_led;
//     impulse_state.current_radius = ANIM_NUM_LEDS;
//     impulse_state.max_radius = 0;
//     impulse_state.growth_rate = speed;
//     impulse_state.state = 0;  // Collapsing
//     impulse_state.hold_frames = 0;
// }

// /**
//  * @brief Update reverse impulse animation
//  * @return Pointer to pattern bitmask, NULL when complete
//  */
// uint32_t* anim_impulse_reverse_update(void)
// {
//     anim_frame_counter++;
    
//     if (impulse_state.state == 0)  // Collapsing
//     {
//         /* Decrease the radius */
//         if (impulse_state.current_radius >= impulse_state.growth_rate) {
//             impulse_state.current_radius -= impulse_state.growth_rate;
//         } else {
//             impulse_state.current_radius = 0;
//         }
        
//         /* Clear pattern and light only LEDs within radius */
//         anim_clear_all();
        
//         for (uint8_t led = 0; led < ANIM_NUM_LEDS; led++) {
//             /* Calculate distance from center */
//             int16_t dist1 = abs((int16_t)led - (int16_t)impulse_state.center_led);
//             int16_t dist2 = ANIM_NUM_LEDS - dist1;
//             uint8_t distance = (dist1 < dist2) ? dist1 : dist2;
            
//             if (distance <= impulse_state.current_radius) {
//                 anim_set_led(led, 1);
//             }
//         }
        
//         /* Check if we've collapsed to center */
//         if (impulse_state.current_radius == 0) {
//             /* Only center LED should be on */
//             anim_clear_all();
//             anim_set_led(impulse_state.center_led, 1);
//             impulse_state.state = 2;  // Done
//         }
//     }
//     else  // Done
//     {
//         return NULL;
//     }
    
//     return anim_pattern;
// }

/* ===================================================================
 * USAGE EXAMPLES
 * =================================================================== */

// /**
//  * @brief Example: Sparkle effect
//  */
// void anim_example_sparkle(void)
// {
//     /* Seed random number generator (do this once in your main) */
//     // srand(SystemCoreClock);  // Or use a better seed
    
//     /* Initialize: 8 LEDs twinkling, medium speed */
//     anim_sparkle_init(8, 128);
    
//     /* Enable multiplexing with initial pattern */
//     extern void charlie_enable_multiplex(uint32_t *bitmask);
//     charlie_enable_multiplex(anim_sparkle_update());
    
//     /* Update loop - call this every 20-50ms */
//     while(1) {
//         /* Update animation */
//         uint32_t *pattern = anim_sparkle_update();
        
//         /* Update display */
//         extern void charlie_update_multiplex_pattern(uint32_t *bitmask);
//         charlie_update_multiplex_pattern(pattern);
        
//         /* Delay between updates (adjust for desired animation speed) */
//         for(volatile int i = 0; i < 50000; i++);
//     }
// }

// /**
//  * @brief Example: Impulse effect
//  */
// void anim_example_impulse(void)
// {
//     while(1) {
//         /* Initialize impulse from LED 0, medium speed, hold for 100 frames */
//         anim_impulse_init(0, 2, 100);
        
//         extern void charlie_enable_multiplex(uint32_t *bitmask);
//         charlie_enable_multiplex(anim_impulse_update());
        
//         /* Animate expansion */
//         while (!anim_impulse_is_complete()) {
//             uint32_t *pattern = anim_impulse_update();
//             if (pattern) {
//                 extern void charlie_update_multiplex_pattern(uint32_t *bitmask);
//                 charlie_update_multiplex_pattern(pattern);
//             }
            
//             /* Update rate: ~50Hz */
//             for(volatile int i = 0; i < 20000; i++);
//         }
        
//         /* Hold briefly, then reverse */
//         for(volatile int i = 0; i < 500000; i++);
        
//         /* Collapse back */
//         anim_impulse_reverse_init(0, 2);
        
//         while (anim_impulse_reverse_update() != NULL) {
//             uint32_t *pattern = anim_impulse_reverse_update();
//             if (pattern) {
//                 extern void charlie_update_multiplex_pattern(uint32_t *bitmask);
//                 charlie_update_multiplex_pattern(pattern);
//             }
            
//             for(volatile int i = 0; i < 20000; i++);
//         }
        
//         /* Wait before repeating */
//         for(volatile int i = 0; i < 1000000; i++);
//     }
// }

// /**
//  * @brief Example: Combined effect - impulse then sparkle
//  */
// void anim_example_combined(void)
// {
//     extern void charlie_enable_multiplex(uint32_t *bitmask);
//     extern void charlie_update_multiplex_pattern(uint32_t *bitmask);
    
//     while(1) {
//         /* Impulse expands */
//         anim_impulse_init(ANIM_NUM_LEDS / 2, 3, 0);
//         charlie_enable_multiplex(anim_impulse_update());
        
//         while (!anim_impulse_is_complete()) {
//             uint32_t *pattern = anim_impulse_update();
//             if (pattern) {
//                 charlie_update_multiplex_pattern(pattern);
//             }
//             for(volatile int i = 0; i < 20000; i++);
//         }
        
//         /* Transition to sparkle */
//         for(volatile int i = 0; i < 200000; i++);
        
//         /* Sparkle for a while */
//         anim_sparkle_init(10, 180);
//         charlie_update_multiplex_pattern(anim_sparkle_update());
        
//         for (int frame = 0; frame < 500; frame++) {
//             charlie_update_multiplex_pattern(anim_sparkle_update());
//             for(volatile int i = 0; i < 30000; i++);
//         }
        
//         /* Collapse */
//         anim_impulse_reverse_init(ANIM_NUM_LEDS / 2, 3);
        
//         while (anim_impulse_reverse_update() != NULL) {
//             uint32_t *pattern = anim_impulse_reverse_update();
//             if (pattern) {
//                 charlie_update_multiplex_pattern(pattern);
//             }
//             for(volatile int i = 0; i < 20000; i++);
//         }
        
//         for(volatile int i = 0; i < 500000; i++);
//     }
// }

/* ===================================================================
 * NOTES
 * =================================================================== */

/*
 * SPARKLE ANIMATION:
 * - Creates a random twinkling effect
 * - Specify target number of LEDs to have on
 * - Speed controls how often LEDs change
 * - Good for ambient/decorative effects
 * - Adjust update rate (delay) to control overall animation speed
 * 
 * IMPULSE ANIMATION:
 * - Expanding wave from a center point
 * - LEDs turn on as the wave reaches them
 * - Can hold at full expansion
 * - Reverse version collapses back to center
 * - Good for power-on effects, notifications, reactions
 * 
 * COMBINING ANIMATIONS:
 * - Run one animation to completion
 * - Then start another
 * - Can create complex sequences
 * 
 * PERFORMANCE:
 * - Animations are lightweight (just bitmask manipulation)
 * - Update rate typically 20-50Hz (every 20-50ms)
 * - Hardware timer handles display - no blocking
 * 
 * RANDOM NUMBER GENERATION:
 * - Uses stdlib rand()
 * - IMPORTANT: Call srand() in your main() to seed it!
 * - Example: srand(SystemCoreClock) or srand(TIM2->CNT)
 * - Without seeding, pattern will be same every power-on
 */