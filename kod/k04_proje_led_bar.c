/* Liste 4-2  k04_proje_led_bar.c - Bolum 4 projesi
 * Potansiyometre konumu uc LED'li cubuk gostergede: 0, 1, 2 veya 3 LED yanar.
 * Esik degerlerinde titremeyi onlemek icin histerezis kullanilir.
 */
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

/* ---- uc harici LED: MISO/D12 (PA6), PWM/MOSI/D11 (PA7), D7 (PA8) ---- */
static const uint8_t LED_PIN[3] = {6, 7, 8};

void leds_init(void)
{
    RCC_IOPENR |= (1u << 0);                         /* GPIOA saati */
    for (int i = 0; i < 3; i++) {
        GPIOA_MODER &= ~(3u << (LED_PIN[i] * 2));
        GPIOA_MODER |=  (1u << (LED_PIN[i] * 2));    /* cikis */
    }
}
void leds_show(uint8_t mask)                  /* bit i -> LED i */
{
    uint32_t set = 0, clr = 0;
    for (int i = 0; i < 3; i++) {
        if (mask & (1u << i)) set |= 1u << LED_PIN[i];
        else                  clr |= 1u << LED_PIN[i];
    }
    GPIOA_BSRR = set | (clr << 16);
}

int main(void)
{
    systick_init();
    adc_init();
    leds_init();

    uint8_t level = 0;                                /* 0..3 yanan LED sayisi */
    const uint16_t step = 4096 / 4;                   /* 1024: her seviye      */
    const uint16_t hyst = 60;                         /* ~50 mV histerezis     */

    for (;;) {
        uint16_t v = adc_read();
        /* yukari cikmak icin esigi hyst kadar gecmek, inmek icin hyst kadar altina dusmek gerekir */
        if (level < 3 && v > (level + 1) * step + hyst) level++;
        if (level > 0 && v < level * step - hyst)       level--;
        leds_show((uint8_t)((1u << level) - 1u));     /* 0->000 1->001 2->011 3->111 */
        delay_ms(20);
    }
}
