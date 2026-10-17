/* Liste 8-1  k08_proje_oled_voltmetre.c - Bolum 8 projesi
 * Potansiyometre gerilimi OLED'de: buyuk sayi (V), cubuk grafik ve min/max.
 */
#include "g071.h"
#include <string.h>

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

/* ---- OLED 128x64, SPI1 uzerinden (Lab 05 ile ayni surucu) ----
 * SCK=SCK/D13(PA5) MOSI=PWM/MOSI/D11(PA7) CS=PWM/CS/D10(PB0)
 * RES=D8(PA9) DC=PWM/D9(PC7) */
#define OLED_SH1106   1      /* SSD1306 modulu icin 0 */
#define CS_LOW()    (GPIOB_BSRR = (1u << (0 + 16)))   /* PB0 = PWM/CS/D10 */
#define CS_HIGH()   (GPIOB_BSRR = (1u <<  0))
#define RES_LOW()   (GPIOA_BSRR = (1u << (9 + 16)))
#define RES_HIGH()  (GPIOA_BSRR = (1u <<  9))
#define DC_LOW()    (GPIOC_BSRR = (1u << (7 + 16)))
#define DC_HIGH()   (GPIOC_BSRR = (1u <<  7))

void spi_tx(uint8_t b)
{
    while (!(SPI1_SR & (1u << 1))) { }     /* TXE */
    SPI1_DR8 = b;
}
void spi_flush(void) { while (SPI1_SR & (1u << 7)) { } }   /* BSY */

void oled_hw_init(void)
{
    RCC_IOPENR  |= (1u << 0) | (1u << 1) | (1u << 2);  /* GPIOA, GPIOB, GPIOC */
    RCC_APBENR2 |= (1u << 12);             /* SPI1 */

    /* PA5 AF(10), PA7 AF(10), PA9 out(01) */
    uint32_t m = GPIOA_MODER;
    m &= ~((3u<<(5*2))|(3u<<(7*2))|(3u<<(9*2)));
    m |=  ((2u<<(5*2))|(2u<<(7*2))|(1u<<(9*2)));
    GPIOA_MODER = m;

    GPIOB_MODER &= ~(3u<<(0*2));
    GPIOB_MODER |=  (1u<<(0*2));            /* PB0 out (CS) */

    /* PA5, PA7 -> AF0 (SPI1_SCK / SPI1_MOSI on STM32G0) */
    GPIOA_AFRL &= ~((0xFu<<(5*4))|(0xFu<<(7*4)));
    /* AF0 = 0, nothing to set */
    GPIOA_OSPEEDR |= (3u<<(5*2))|(3u<<(7*2));

    GPIOC_MODER &= ~(3u<<(7*2));
    GPIOC_MODER |=  (1u<<(7*2));            /* PC7 out */

    CS_HIGH(); DC_HIGH(); RES_HIGH();

    /* SPI1 master, 8-bit, mode0, MSB first, BR=/16, software NSS */
    SPI1_CR1 = 0;
    SPI1_CR2 = (0x7u << 8) | (1u << 12);               /* DS=8bit, FRXTH */
    SPI1_CR1 = (1u << 2) | (3u << 3) | (1u << 9) | (1u << 8); /* MSTR,BR/16,SSM,SSI */
    SPI1_CR1 |= (1u << 6);                             /* SPE */
}

/* ---------------- OLED driver ---------------- */
#define OLED_W 128
#define OLED_H 64
static uint8_t  buffer[OLED_W * OLED_H / 8];
static uint16_t cx = 0, cy = 0;

void oled_cmd(uint8_t c) { DC_LOW(); CS_LOW(); spi_tx(c); spi_flush(); CS_HIGH(); }

void oled_init(void)
{
    RES_HIGH(); delay_ms(10); RES_LOW(); delay_ms(10); RES_HIGH(); delay_ms(10);
    oled_cmd(0xAE);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0xA1); oled_cmd(0xC8);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0x81); oled_cmd(0xCF);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0xA4); oled_cmd(0xA6); oled_cmd(0xAF);
}

