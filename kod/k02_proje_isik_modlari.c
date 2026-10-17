/* Liste 2-4  k02_proje_isik_modlari.c - Bolum 2 projesi
 * B1'e her basista uc LED'in isik modu degisir:
 *   0 = kayan isik   1 = gidip gelen (Knight Rider)   2 = hepsi yanip soner   3 = kapali
 * Tusa basmayi kacirmamak icin bekleme 10 ms'lik dilimlere bolunmustur.
 */
#include "g071.h"

static void delay_ms(volatile uint32_t ms)      /* yaklasik bekleme dongusu */
{
    while (ms--) { volatile uint32_t i = 4000; while (i--) __asm volatile ("nop"); }
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

/* ---- USER butonu B1 = PC13, basinca 0 ---- */
void button_init(void)
{
    RCC_IOPENR  |= (1u << 2);                        /* GPIOC saati */
    GPIOC_MODER &= ~(3u << (13 * 2));                /* giris */
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));                /* pull-up */
}
int button_down(void) { return !(GPIOC_IDR & (1u << 13)); }

static uint8_t mode = 0;

/* 'ms' kadar bekler ama her 10 ms'de butona bakar; basildiysa 1 dondurur */
static int wait_or_press(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 10) {
        if (button_down()) {
            delay_ms(20);                                /* debounce */
            if (button_down()) {
                mode = (uint8_t)((mode + 1) & 3);
                while (button_down()) { }
                delay_ms(20);
                return 1;
            }
        }
        delay_ms(10);
    }
    return 0;
}

int main(void)
{
    leds_init();
    button_init();
    static const uint8_t knight[4] = {1, 2, 4, 2};      /* 001 010 100 010 */

    for (;;) {
        switch (mode) {
        case 0:                                          /* kayan isik */
            for (int i = 0; i < 3; i++) { leds_show(1u << i); if (wait_or_press(150)) break; }
            break;
        case 1:                                          /* gidip gelen */
            for (int i = 0; i < 4; i++) { leds_show(knight[i]); if (wait_or_press(120)) break; }
            break;
        case 2:                                          /* hepsi yanip soner */
            leds_show(7); if (wait_or_press(300)) break;
            leds_show(0); wait_or_press(300);
            break;
        default:                                         /* kapali */
            leds_show(0); wait_or_press(100);
            break;
        }
    }
}
