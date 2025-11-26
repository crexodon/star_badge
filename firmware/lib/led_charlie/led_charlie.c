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
static uint8_t multiplex_pattern[CHARLIE_NUM_LEDS] = {0};  // Which LEDs are currently on (1=on, 0=off)
static uint8_t multiplex_enabled = 0;
static uint8_t current_led_index = 0;

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
    charlie_tim.TIM_Prescaler = 48 - 1;
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
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET){
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // clear flag

        pwm_counter++;

        // Turn on if pwm counter is not reached
        if(pwm_counter < charlie_brightness && charlie_brightness > 0){
            if(!led_is_on && current_anode_pin != 0){
                charlie_pin_high(current_anode_pin);
                charlie_pin_low(current_cathode_pin);
                led_is_on = 1;
            }
        } else { // turn led off
            if(led_is_on){
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

void charlie_multi_on(uint8_t pattern[]){
    __disable_irq();

    for(int i = 0; i < CHARLIE_NUM_LEDS; i++){
        multiplex_pattern[i] = pattern[i];
    }
}

// Lights a single LED, use schematic to find led number pair (odd toplayer, even bottlayer)
// state 0 = off, 1 = on (with pwm)
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

void charlie_test(){
    charlie_set_brightness(255);
    int cnt = 0;

    while(cnt < 5){
        for (uint8_t led = 0; led < CHARLIE_NUM_LEDS; led++){
            charlie_single(led, 1);

            for(volatile int i = 0; i < 500000; i++);
            cnt++;
        }
    }

    charlie_off();

    charlie_single(41, 1);

    cnt = 0;
    while(cnt < 10){
        for (uint8_t brightness = 0; brightness < 255; brightness++){
            charlie_set_brightness(brightness);
            for(volatile int i = 0; i < 500000; i++);
        }
        for (uint8_t brightness = 255; brightness > 0; brightness--){
            charlie_set_brightness(brightness);
            for(volatile int i = 0; i < 500000; i++);
        }
    }


}