/* Liste 9-2  k09_bme280_uart.c - BME280: sicaklik, basinc, nem -> seri terminal. */
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

int main(void)
{
    systick_init();
    uart_init();
    i2c_init();
    delay_ms(5);                                     /* sensorun acilisi icin */

    uint8_t id = bme280_init();
    uart_puts("\r\nchip id = 0x"); uart_put_hex8(id);
    if (id == 0x60)      uart_puts("  BME280\r\n");
    else if (id == 0x58) uart_puts("  BMP280 (nem olcmez!)\r\n");
    else                 uart_puts("  bulunamadi: kablolari ve adresi kontrol edin\r\n");

    for (;;) {
        int32_t t; uint32_t p, h;
        if (bme280_read(&t, &p, &h) == 0) {
            uart_puts("T = ");  uart_put_fix(t, 2);              uart_puts(" C   ");
            uart_puts("P = ");  uart_put_fix((int32_t)p, 2);     uart_puts(" hPa   ");
            if (id == 0x60) { uart_puts("RH = "); uart_put_fix((int32_t)h, 2); uart_puts(" %"); }
            uart_puts("\r\n");
        } else {
            uart_puts("okuma hatasi\r\n");
        }
        delay_ms(1000);
    }
}
