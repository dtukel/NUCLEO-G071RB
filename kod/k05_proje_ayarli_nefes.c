/* Liste 5-3  k05_proje_ayarli_nefes.c - Bolum 5 projesi
 * Nefes alan LED; nefes hizini potansiyometre belirler (yaklasik 0.5 s .. 5 s).
 * Gozun parlakligi dogrusal algilamamasi icin kare (gamma ~2) egrisi kullanilir.
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
    adc_init();
    pwm_init();

    uint16_t phase = 0;                                  /* 0..511: yukari + asagi */
    for (;;) {
        uint16_t pot = adc_read();                       /* 0..4095 */
        uint32_t step_ms = 1 + pot / 512u;               /* 1..8 ms her adim */

        uint16_t x = (phase < 256) ? phase : (uint16_t)(511 - phase);   /* ucgen dalga */
        pwm_set((uint8_t)((x * x) / 255u));              /* gamma ~2 */

        phase = (uint16_t)((phase + 1) & 511);
        delay_ms(step_ms);
    }
}
