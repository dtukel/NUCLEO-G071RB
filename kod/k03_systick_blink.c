/* Liste 3-1  k03_systick_blink.c - SysTick ile tam 1 Hz yanip sonme. */
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

int main(void)
{
    RCC_IOPENR  |= (1u << 0);
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (1u << (5 * 2));            /* PA5 (LD4) cikis */
    systick_init();

    for (;;) {
        GPIOA_ODR ^= (1u << 5);                 /* LD4'u tersle */
        delay_ms(500);                          /* artik gercekten 500 ms */
    }
}
