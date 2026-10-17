/* Liste 10-1  k10_mpu6050_uart.c - MPU6050 ivme ve aci hizi -> seri terminal. */
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

/* ---- MPU6050 (GY-521) ---- */
#define MPU_ADDR 0x68                    /* AD0 -> GND (AD0 -> VCC ise 0x69) */

typedef struct { int16_t ax, ay, az, temp, gx, gy, gz; } mpu_raw_t;

uint8_t mpu_init(void)            /* WHO_AM_I dondurur (orijinal yongada 0x68) */
{
    uint8_t who = 0;
    i2c_read_regs(MPU_ADDR, 0x75, &who, 1);
    i2c_write_reg(MPU_ADDR, 0x6B, 0x00);   /* PWR_MGMT_1: uyandir, ic saat */
    i2c_write_reg(MPU_ADDR, 0x1A, 0x03);   /* CONFIG: sayisal alcak geciren ~44 Hz */
    i2c_write_reg(MPU_ADDR, 0x1B, 0x00);   /* GYRO_CONFIG:  +-250 deg/s */
    i2c_write_reg(MPU_ADDR, 0x1C, 0x00);   /* ACCEL_CONFIG: +-2 g */
    return who;
}

int mpu_read(mpu_raw_t *m)        /* ACCEL_XOUT_H'dan (0x3B) baslayan 14 bayt */
{
    uint8_t b[14];
    if (i2c_read_regs(MPU_ADDR, 0x3B, b, 14)) return -1;
    m->ax   = (int16_t)(b[0]  << 8 | b[1]);
    m->ay   = (int16_t)(b[2]  << 8 | b[3]);
    m->az   = (int16_t)(b[4]  << 8 | b[5]);
    m->temp = (int16_t)(b[6]  << 8 | b[7]);
    m->gx   = (int16_t)(b[8]  << 8 | b[9]);
    m->gy   = (int16_t)(b[10] << 8 | b[11]);
    m->gz   = (int16_t)(b[12] << 8 | b[13]);
    return 0;
}
int32_t mpu_mg(int16_t raw)   { return (int32_t)raw * 1000 / 16384; }  /* +-2 g   */
int32_t mpu_dps10(int16_t raw){ return (int32_t)raw * 10 / 131; }      /* 0.1 dps */
int32_t mpu_temp100(int16_t raw) { return (int32_t)raw * 100 / 340 + 3653; }

int main(void)
{
    systick_init();
    uart_init();
    i2c_init();
    delay_ms(100);                                   /* MPU6050 acilis suresi */

    uint8_t who = mpu_init();
    uart_puts("\r\nWHO_AM_I = 0x"); uart_put_hex8(who); uart_puts("\r\n");

    for (;;) {
        mpu_raw_t m;
        if (mpu_read(&m) == 0) {
            uart_puts("a[mg] ");
            uart_put_int(mpu_mg(m.ax)); uart_puts(" ");
            uart_put_int(mpu_mg(m.ay)); uart_puts(" ");
            uart_put_int(mpu_mg(m.az));
            uart_puts("   g[dps] ");
            uart_put_fix(mpu_dps10(m.gx), 1); uart_puts(" ");
            uart_put_fix(mpu_dps10(m.gy), 1); uart_puts(" ");
            uart_put_fix(mpu_dps10(m.gz), 1);
            uart_puts("   T ");
            uart_put_fix(mpu_temp100(m.temp), 2);
            uart_puts("\r\n");
        }
        delay_ms(200);
    }
}
