/* Liste 12-1  k12_proje_havalandirma.c - Bolum 12 bitirme projesi
 * Akilli havalandirma kapagi:
 *   - BME280 sicakligi olcer (I2C)
 *   - potansiyometre esik sicakligini ayarlar: 20.00 .. 35.00 C (ADC)
 *   - sicaklik esik+0.5'i asinca step motor kapagi 90 derece acar,
 *     esik-0.5'in altina inince kapatir (histerezis)
 *   - B1 (EXTI kesmesi) OTOMATIK / MANUEL mod arasinda gecer; manuelde kapak
 *     her basista ac/kapa yapar
 *   - durum OLED'de (SPI) ve seri terminalde (UART)
 * Ana dongu hic beklemez: her gorev millis() ile kendi zamaninda calisir.
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

/* ---- I2C1 master, 100 kHz, SCL/D15 = PB8, SDA/D14 = PB9 (AF6) ---- */
#define I2C_TIMEOUT 100000u

void i2c_init(void)
{
    RCC_IOPENR  |= (1u << 1);                        /* GPIOB */
    RCC_APBENR1 |= (1u << 21);                       /* I2C1  */
    GPIOB_MODER  &= ~((3u << (8*2)) | (3u << (9*2)));
    GPIOB_MODER  |=  ((2u << (8*2)) | (2u << (9*2)));/* AF modu           */
    GPIOB_OTYPER |=  (1u << 8) | (1u << 9);          /* acik-akac         */
    GPIOB_PUPDR  &= ~((3u << (8*2)) | (3u << (9*2)));
    GPIOB_PUPDR  |=  ((1u << (8*2)) | (1u << (9*2)));/* zayif pull-up     */
    GPIOB_AFRH   &= ~((0xFu << 0) | (0xFu << 4));
    GPIOB_AFRH   |=  ((6u << 0) | (6u << 4));        /* AF6 = I2C1        */

    I2C1_CR1     = 0;                                /* ayar yaparken PE = 0 */
    I2C1_TIMINGR = 0x30420F13;                       /* 100 kHz @ 16 MHz (RM0444) */
    I2C1_CR1     = (1u << 0);                        /* PE = 1 */
}

/* bir ISR bayragini bekle; -1 = slave NACK verdi, -2 = zaman asimi */
int i2c_wait(uint32_t flag)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C1_ISR & flag)) {
        if (I2C1_ISR & (1u << 4)) {                  /* NACKF */
            while (!(I2C1_ISR & (1u << 5)) && --t) { }   /* STOP'u donanim gonderir */
            I2C1_ICR = (1u << 4) | (1u << 5);        /* NACKF ve STOPF'yi temizle */
            return -1;
        }
        if (--t == 0) return -2;
    }
    return 0;
}

/* yalnizca adres: addr'de bir cihaz ACK verirse 1 dondurur */
int i2c_probe(uint8_t addr)
{
    I2C1_CR2 = ((uint32_t)addr << 1) | (1u << 25) | (1u << 13);  /* NBYTES=0, AUTOEND, START */
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C1_ISR & (1u << 5)) && --t) { }       /* STOPF'yi bekle */
    int ack = !(I2C1_ISR & (1u << 4));
    I2C1_ICR = (1u << 4) | (1u << 5);
    return ack;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    I2C1_CR2 = ((uint32_t)addr << 1) | (2u << 16) | (1u << 25) | (1u << 13);
    if (i2c_wait(1u << 1)) return -1;      /* TXIS */
    I2C1_TXDR = reg;
    if (i2c_wait(1u << 1)) return -1;
    I2C1_TXDR = val;
    if (i2c_wait(1u << 5)) return -1;                        /* STOPF */
    I2C1_ICR = (1u << 5);
    return 0;
}

int i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n)
{
    I2C1_CR2 = ((uint32_t)addr << 1) | (1u << 16) | (1u << 13);   /* 1 bayt yaz, STOP yok */
    if (i2c_wait(1u << 1)) return -1;           /* TXIS */
    I2C1_TXDR = reg;
    if (i2c_wait(1u << 6)) return -1;                             /* TC: tekrar START'a hazir */
    I2C1_CR2 = ((uint32_t)addr << 1) | (1u << 10) | ((uint32_t)n << 16)
             | (1u << 25) | (1u << 13);                           /* n bayt oku, AUTOEND, START */
    for (uint8_t i = 0; i < n; i++) {
        if (i2c_wait(1u << 2)) return -1;                         /* RXNE */
        buf[i] = (uint8_t)I2C1_RXDR;
    }
    if (i2c_wait(1u << 5)) return -1;                             /* STOPF */
    I2C1_ICR = (1u << 5);
    return 0;
}

/* ---- BME280 (Bosch) - datasheet'teki tamsayi kompanzasyon formulleri ---- */
#define BME280_ADDR 0x76                 /* SDO -> GND; SDO -> VCC ise 0x77 */

static uint16_t dig_T1, dig_P1;
static int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;
static int32_t  t_fine;

