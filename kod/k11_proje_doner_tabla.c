/* Liste 11-2  k11_proje_doner_tabla.c - Bolum 11 projesi
 * Doner tabla: B1'e her basista motor tam 90 derece doner.
 * Potansiyometre donus hizini ayarlar; seri terminal acisi yazar.
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

/* ---- USART2 115200 8-N-1, PA2/PA3 (ST-LINK sanal COM portu) ---- */
void uart_init(void)
{
    RCC_IOPENR  |= (1u << 0);                        /* GPIOA  */
    RCC_APBENR1 |= (1u << 17);                       /* USART2 */
    GPIOA_MODER &= ~((3u << (2*2)) | (3u << (3*2)));
    GPIOA_MODER |=  ((2u << (2*2)) | (2u << (3*2))); /* PA2, PA3 -> AF */
    GPIOA_AFRL  &= ~((0xFu << (2*4)) | (0xFu << (3*4)));
    GPIOA_AFRL  |=  ((1u << (2*4)) | (1u << (3*4))); /* AF1 = USART2 */
    USART2_BRR = 139;                                /* 16 000 000 / 115 200 */
    USART2_CR1 = (1u << 3) | (1u << 2) | (1u << 0);  /* TE | RE | UE */
}
void uart_putc(char c)
{
    while (!(USART2_ISR & (1u << 7))) { }            /* TXE'yi bekle */
    USART2_TDR = (uint8_t)c;
}
void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
void uart_put_int(int32_t v)                  /* isaretli ondalik sayi */
{
    char b[11]; int n = 0;
    uint32_t u = (v < 0) ? (uint32_t)(-(v + 1)) + 1u : (uint32_t)v;
    if (v < 0) uart_putc('-');
    do { b[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
    while (n--) uart_putc(b[n]);
}
void uart_put_fix(int32_t v, uint8_t dec)     /* 2345,2 -> "23.45" */
{
    int32_t p = 1;
    for (uint8_t i = 0; i < dec; i++) p *= 10;
    if (v < 0) { uart_putc('-'); v = -v; }
    uart_put_int(v / p);
    uart_putc('.');
    for (p /= 10; p > 0; p /= 10) uart_putc((char)('0' + (v / p) % 10));
}
void uart_put_hex8(uint8_t v)
{
    const char *h = "0123456789ABCDEF";
    uart_putc(h[v >> 4]); uart_putc(h[v & 0xF]);
}
int  uart_rx_ready(void) { return (USART2_ISR & (1u << 5)) != 0; }   /* RXNE */
char uart_getc(void)     { return (char)USART2_RDR; }

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

/* ---- USER butonu B1 = PC13, basinca 0 ---- */
void button_init(void)
{
    RCC_IOPENR  |= (1u << 2);                        /* GPIOC saati */
    GPIOC_MODER &= ~(3u << (13 * 2));                /* giris */
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));                /* pull-up */
}
int button_down(void) { return !(GPIOC_IDR & (1u << 13)); }

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

#define STEPS_PER_REV 4096

int main(void)
{
    systick_init();
    uart_init();
    adc_init();
    button_init();
    motor_init();
    uart_puts("\r\nDoner tabla: B1 = +90 derece\r\n");

    uint8_t  idx = 0;
    int32_t  pos = 0;                                /* toplam adim */
    int32_t  target = 0;
    uint32_t next = 0;

    for (;;) {
        if (button_down()) {
            delay_ms(20);
            if (button_down()) {
                target += STEPS_PER_REV / 4;         /* 90 derece = 1024 adim */
                while (button_down()) { }
            }
        }
        if (pos != target && (int32_t)(millis() - next) >= 0) {
            uint32_t ms = 1 + adc_read() / 1024u;    /* 1..4 ms/adim */
            next = millis() + ms;
            idx = (uint8_t)((idx + 1) & 7);
            motor_phase(HALF_STEP[idx]);
            pos++;
            if (pos == target) {
                motor_phase(0);
                uart_puts("aci = ");
                uart_put_int((pos % STEPS_PER_REV) * 360 / STEPS_PER_REV);
                uart_puts(" derece\r\n");
            }
        }
    }
}
