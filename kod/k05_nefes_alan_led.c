/* Liste 5-2  k05_nefes_alan_led.c - LD4 yavasca parlayip soner ("nefes alir"). */
#include "g071.h"

/* ---- SysTick: 1 ms zaman tabani ---- */
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) { ms_ticks++; }          /* her 1 ms'de calisir */

void systick_init(void)
{
    STK_LOAD = 16000 - 1;                            /* 16 MHz / 16000 = 1 kHz */
    STK_VAL  = 0;
    STK_CTRL = 0x7;                                  /* cekirdek saati, TICKINT, ENABLE */
}
uint32_t millis(void) { return ms_ticks; }
void delay_ms(uint32_t ms)
{
    uint32_t t0 = ms_ticks;
    while ((ms_ticks - t0) < ms) { }                 /* dogru ve tasmaya dayanikli */
}

/* ---- TIM2_CH1 PWM, PA5 (LD4, SCK/D13), ~976 Hz, doluluk 0..255 ---- */
void pwm_init(void)
{
    RCC_IOPENR  |= (1u << 0);                        /* GPIOA */
    RCC_APBENR1 |= (1u << 0);                        /* TIM2  */
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (2u << (5 * 2));                 /* PA5 -> AF */
    GPIOA_AFRL  &= ~(0xFu << (5 * 4));
    GPIOA_AFRL  |=  (2u << (5 * 4));                 /* AF2 = TIM2_CH1 */

    TIM2_PSC   = 63;                                 /* 16 MHz / 64 = 250 kHz */
    TIM2_ARR   = 255;                                /* 250 kHz / 256 = ~976 Hz */
    TIM2_CCMR1 = (6u << 4) | (1u << 3);              /* PWM mod 1 + on yukleme */
    TIM2_CCER  = (1u << 0);                          /* kanal 1 cikisini ac */
    TIM2_CCR1  = 0;                                  /* doluluk = 0 */
    TIM2_EGR   = (1u << 0);                          /* ayarlari yukle (UG) */
    TIM2_CR1   = (1u << 0);                          /* sayiciyi baslat */
}
void pwm_set(uint8_t duty) { TIM2_CCR1 = duty; }

int main(void)
{
    systick_init();
    pwm_init();
    for (;;) {
        for (int d = 0; d <= 255; d++)  { pwm_set((uint8_t)d); delay_ms(4); }  /* parla */
        for (int d = 255; d >= 0; d--)  { pwm_set((uint8_t)d); delay_ms(4); }  /* son   */
    }
}
