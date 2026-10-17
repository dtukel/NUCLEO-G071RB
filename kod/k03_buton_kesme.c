/* Liste 3-2  k03_buton_kesme.c - B1 kesmesi LD4'u tersler (yoklama yok). */
#include "g071.h"

void EXTI4_15_IRQHandler(void)                  /* B1 = PC13 = EXTI hatti 13 */
{
    if (EXTI_FPR1 & (1u << 13)) {               /* dusen kenar mi geldi?     */
        EXTI_FPR1 = (1u << 13);                 /* bayragi temizle (1 yaz)   */
        GPIOA_ODR ^= (1u << 5);                 /* LD4'u tersle              */
    }
}

int main(void)
{
    RCC_IOPENR  |= (1u << 0) | (1u << 2);       /* GPIOA, GPIOC */
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (1u << (5 * 2));            /* PA5 cikis */
    GPIOC_MODER &= ~(3u << (13 * 2));           /* PC13 giris */
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));           /* pull-up */

    EXTI_EXTICR4 &= ~(0xFFu << 8);              /* hat 13 alani = bit 15:8 */
    EXTI_EXTICR4 |=  (2u << 8);                 /* 2 = port C              */
    EXTI_FTSR1   |=  (1u << 13);                /* dusen kenarda tetikle   */
    EXTI_IMR1    |=  (1u << 13);                /* hatti ac (maske)        */
    NVIC_ISER     =  (1u << EXTI4_15_IRQn);     /* NVIC'te kesmeyi ac      */

    for (;;) {
        __asm volatile ("wfi");                 /* kesme gelene kadar uyu  */
    }
}
