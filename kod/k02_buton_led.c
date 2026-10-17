/* Liste 2-2  k02_buton_led.c - B1'e basili oldukca LD4 yanar. */
#include <stdint.h>

#define REG32(a)     (*(volatile uint32_t *)(a))
#define RCC_IOPENR   REG32(0x40021000UL + 0x34)
#define GPIOA_MODER  REG32(0x50000000UL + 0x00)
#define GPIOA_BSRR   REG32(0x50000000UL + 0x18)
#define GPIOC_MODER  REG32(0x50000800UL + 0x00)
#define GPIOC_PUPDR  REG32(0x50000800UL + 0x0C)
#define GPIOC_IDR    REG32(0x50000800UL + 0x10)

int main(void)
{
    RCC_IOPENR  |= (1u << 0) | (1u << 2);   /* GPIOA ve GPIOC saatleri  */
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (1u << (5 * 2));        /* PA5 cikis (LD4)          */
    GPIOC_MODER &= ~(3u << (13 * 2));       /* PC13 giris (00)          */
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));       /* PC13 pull-up (01)        */

    for (;;) {
        if ((GPIOC_IDR & (1u << 13)) == 0)  /* 0 okunursa: basili      */
            GPIOA_BSRR = (1u << 5);
        else
            GPIOA_BSRR = (1u << (5 + 16));
    }
}