void oled_clear(void) { memset(buffer, 0, sizeof(buffer)); cx = cy = 0; }

void oled_update(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_cmd(0xB0 | page);
#if OLED_SH1106
        oled_cmd(0x02); oled_cmd(0x10);
#else
        oled_cmd(0x00); oled_cmd(0x10);
#endif
        DC_HIGH(); CS_LOW();
        for (uint8_t x = 0; x < OLED_W; x++) spi_tx(buffer[page*OLED_W + x]);
        spi_flush(); CS_HIGH();
    }
}

void oled_pixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= OLED_W || y >= OLED_H) return;
    if (on) buffer[x + (y/8)*OLED_W] |=  (1u << (y & 7));
    else    buffer[x + (y/8)*OLED_W] &= ~(1u << (y & 7));
}
void oled_cursor(uint8_t x, uint8_t y) { cx = x; cy = y; }

static const uint8_t Font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44},
};

void oled_char(char ch)
{
    if (ch < 0x20 || ch > 0x7E) ch = 0x20;
    if (cx + 6 > OLED_W) { cx = 0; cy += 8; }
    const uint8_t *g = Font5x7[ch - 0x20];
    for (uint8_t col = 0; col < 5; col++)
        for (uint8_t row = 0; row < 7; row++)
            oled_pixel(cx + col, cy + row, (g[col] >> row) & 1);
    cx += 6;
}
void oled_print(const char *s) { while (*s) oled_char(*s++); }

void oled_u16(uint16_t v)
{
    char b[6]; int n = 0;
    if (!v) { oled_char('0'); return; }
    while (v) { b[n++] = (char)('0' + v % 10); v /= 10; }
    while (n--) oled_char(b[n]);
}

static void oled_int(int32_t v)
{
    char b[11]; int n = 0;
    uint32_t u = (v < 0) ? (uint32_t)(-(v + 1)) + 1u : (uint32_t)v;
    if (v < 0) oled_char('-');
    do { b[n++] = (char)('0' + u % 10u); u /= 10u; } while (u);
    while (n--) oled_char(b[n]);
}
void oled_fix(int32_t v, uint8_t dec)          /* 2345,2 -> "23.45" */
{
    int32_t p = 1;
    for (uint8_t i = 0; i < dec; i++) p *= 10;
    if (v < 0) { oled_char('-'); v = -v; }
    oled_int(v / p);
    oled_char('.');
    for (p /= 10; p > 0; p /= 10) oled_char((char)('0' + (v / p) % 10));
}
void oled_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t fill)
{
    for (uint8_t i = 0; i < w; i++)
        for (uint8_t j = 0; j < h; j++)
            if (fill || i == 0 || j == 0 || i == w - 1 || j == h - 1)
                oled_pixel(x + i, y + j, 1);
}

int main(void)
{
    systick_init();
    adc_init();
    oled_hw_init();
    oled_init();

    uint32_t vmin = 3300, vmax = 0;
    for (;;) {
        uint32_t mv = (uint32_t)adc_read() * 3300u / 4095u;
        if (mv < vmin) vmin = mv;
        if (mv > vmax) vmax = mv;

        oled_clear();
        oled_cursor(22, 0);  oled_print("OLED VOLTMETRE");
        oled_cursor(40, 16); oled_fix((int32_t)mv, 3); oled_print(" V");
        oled_rect(4, 30, 120, 12, 0);                          /* cerceve        */
        oled_rect(6, 32, (uint8_t)(116u * mv / 3300u), 8, 1);  /* dolu kisim     */
        oled_cursor(0, 50);  oled_print("min "); oled_fix((int32_t)vmin, 2);
        oled_cursor(66, 50); oled_print("max "); oled_fix((int32_t)vmax, 2);
        oled_update();
        delay_ms(100);
    }
}
