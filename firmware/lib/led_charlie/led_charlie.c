#include "led_charlie.h"
#include <ch32v00x.h>

#define CHARLIE_GPIO_PORT   GPIOC
#define CHARLIE_RCC_PERIPH  RCC_APB2Periph_GPIOC

#define CHARLIE_PIN_0       GPIO_Pin_0
#define CHARLIE_PIN_1       GPIO_Pin_1
#define CHARLIE_PIN_2       GPIO_Pin_2
#define CHARLIE_PIN_3       GPIO_Pin_3
#define CHARLIE_PIN_4       GPIO_Pin_5
#define CHARLIE_PIN_5       GPIO_Pin_6
#define CHARLIE_PIN_6       GPIO_Pin_7

#define CHARLIE_NUM_PINS    7
#define CHARLIE_NUM_LEDS    42
#define CHARLIE_BITMASK_SIZE 2

static const uint16_t charlie_pins[CHARLIE_NUM_PINS] = {
    CHARLIE_PIN_0,
    CHARLIE_PIN_1,
    CHARLIE_PIN_2,
    CHARLIE_PIN_3,
    CHARLIE_PIN_4,
    CHARLIE_PIN_5,
    CHARLIE_PIN_6
};

typedef struct {
    uint16_t anode;
    uint16_t cathode;
} charlie_led_config;

static const charlie_led_config full_charlie_matrix[CHARLIE_NUM_LEDS] = {
    {CHARLIE_PIN_6, CHARLIE_PIN_0}, // D1,2
    {CHARLIE_PIN_0, CHARLIE_PIN_6}, // D3,4
    {CHARLIE_PIN_5, CHARLIE_PIN_0}, // D5,6
    {CHARLIE_PIN_0, CHARLIE_PIN_5}, // D7,8
    {CHARLIE_PIN_4, CHARLIE_PIN_0}, // D9,10
    {CHARLIE_PIN_0, CHARLIE_PIN_4}, // D11,12
    {CHARLIE_PIN_3, CHARLIE_PIN_0}, // D13,14
    {CHARLIE_PIN_0, CHARLIE_PIN_3}, // D15,16
    {CHARLIE_PIN_2, CHARLIE_PIN_0}, // D17,18
    {CHARLIE_PIN_0, CHARLIE_PIN_2}, // D19,20
    {CHARLIE_PIN_1, CHARLIE_PIN_0}, // D21,22
    {CHARLIE_PIN_0, CHARLIE_PIN_1}, // D23,24
    {CHARLIE_PIN_6, CHARLIE_PIN_1}, // D25,26
    {CHARLIE_PIN_1, CHARLIE_PIN_6}, // D27,28
    {CHARLIE_PIN_5, CHARLIE_PIN_1}, // D29,30
    {CHARLIE_PIN_1, CHARLIE_PIN_5}, // D31,32
    {CHARLIE_PIN_4, CHARLIE_PIN_1}, // D33,34
    {CHARLIE_PIN_1, CHARLIE_PIN_4}, // D35,36
    {CHARLIE_PIN_3, CHARLIE_PIN_1}, // D37,38
    {CHARLIE_PIN_1, CHARLIE_PIN_3}, // D39,40
    {CHARLIE_PIN_2, CHARLIE_PIN_1}, // D41,42
    {CHARLIE_PIN_1, CHARLIE_PIN_2}, // D43,44
    {CHARLIE_PIN_6, CHARLIE_PIN_2}, // D45,46
    {CHARLIE_PIN_2, CHARLIE_PIN_6}, // D47,48
    {CHARLIE_PIN_5, CHARLIE_PIN_2}, // D49,50
    {CHARLIE_PIN_2, CHARLIE_PIN_5}, // D51,52
    {CHARLIE_PIN_4, CHARLIE_PIN_2}, // D53,54
    {CHARLIE_PIN_2, CHARLIE_PIN_4}, // D55,56
    {CHARLIE_PIN_3, CHARLIE_PIN_2}, // D57,58
    {CHARLIE_PIN_2, CHARLIE_PIN_3}, // D59,60
    {CHARLIE_PIN_6, CHARLIE_PIN_3}, // D61,62
    {CHARLIE_PIN_3, CHARLIE_PIN_6}, // D63,64
    {CHARLIE_PIN_5, CHARLIE_PIN_3}, // D65,66
    {CHARLIE_PIN_3, CHARLIE_PIN_5}, // D67,68
    {CHARLIE_PIN_4, CHARLIE_PIN_3}, // D69,70
    {CHARLIE_PIN_3, CHARLIE_PIN_4}, // D71,72
    {CHARLIE_PIN_6, CHARLIE_PIN_4}, // D73,74
    {CHARLIE_PIN_4, CHARLIE_PIN_6}, // D75,76
    {CHARLIE_PIN_5, CHARLIE_PIN_4}, // D77,78
    {CHARLIE_PIN_4, CHARLIE_PIN_5}, // D79,80
    {CHARLIE_PIN_6, CHARLIE_PIN_5}, // D81,82
    {CHARLIE_PIN_5, CHARLIE_PIN_6}, // D83,84
};

