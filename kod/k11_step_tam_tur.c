/* Liste 11-1  k11_step_tam_tur.c - motor bir tur ileri, bir tur geri doner. */
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

/* ---- 28BYJ-48 + ULN2003: IN1..IN4 = PWM/D3, D4, PWM/D5, PWM/D6 ---- */
static const uint8_t MOTOR_PIN[4] = {3, 5, 4, 14};   /* PB3, PB5, PB4, PB14 */
static const uint8_t HALF_STEP[8] = {0x8, 0xC, 0x4, 0x6, 0x2, 0x3, 0x1, 0x9};

void motor_init(void)
{
    RCC_IOPENR |= (1u << 1);                         /* GPIOB */
    for (int i = 0; i < 4; i++) {
        GPIOB_MODER &= ~(3u << (MOTOR_PIN[i] * 2));
        GPIOB_MODER |=  (1u << (MOTOR_PIN[i] * 2));  /* cikis */
    }
}
void motor_phase(uint8_t p)                   /* bit3 = IN1 ... bit0 = IN4 */
{
    uint32_t set = 0, clr = 0;
    for (int i = 0; i < 4; i++) {
        if (p & (8u >> i)) set |= 1u << MOTOR_PIN[i];
        else               clr |= 1u << MOTOR_PIN[i];
    }
    GPIOB_BSRR = set | (clr << 16);
}

#define STEPS_PER_REV 4096        /* yarim adim modunda 28BYJ-48: ~4096 adim/tur */

static void rotate(int32_t steps, uint32_t ms_per_step)
{
    static uint8_t idx = 0;
    int dir = (steps >= 0) ? 1 : -1;
    for (int32_t i = 0; i != steps; i += dir) {
        idx = (uint8_t)((idx + dir) & 7);
        motor_phase(HALF_STEP[idx]);
        delay_ms(ms_per_step);
    }
    motor_phase(0);                                 /* bobinleri birak (isinmasin) */
}

int main(void)
{
    systick_init();
    motor_init();
    for (;;) {
        rotate(+STEPS_PER_REV, 1);                  /* saat yonu: ~4 s */
        delay_ms(1000);
        rotate(-STEPS_PER_REV, 1);                  /* ters yon */
        delay_ms(1000);
    }
}