uint8_t bme280_init(void)         /* kimlik dondurur: 0x60 BME280, 0x58 BMP280 */
{
    uint8_t id = 0, c[26], h[7];
    if (i2c_read_regs(BME280_ADDR, 0xD0, &id, 1)) return 0;
    i2c_write_reg(BME280_ADDR, 0xE0, 0xB6);           /* yazilimla sifirla */
    delay_ms(10);

    i2c_read_regs(BME280_ADDR, 0x88, c, 26);         /* kalibrasyon T1..P9, H1 */
    dig_T1 = (uint16_t)(c[1] << 8 | c[0]);
    dig_T2 = (int16_t)(c[3] << 8 | c[2]);
    dig_T3 = (int16_t)(c[5] << 8 | c[4]);
    dig_P1 = (uint16_t)(c[7] << 8 | c[6]);
    dig_P2 = (int16_t)(c[9] << 8 | c[8]);
    dig_P3 = (int16_t)(c[11] << 8 | c[10]);
    dig_P4 = (int16_t)(c[13] << 8 | c[12]);
    dig_P5 = (int16_t)(c[15] << 8 | c[14]);
    dig_P6 = (int16_t)(c[17] << 8 | c[16]);
    dig_P7 = (int16_t)(c[19] << 8 | c[18]);
    dig_P8 = (int16_t)(c[21] << 8 | c[20]);
    dig_P9 = (int16_t)(c[23] << 8 | c[22]);
    dig_H1 = c[25];                                  /* 0xA1 */
    if (id == 0x60) {
        i2c_read_regs(BME280_ADDR, 0xE1, h, 7);      /* kalibrasyon H2..H6 */
        dig_H2 = (int16_t)(h[1] << 8 | h[0]);
        dig_H3 = h[2];
        dig_H4 = (int16_t)(((int16_t)(int8_t)h[3] * 16) | (h[4] & 0x0F));
        dig_H5 = (int16_t)(((int16_t)(int8_t)h[5] * 16) | (h[4] >> 4));
        dig_H6 = (int8_t)h[6];
        i2c_write_reg(BME280_ADDR, 0xF2, 0x01);      /* nem x1 (0xF4'ten ONCE!) */
    }
    i2c_write_reg(BME280_ADDR, 0xF5, 0xA0);          /* 1 s bekleme, filtre kapali */
    i2c_write_reg(BME280_ADDR, 0xF4, 0x27);          /* sicaklik x1, basinc x1, normal mod */
    return id;
}

int32_t bme280_temp(int32_t adc_T)            /* 0.01 degC */
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)dig_T1) * ((adc_T >> 4) - (int32_t)dig_T1)) >> 12)
                  * (int32_t)dig_T3) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

uint32_t bme280_press(int32_t adc_P)          /* Pa */
{
    int64_t v1 = (int64_t)t_fine - 128000;
    int64_t v2 = v1 * v1 * (int64_t)dig_P6;
    v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
    v2 = v2 + ((int64_t)dig_P4 << 35);
    v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
    v1 = ((((int64_t)1 << 47) + v1) * (int64_t)dig_P1) >> 33;
    if (v1 == 0) return 0;                           /* sifira bolmeyi onle */
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - v2) * 3125) / v1;
    v1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    v2 = ((int64_t)dig_P8 * p) >> 19;
    p = ((p + v1 + v2) >> 8) + ((int64_t)dig_P7 << 4);
    return (uint32_t)(p >> 8);                       /* Q24.8 -> Pa */
}