// more charlie matrix combinations

// Timer
static volatile uint8_t charlie_brightness = 128;
static volatile uint16_t current_anode_pin = 0;
static volatile uint16_t current_cathode_pin = 0;
static volatile uint8_t pwm_counter = 0;
static volatile uint8_t led_is_on = 0;

// Multiplex
static uint32_t multiplex_bitmask[CHARLIE_BITMASK_SIZE] = {0};
static uint8_t multiplex_enabled = 0;
static uint8_t current_led_index = 0;
static volatile uint8_t fast_pwm_mode = 0;

// --- Internal Charlie Functions ---

void charlie_pin_high(uint16_t pin){
    GPIO_InitTypeDef cph_init = {0};

    cph_init.GPIO_Pin = pin;
    cph_init.GPIO_Mode = GPIO_Mode_Out_PP;
    cph_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CHARLIE_GPIO_PORT, &cph_init);

    GPIO_SetBits(CHARLIE_GPIO_PORT, pin);
}

void charlie_pin_low(uint16_t pin){
    GPIO_InitTypeDef cpl_init = {0};

    cpl_init.GPIO_Pin = pin;
    cpl_init.GPIO_Mode = GPIO_Mode_Out_PP;
    cpl_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(CHARLIE_GPIO_PORT, &cpl_init);

    GPIO_ResetBits(CHARLIE_GPIO_PORT, pin);
}

void charlie_pin_tri(uint16_t pin){
    GPIO_InitTypeDef cpt_init = {0};

    cpt_init.GPIO_Pin = pin;
    cpt_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(CHARLIE_GPIO_PORT, &cpt_init);
}

void charlie_set_fast_pwm_mode(uint8_t enable)
{
    fast_pwm_mode = enable;
}

static inline uint8_t charlie_is_led_enabled(uint8_t led_num)
{
    if (led_num >= CHARLIE_NUM_LEDS) return 0;
    
    uint8_t index = led_num / 32; // Which uint32_t in the array
    uint8_t bit = led_num % 32; // Which bit in that uint32_t
    
    return (multiplex_bitmask[index] & (1UL << bit)) != 0;
}

void charlie_disable_multiplex(void)
{
    __disable_irq();
    multiplex_enabled = 0;
    charlie_set_led(current_led_index, 0);
    __enable_irq();
}

// ---

// Turns of all leds
void charlie_off(){
    for(int i = 0; i < CHARLIE_NUM_PINS; i++){
        charlie_pin_tri(charlie_pins[i]);
    }
}

