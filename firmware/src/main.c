#include <ch32v00x.h>
#include <debug.h>
#include <stdlib.h>

#include "led_charlie.h"
#include "animations_simple.h"

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void Delay_Init(void);
void Delay_Ms(uint32_t n);

// Remaps I2C to PD0 (SDA) and PD1 (SCL)
// Warning: Disables SWD!
void i2c_remap(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_FullRemap_I2C1, ENABLE);
}

int is_button_pressed(void){
    return (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 1);
}

void init_button(void){
    GPIO_InitTypeDef button_init = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    button_init.GPIO_Pin = GPIO_Pin_2;
    button_init.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOD, &button_init);
}

int main(void){
    init_button();
    charlie_init();
    if(is_button_pressed()){
        while(1){   // Wait for SWD...
            charlie_single(17, 1);
            for(volatile int i = 0; i < 100000; i++);
            charlie_single(17, 0);
            for(volatile int i = 0; i < 100000; i++);
        }
    } else {
        i2c_remap();
    }

    //charlie_test();

    charlie_set_fast_pwm_mode(1);
    charlie_set_brightness(40);  // Adjust brightness as needed
    twinkle_init();
    
    /* Start with first random frame */
    charlie_enable_multiplex(twinkle_next_frame());
    
    /* Main loop - just keep changing frames randomly */
    while(1) {
        /* Get next random frame and display it */
        charlie_update_multiplex_pattern(twinkle_next_frame());
        
        /* Wait ~500ms between changes (adjust delay to taste) */
        for(volatile uint32_t i = 0; i < 500000; i++);
    }
    

    // set interrupts

    // enable i2c

    // init sc7a20

    // set sleep
    
}


    