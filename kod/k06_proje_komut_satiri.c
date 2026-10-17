/* Liste 6-3  k06_proje_komut_satiri.c - Bolum 6 projesi
 * Basit bir komut yorumlayici. Enter ile biten satirlari anlar:
 *   help        komut listesi
 *   led on|off  LD4'u yak / sondur
 *   pot         potansiyometreyi mV olarak yaz
 *   blink N     LD4'u N kez yakip sondur
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

static int str_eq(const char *a, const char *b)       /* string.h olmadan esitlik */
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static int starts_with(const char *s, const char *p)
{
    while (*p) if (*s++ != *p++) return 0;
    return 1;
}
static int32_t to_int(const char *s)
{
    int32_t v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}
static void led(int on) { GPIOA_BSRR = on ? (1u << 5) : (1u << (5 + 16)); }

static void run(const char *cmd)
{
    if (str_eq(cmd, "help")) {
        uart_puts("komutlar: help, led on, led off, pot, blink N\r\n");
    } else if (str_eq(cmd, "led on")) {
        led(1); uart_puts("tamam\r\n");
    } else if (str_eq(cmd, "led off")) {
        led(0); uart_puts("tamam\r\n");
    } else if (str_eq(cmd, "pot")) {
        uint32_t mv = (uint32_t)adc_read() * 3300u / 4095u;
        uart_puts("pot = "); uart_put_int((int32_t)mv); uart_puts(" mV\r\n");
    } else if (starts_with(cmd, "blink ")) {
        int32_t n = to_int(cmd + 6);
        for (int32_t i = 0; i < n; i++) { led(1); delay_ms(150); led(0); delay_ms(150); }
        uart_puts("tamam\r\n");
    } else if (cmd[0] != 0) {
        uart_puts("bilinmeyen komut, 'help' yazin\r\n");
    }
}

int main(void)
{
    systick_init();
    uart_init();
    adc_init();
    RCC_IOPENR  |= (1u << 0);
    GPIOA_MODER &= ~(3u << (5 * 2));
    GPIOA_MODER |=  (1u << (5 * 2));

    char line[32];
    uint8_t n = 0;
    uart_puts("\r\nCOME327 komut satiri - 'help' yazin\r\n> ");
    for (;;) {
        if (!uart_rx_ready()) continue;
        char c = uart_getc();
        if (c == '\r' || c == '\n') {                  /* satir bitti */
            uart_puts("\r\n");
            line[n] = 0;
            run(line);
            n = 0;
            uart_puts("> ");
        } else if ((c == 8 || c == 127) && n > 0) {    /* geri silme */
            n--; uart_puts("\b \b");
        } else if (c >= ' ' && n < sizeof(line) - 1) {
            line[n++] = c;
            uart_putc(c);                              /* yanki */
        }
    }
}