void charlie_init(){
    RCC_APB2PeriphClockCmd(CHARLIE_RCC_PERIPH, ENABLE);

    charlie_off();

    TIM_TimeBaseInitTypeDef charlie_tim = {0};
    NVIC_InitTypeDef charlie_nvic = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // Set Timer
    charlie_tim.TIM_Period = 20 - 1;
    charlie_tim.TIM_Prescaler = 6 - 1;
    charlie_tim.TIM_ClockDivision = TIM_CKD_DIV1;
    charlie_tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &charlie_tim);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    // Set Timer Interrupt
    charlie_nvic.NVIC_IRQChannel = TIM2_IRQn;
    charlie_nvic.NVIC_IRQChannelPreemptionPriority = 1;
    charlie_nvic.NVIC_IRQChannelSubPriority = 0;
    charlie_nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&charlie_nvic);

    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM2_IRQHandler(void){
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // clear bit
        
        if (fast_pwm_mode) {
            pwm_counter++;
            if (pwm_counter >= 64) pwm_counter = 0;
        } else {
            pwm_counter++;  // Wraps naturally at 256
        }
        
        if (pwm_counter == 0 && multiplex_enabled){ // switch led in multi mode
            uint8_t found = 0;
            uint8_t search_count = 0;
            
            while (search_count < CHARLIE_NUM_LEDS){ // look for next led
                if (charlie_is_led_enabled(current_led_index)){ // check bitmask
                    if (current_led_index < CHARLIE_NUM_LEDS) {  // Bounds check for led_matrix
                        current_anode_pin = full_charlie_matrix[current_led_index].anode;
                        current_cathode_pin = full_charlie_matrix[current_led_index].cathode;
                        found = 1;
                    }

                    current_led_index++;
                    if (current_led_index >= CHARLIE_NUM_LEDS) {
                        current_led_index = 0;
                    }
                
                    break;
                }

                current_led_index++;
                if (current_led_index >= CHARLIE_NUM_LEDS) {
                    current_led_index = 0;
                }
                
                search_count++;
            }
            
            if (!found)
            {
                current_anode_pin = 0;
                current_cathode_pin = 0;
            }
            
            led_is_on = 0;
        }
        
        if (pwm_counter < charlie_brightness && charlie_brightness > 0)
        {
            if (!led_is_on && current_anode_pin != 0)
            {
                charlie_pin_high(current_anode_pin);
                charlie_pin_low(current_cathode_pin);
                led_is_on = 1;
            }
        }
        else
        {
            /* LED should be OFF */
            if (led_is_on)
            {
                /* Turn LED off (tri-state) */
                charlie_off();
                led_is_on = 0;
            }
        }
    }
}



// internal, disables briefly interrupts to update led
void charlie_light_single_on(uint16_t anode_pin, uint16_t cathode_pin){
    __disable_irq();
    charlie_off();
    led_is_on = 0;

    current_anode_pin = anode_pin;
    current_cathode_pin = cathode_pin;
    pwm_counter = 0;

    __enable_irq();
}

void charlie_light_single_off(){
    __disable_irq();
    current_anode_pin = 0;
    current_cathode_pin = 0;

    charlie_off();
    led_is_on = 0;

    __enable_irq();
}

// new bitmask approach to set leds
void charlie_set_led(uint8_t led_num, uint8_t state)
{
    if (led_num >= CHARLIE_NUM_LEDS) return;
    
    uint8_t index = led_num / 32;
    uint8_t bit = led_num % 32;
    
    __disable_irq();
    
    if (state) {
        multiplex_bitmask[index] |= (1UL << bit);   // Set bit
    } else {
        multiplex_bitmask[index] &= ~(1UL << bit);  // Clear bit
    }
    
    __enable_irq();
}

void charlie_enable_multiplex(uint32_t *bitmask)
{
    __disable_irq();
    
    for (int i = 0; i < CHARLIE_BITMASK_SIZE; i++) {
        multiplex_bitmask[i] = bitmask[i]; // copy to buffer
    }
    
    multiplex_enabled = 1;
    current_led_index = 0;
    
    __enable_irq();
}



void charlie_update_multiplex_pattern(uint32_t *bitmask)
{
    __disable_irq();
    
    for (int i = 0; i < CHARLIE_BITMASK_SIZE; i++) {
        multiplex_bitmask[i] = bitmask[i];
    }
    
    __enable_irq();
}

