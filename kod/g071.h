/* g071.h - STM32G071 register definitions used in the COME327 booklet.
 * Only the registers we use; addresses from RM0444 / STM32G071.svd.
 * Put this file in the project's Inc/ folder and write  #include "g071.h"
 */
#ifndef G071_H
#define G071_H
#include <stdint.h>

#define REG32(a)      (*(volatile uint32_t *)(a))

/* ---- RCC: clock enables ---- */
#define RCC_BASE      0x40021000UL
#define RCC_IOPENR    REG32(RCC_BASE + 0x34)   /* bit0 GPIOA, bit1 GPIOB, bit2 GPIOC  */
#define RCC_APBENR1   REG32(RCC_BASE + 0x3C)   /* bit0 TIM2, bit17 USART2, bit21 I2C1 */
#define RCC_APBENR2   REG32(RCC_BASE + 0x40)   /* bit12 SPI1, bit20 ADC               */

/* ---- GPIO (IOPORT bus) ---- */
#define GPIOA_BASE    0x50000000UL
#define GPIOB_BASE    0x50000400UL
#define GPIOC_BASE    0x50000800UL
#define GPIOA_MODER   REG32(GPIOA_BASE + 0x00)
#define GPIOA_OTYPER  REG32(GPIOA_BASE + 0x04)
#define GPIOA_OSPEEDR REG32(GPIOA_BASE + 0x08)
#define GPIOA_PUPDR   REG32(GPIOA_BASE + 0x0C)
#define GPIOA_IDR     REG32(GPIOA_BASE + 0x10)
#define GPIOA_ODR     REG32(GPIOA_BASE + 0x14)
#define GPIOA_BSRR    REG32(GPIOA_BASE + 0x18)
#define GPIOA_AFRL    REG32(GPIOA_BASE + 0x20)
#define GPIOA_AFRH    REG32(GPIOA_BASE + 0x24)
#define GPIOB_MODER   REG32(GPIOB_BASE + 0x00)
#define GPIOB_OTYPER  REG32(GPIOB_BASE + 0x04)
#define GPIOB_OSPEEDR REG32(GPIOB_BASE + 0x08)
#define GPIOB_PUPDR   REG32(GPIOB_BASE + 0x0C)
#define GPIOB_IDR     REG32(GPIOB_BASE + 0x10)
#define GPIOB_ODR     REG32(GPIOB_BASE + 0x14)
#define GPIOB_BSRR    REG32(GPIOB_BASE + 0x18)
#define GPIOB_AFRL    REG32(GPIOB_BASE + 0x20)
#define GPIOB_AFRH    REG32(GPIOB_BASE + 0x24)
#define GPIOC_MODER   REG32(GPIOC_BASE + 0x00)
#define GPIOC_PUPDR   REG32(GPIOC_BASE + 0x0C)
#define GPIOC_IDR     REG32(GPIOC_BASE + 0x10)
#define GPIOC_ODR     REG32(GPIOC_BASE + 0x14)
#define GPIOC_BSRR    REG32(GPIOC_BASE + 0x18)

/* ---- Cortex-M0+ core: SysTick and NVIC ---- */
#define STK_CTRL      REG32(0xE000E010UL)
#define STK_LOAD      REG32(0xE000E014UL)
#define STK_VAL       REG32(0xE000E018UL)
#define NVIC_ISER     REG32(0xE000E100UL)
#define EXTI4_15_IRQn 7

/* ---- EXTI ---- */
#define EXTI_BASE     0x40021800UL
#define EXTI_RTSR1    REG32(EXTI_BASE + 0x00)
#define EXTI_FTSR1    REG32(EXTI_BASE + 0x04)
#define EXTI_RPR1     REG32(EXTI_BASE + 0x0C)
#define EXTI_FPR1     REG32(EXTI_BASE + 0x10)
#define EXTI_EXTICR4  REG32(EXTI_BASE + 0x6C)
#define EXTI_IMR1     REG32(EXTI_BASE + 0x80)

/* ---- TIM2 (32-bit general-purpose timer) ---- */
#define TIM2_BASE     0x40000000UL
#define TIM2_CR1      REG32(TIM2_BASE + 0x00)
#define TIM2_EGR      REG32(TIM2_BASE + 0x14)
#define TIM2_CCMR1    REG32(TIM2_BASE + 0x18)
#define TIM2_CCER     REG32(TIM2_BASE + 0x20)
#define TIM2_CNT      REG32(TIM2_BASE + 0x24)
#define TIM2_PSC      REG32(TIM2_BASE + 0x28)
#define TIM2_ARR      REG32(TIM2_BASE + 0x2C)
#define TIM2_CCR1     REG32(TIM2_BASE + 0x34)

/* ---- ADC ---- */
#define ADC_BASE      0x40012400UL
#define ADC_ISR       REG32(ADC_BASE + 0x00)   /* ADRDY(0) EOC(2) CCRDY(13) */
#define ADC_CR        REG32(ADC_BASE + 0x08)   /* ADEN(0) ADSTART(2) ADVREGEN(28) ADCAL(31) */
#define ADC_CFGR2     REG32(ADC_BASE + 0x10)
#define ADC_SMPR      REG32(ADC_BASE + 0x14)
#define ADC_CHSELR    REG32(ADC_BASE + 0x28)
#define ADC_DR        REG32(ADC_BASE + 0x40)

/* ---- USART2 (ST-LINK Virtual COM Port) ---- */
#define USART2_BASE   0x40004400UL
#define USART2_CR1    REG32(USART2_BASE + 0x00)
#define USART2_BRR    REG32(USART2_BASE + 0x0C)
#define USART2_ISR    REG32(USART2_BASE + 0x1C)   /* RXNE(5) TXE(7) */
#define USART2_RDR    REG32(USART2_BASE + 0x24)
#define USART2_TDR    REG32(USART2_BASE + 0x28)

/* ---- SPI1 ---- */
#define SPI1_BASE     0x40013000UL
#define SPI1_CR1      REG32(SPI1_BASE + 0x00)
#define SPI1_CR2      REG32(SPI1_BASE + 0x04)
#define SPI1_SR       REG32(SPI1_BASE + 0x08)    /* TXE(1) BSY(7) */
#define SPI1_DR8      (*(volatile uint8_t *)(SPI1_BASE + 0x0C))

/* ---- I2C1 ---- */
#define I2C1_BASE     0x40005400UL
#define I2C1_CR1      REG32(I2C1_BASE + 0x00)
#define I2C1_CR2      REG32(I2C1_BASE + 0x04)
#define I2C1_TIMINGR  REG32(I2C1_BASE + 0x10)
#define I2C1_ISR      REG32(I2C1_BASE + 0x18)
#define I2C1_ICR      REG32(I2C1_BASE + 0x1C)
#define I2C1_RXDR     REG32(I2C1_BASE + 0x24)
#define I2C1_TXDR     REG32(I2C1_BASE + 0x28)

#endif
