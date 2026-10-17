/* Liste 2-1  k02_kayan_isik.c - uc LED'de kayan isik. */
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

static const uint8_t LED_PIN[3] = {6, 7, 8};   /* MISO/D12, PWM/MOSI/D11, D7 */

int main(void)
{
    RCC_IOPENR |= (1u << 0);                          /* GPIOA */
    for (int i = 0; i < 3; i++) {
        GPIOA_MODER &= ~(3u << (LED_PIN[i] * 2));
        GPIOA_MODER |=  (1u << (LED_PIN[i] * 2));     /* cikis */
    }
    for (;;) {
        for (int i = 0; i < 3; i++) {
            GPIOA_BSRR = (1u << LED_PIN[i]);          /* yak    */
            delay_ms(150);
            GPIOA_BSRR = (1u << (LED_PIN[i] + 16));   /* sondur */
        }
    }
}
