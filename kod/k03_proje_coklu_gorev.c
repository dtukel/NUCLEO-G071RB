/* Liste 3-3  k03_proje_coklu_gorev.c - Bolum 3 projesi
 * Uc LED uc farkli hizda, birbirini beklemeden yanip soner (millis zamanlama).
 * B1 kesmesi tum animasyonu durdurur / yeniden baslatir.
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

static volatile uint8_t paused = 0;

void EXTI4_15_IRQHandler(void)
{
    static uint32_t last = 0;
    if (EXTI_FPR1 & (1u << 13)) {
        EXTI_FPR1 = (1u << 13);
        if (ms_ticks - last > 200) {             /* yazilim debounce: 200 ms */
            paused = !paused;
            last = ms_ticks;
        }
    }
}

static void button_irq_init(void)
{
    RCC_IOPENR  |= (1u << 2);
    GPIOC_MODER &= ~(3u << (13 * 2));
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));
    EXTI_EXTICR4 &= ~(0xFFu << 8);
    EXTI_EXTICR4 |=  (2u << 8);
    EXTI_FTSR1   |=  (1u << 13);
    EXTI_IMR1    |=  (1u << 13);
    NVIC_ISER     =  (1u << EXTI4_15_IRQn);
}

int main(void)
{
    systick_init();
    leds_init();
    button_irq_init();

    static const uint32_t period[3] = {100, 333, 750};  /* ms: her LED'in yari periyodu */
    uint32_t next[3] = {0, 0, 0};
    uint8_t  state = 0;                                  /* bit i = LED i */

    for (;;) {
        uint32_t now = millis();
        if (!paused) {
            for (int i = 0; i < 3; i++) {
                if ((int32_t)(now - next[i]) >= 0) {      /* zamani geldi mi? */
                    state ^= (uint8_t)(1u << i);
                    next[i] = now + period[i];
                }
            }
            leds_show(state);
        } else {
            leds_show(0);
        }
        __asm volatile ("wfi");                           /* bir sonraki tick'e kadar uyu */
    }
}