uint32_t bme280_hum(int32_t adc_H)            /* 0.01 %RH */
{
    int32_t v = t_fine - 76800;
    v = (((((adc_H << 14) - ((int32_t)dig_H4 << 20) - ((int32_t)dig_H5 * v)) + 16384) >> 15)
         * (((((((v * (int32_t)dig_H6) >> 10) * (((v * (int32_t)dig_H3) >> 11) + 32768)) >> 10)
              + 2097152) * (int32_t)dig_H2 + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * (int32_t)dig_H1) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (((uint32_t)v >> 12) * 100u) >> 10;       /* Q22.10 %RH -> 0.01 %RH */
}

/* bir olcum: T 0.01 C, P Pa, H 0.01 %RH biriminde */
int bme280_read(int32_t *t, uint32_t *p, uint32_t *h)
{
    uint8_t d[8];
    if (i2c_read_regs(BME280_ADDR, 0xF7, d, 8)) return -1;
    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    int32_t adc_H = ((int32_t)d[6] << 8) | d[7];
    *t = bme280_temp(adc_T);                         /* once bu: t_fine'i hesaplar */
    *p = bme280_press(adc_P);
    *h = bme280_hum(adc_H);
    return 0;
}

/* ---- OLED 128x64, SPI1 uzerinden (Bolum 8 surucusu) ----
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

    /* PA5 AF(10), PA7 AF(10), PA9 cikis(01) */
    uint32_t m = GPIOA_MODER;
    m &= ~((3u<<(5*2))|(3u<<(7*2))|(3u<<(9*2)));
    m |=  ((2u<<(5*2))|(2u<<(7*2))|(1u<<(9*2)));
    GPIOA_MODER = m;

    GPIOB_MODER &= ~(3u<<(0*2));
    GPIOB_MODER |=  (1u<<(0*2));            /* PB0 cikis (CS) */

    /* PA5, PA7 -> AF0 (STM32G0'da SPI1_SCK / SPI1_MOSI) */
    GPIOA_AFRL &= ~((0xFu<<(5*4))|(0xFu<<(7*4)));
    /* AF0 = 0, ayrica yazmaya gerek yok */
    GPIOA_OSPEEDR |= (3u<<(5*2))|(3u<<(7*2));

    GPIOC_MODER &= ~(3u<<(7*2));
    GPIOC_MODER |=  (1u<<(7*2));            /* PC7 cikis */

    CS_HIGH(); DC_HIGH(); RES_HIGH();

    /* SPI1 master, 8 bit, mod 0, once MSB, BR=/16, yazilim NSS */
    SPI1_CR1 = 0;
    SPI1_CR2 = (0x7u << 8) | (1u << 12);               /* DS=8 bit, FRXTH */
    SPI1_CR1 = (1u << 2) | (3u << 3) | (1u << 9) | (1u << 8); /* MSTR,BR/16,SSM,SSI */
    SPI1_CR1 |= (1u << 6);                             /* SPE */
}

/* ---------------- OLED surucusu ---------------- */
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

#define OPEN_STEPS 1024                              /* 90 derece */

static volatile uint8_t manual = 0, manual_open = 0;

void EXTI4_15_IRQHandler(void)                       /* B1 */
{
    static uint32_t last = 0;
    if (EXTI_FPR1 & (1u << 13)) {
        EXTI_FPR1 = (1u << 13);
        if (ms_ticks - last < 250) return;           /* debounce */
        last = ms_ticks;
        if (!manual) { manual = 1; manual_open = 0; }
        else if (!manual_open) manual_open = 1;
        else manual = 0;                             /* ucuncu basis: otomatige don */
    }
}

static void button_irq_init(void)
{
    RCC_IOPENR  |= (1u << 2);
    GPIOC_MODER &= ~(3u << (13 * 2));
    GPIOC_PUPDR &= ~(3u << (13 * 2));
    GPIOC_PUPDR |=  (1u << (13 * 2));
    EXTI_EXTICR4 &= ~(0xFFu << 8);
    EXTI_EXTICR4 |=  (2u << 8);
    EXTI_FTSR1   |=  (1u << 13);
    EXTI_IMR1    |=  (1u << 13);
    NVIC_ISER     =  (1u << EXTI4_15_IRQn);
}

int main(void)
{
    systick_init();
    uart_init();
    adc_init();
    i2c_init();
    oled_hw_init();
    oled_init();
    motor_init();
    button_irq_init();
    delay_ms(5);
    bme280_init();
    uart_puts("\r\nAkilli havalandirma - zaman;T;esik;kapak\r\n");

    int32_t  temp = 0, thr = 2500, pos = 0, target = 0;
    uint8_t  idx = 0, want_open = 0;
    uint32_t t_step = 0, t_sense = 0;

    for (;;) {
        uint32_t now = millis();

        /* GOREV 1 (her 2 ms): kapagi hedefe dogru bir adim yurut */
        if (pos != target && (int32_t)(now - t_step) >= 0) {
            t_step = now + 2;
            int dir = (target > pos) ? 1 : -1;
            idx = (uint8_t)((idx + dir) & 7);
            motor_phase(HALF_STEP[idx]);
            pos += dir;
            if (pos == target) motor_phase(0);
        }

        /* GOREV 2 (her 500 ms): olc, karar ver, goster */
        if ((int32_t)(now - t_sense) >= 0) {
            t_sense = now + 500;
            uint32_t p, h;
            bme280_read(&temp, &p, &h);
            thr = 2000 + (int32_t)adc_read() * 1500 / 4095;          /* 20.00..35.00 C */

            if (manual)                     want_open = manual_open;
            else if (temp > thr + 50)       want_open = 1;           /* histerezis */
            else if (temp < thr - 50)       want_open = 0;
            target = want_open ? OPEN_STEPS : 0;

            oled_clear();
            oled_cursor(4, 0);  oled_print("AKILLI HAVALANDIRMA");
            oled_cursor(0, 16); oled_print("Sicaklik "); oled_fix(temp, 2); oled_print(" C");
            oled_cursor(0, 28); oled_print("Esik     "); oled_fix(thr, 2);  oled_print(" C");
            oled_cursor(0, 40); oled_print("Kapak    "); oled_print(want_open ? "ACIK" : "KAPALI");
            oled_cursor(0, 54); oled_print(manual ? "MOD: MANUEL" : "MOD: OTOMATIK");
            oled_rect(100, 40, 26, 8, 0);
            oled_rect(101, 41, (uint8_t)(24 * pos / OPEN_STEPS), 6, 1);   /* kapak konumu */
            oled_update();

            uart_put_int((int32_t)(now / 1000)); uart_puts(";");
            uart_put_fix(temp, 2); uart_puts(";");
            uart_put_fix(thr, 2);  uart_puts(";");
            uart_puts(want_open ? "ACIK\r\n" : "KAPALI\r\n");
        }
    }
}
