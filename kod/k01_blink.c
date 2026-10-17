/* Liste 1-1  k01_blink.c - LD4 (PA5) yanip soner. */
#include <stdint.h>

#define REG32(a)     (*(volatile uint32_t *)(a))   /* "a adresindeki 32 bitlik kutu" */
#define RCC_IOPENR   REG32(0x40021000UL + 0x34)    /* GPIO saat acma     */
#define GPIOA_MODER  REG32(0x50000000UL + 0x00)    /* GPIOA pin modlari  */
#define GPIOA_BSRR   REG32(0x50000000UL + 0x18)    /* GPIOA set / reset  */

static void delay_ms(volatile uint32_t ms)      /* yaklasik bekleme dongusu */
{
    while (ms--) { volatile uint32_t i = 4000; while (i--) __asm volatile ("nop"); }
}

int main(void)
{
    RCC_IOPENR  |= (1u << 0);              /* 1. GPIOA saatini ac            */
    GPIOA_MODER &= ~(3u << (5 * 2));       /* 2. PA5 mod bitlerini temizle   */
    GPIOA_MODER |=  (1u << (5 * 2));       /*    PA5 = 01 = cikis            */

    for (;;) {                             /* 3. sonsuz dongu                */
        GPIOA_BSRR = (1u << 5);            /*    PA5 = 1  -> LED yanar       */
        delay_ms(500);
        GPIOA_BSRR = (1u << (5 + 16));     /*    PA5 = 0  -> LED soner       */
        delay_ms(500);
    }
}