void charlie_clear_multiplex_pattern(void)
{
    __disable_irq();
    
    for (int i = 0; i < CHARLIE_BITMASK_SIZE; i++) {
        multiplex_bitmask[i] = 0;
    }
    
    __enable_irq();
}

void charlie_set_all_multiplex_leds(void)
{
    __disable_irq();
    
    /* Set all bits */
    for (int i = 0; i < CHARLIE_BITMASK_SIZE; i++) {
        multiplex_bitmask[i] = 0xFFFFFFFF;
    }
    
    /* Clear any bits beyond CHARLIE_MAX_LEDS */
    uint8_t remainder = CHARLIE_NUM_LEDS % 32;
    if (remainder > 0) {
        multiplex_bitmask[CHARLIE_BITMASK_SIZE - 1] = (1UL << remainder) - 1;
    }
    
    __enable_irq();
}

//Lights a single LED, use schematic to find led number pair (odd toplayer, even bottlayer)
//state 0 = off, 1 = on (with pwm)
void charlie_single(uint8_t led_num, uint8_t state){
    if(led_num >= CHARLIE_NUM_LEDS) return;

    if(state) {
        charlie_light_single_on(full_charlie_matrix[led_num].anode, full_charlie_matrix[led_num].cathode);
    } else {
        charlie_light_single_off();
    }
}

void charlie_set_brightness(uint8_t brightness){
    charlie_brightness = brightness;
}

uint8_t charlie_get_brightness(){
    return charlie_brightness;
}

void charlie_multi_mask(uint16_t high_mask, uint16_t low_mask, uint16_t tri_mask){
    GPIO_InitTypeDef charlie_multi_init = {0};

    if(tri_mask){
        charlie_multi_init.GPIO_Pin = tri_mask;
        charlie_multi_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(CHARLIE_GPIO_PORT, &charlie_multi_init);
    }

    if(high_mask | low_mask){
        charlie_multi_init.GPIO_Pin = high_mask | low_mask;
        charlie_multi_init.GPIO_Mode = GPIO_Mode_Out_PP;
        charlie_multi_init.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(CHARLIE_GPIO_PORT, &charlie_multi_init);

        if(high_mask){
            GPIO_SetBits(CHARLIE_GPIO_PORT, high_mask);
        }
        if(low_mask){
            GPIO_ResetBits(CHARLIE_GPIO_PORT, low_mask);
        }
    }
}

void charlie_test(){
    charlie_set_brightness(255);
    int cnt = 0;

    while(cnt < 2){
        for (uint8_t led = 0; led < CHARLIE_NUM_LEDS; led++){
            charlie_single(led, 1);

            for(volatile int i = 0; i < 50000; i++);
            cnt++;
        }
    }

    charlie_off();

    charlie_single(41, 1);

    cnt = 0;
    while(cnt < 1){
        for (uint8_t brightness = 0; brightness < 255; brightness++){
            charlie_set_brightness(brightness);
            for(volatile int i = 0; i < 50000; i++);
        }
        for (uint8_t brightness = 255; brightness > 0; brightness--){
            charlie_set_brightness(brightness);
            for(volatile int i = 0; i < 50000; i++);
        }
        cnt++;
    }

    charlie_off();

    charlie_set_brightness(30);
    charlie_set_fast_pwm_mode(1);

    charlie_set_all_multiplex_leds();
    multiplex_enabled = 1;

    for(volatile int i = 0; i < 500000; i++);

    charlie_disable_multiplex();

    uint32_t pattern1[2], pattern2[2];

    pattern1[0] = 0x55555555;
    pattern1[1] = 0x000002AA;

    pattern2[0] = 0xAAAAAAAA;
    pattern2[1] = 0x00000155;

    charlie_enable_multiplex(pattern1);

    while(cnt < 10){
        charlie_update_multiplex_pattern(pattern1);
        for(volatile int i = 0; i < 500000; i++);
        charlie_update_multiplex_pattern(pattern2);
        for(volatile int i = 0; i < 500000; i++);
        cnt++;
    }

    charlie_disable_multiplex();
    charlie_set_fast_pwm_mode(0);

    charlie_off();
}