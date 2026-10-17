/* Liste 4-1  k04_adc_okuma.c - potansiyometreyi oku, degeri hata ayiklayicida izle. */
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

/* ---- ADC: potansiyometre A0 (PA0) = kanal 0 ---- */
void adc_init(void)
{
    RCC_IOPENR  |= (1u << 0);                        /* GPIOA */
    RCC_APBENR2 |= (1u << 20);                       /* ADC   */
    GPIOA_MODER |= (3u << (0 * 2));                  /* PA0 analog (11) */

    ADC_CFGR2 = (1u << 30);                          /* ADC saati = PCLK/2 */
    ADC_CR    = (1u << 28);                          /* ADVREGEN: regulatoru ac */
    for (volatile int i = 0; i < 1000; i++) { }      /* > 20 us acilis suresi */
    ADC_CR |= (1u << 31);                            /* ADCAL: kalibrasyon */
    while (ADC_CR & (1u << 31)) { }
    ADC_CR |= (1u << 0);                             /* ADEN */
    while (!(ADC_ISR & (1u << 0))) { }               /* ADRDY'yi bekle */

    ADC_SMPR   = 7;                                  /* en uzun ornekleme suresi */
    ADC_CHSELR = (1u << 0);                          /* kanal 0 */
    while (!(ADC_ISR & (1u << 13))) { }              /* CCRDY'yi bekle (ayar uygulandi) */
    ADC_ISR = (1u << 13);
}
uint16_t adc_read(void)                       /* 0 .. 4095 */
{
    ADC_CR |= (1u << 2);                             /* ADSTART */
    while (!(ADC_ISR & (1u << 2))) { }               /* EOC'yi bekle */
    return (uint16_t)ADC_DR;
}

volatile uint16_t pot;          /* global: Live Expressions penceresinde izlenir */
volatile uint32_t pot_mV;

int main(void)
{
    systick_init();
    adc_init();
    for (;;) {
        pot    = adc_read();                          /* 0 .. 4095   */
        pot_mV = (uint32_t)pot * 3300u / 4095u;       /* 0 .. 3300 mV */
        delay_ms(100);
    }
}
