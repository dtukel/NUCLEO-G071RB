/* Liste 2-3  k02_toggle_debounce.c - her basista LD4 durum degistirir. */
#include <stdint.h>

#define REG32(a)     (*(volatile uint32_t *)(a))
#define RCC_IOPENR   REG32(0x40021000UL + 0x34)
#define GPIOA_MODER  REG32(0x50000000UL + 0x00)
#define GPIOA_BSRR   REG32(0x50000000UL + 0x18)
#define GPIOC_MODER  REG32(0x50000800UL + 0x00)
#define GPIOC_PUPDR  REG32(0x50000800UL + 0x0C)
#define GPIOC_IDR    REG32(0x50000800UL + 0x10)

static void delay_ms(volatile uint32_t ms)      /* yaklasik bekleme dongusu */
{
    while (ms--) { volatile uint32_t i = 4000; while (i--) __asm volatile ("nop"); }
}

static int pressed(void) { return (GPIOC_IDR & (1u << 13)) == 0; }

int main(void)
{
    RCC_IOPENR  |= (1u << 0) | (1u << 2);
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (1u << (5 * 2));
    GPIOC_MODER &= ~(3u << (13 * 2));
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));

    int led = 0;
    for (;;) {
        if (pressed()) {
            delay_ms(20);                    /* sicramalar bitsin        */
            if (pressed()) {                 /* hala basili mi?          */
                led = !led;
                GPIOA_BSRR = led ? (1u << 5) : (1u << (5 + 16));
                while (pressed()) { }        /* birakilmasini bekle      */
                delay_ms(20);                /* birakma sicramalari      */
            }
        }
    }
}
