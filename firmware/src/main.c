#include <ch32v00x.h>
#include <debug.h>

#include "led_charlie.h"

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

    charlie_test();

    // set interrupts

    // enable i2c

    // init sc7a20

    // set sleep
    
}