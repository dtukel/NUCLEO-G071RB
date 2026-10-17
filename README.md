# Mikrodenetleyici Programlama: STM32 NUCLEO-G071RB Uygulamaları

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23438009.svg)](https://doi.org/10.5281/zenodo.23438009)

*"Mikrodenetleyici Programlama: STM32 NUCLEO-G071RB Uygulamaları LED'den bitirme
projesine"* kitabının kaynak kodları.

📘 **Kitap (PDF, Zenodo):** https://doi.org/10.5281/zenodo.23438009

Hazırlayan: Dr. Öğr. Üyesi Dilek (Bilgin) Tükel
([ORCID 0000-0001-5288-5996](https://orcid.org/0000-0001-5288-5996))

- Kart: **STM32 NUCLEO-G071RB** (STM32G071RB, Arm Cortex-M0+)
- Geliştirme ortamı: **STM32CubeIDE** (Empty Project)
- Yöntem: HAL ve CubeMX kullanmadan, **register düzeyinde C**
- Saat: reset sonrası varsayılan HSI16 = 16 MHz

Tüm programlar STM32CubeIDE 2.2.0 ile gelen `arm-none-eabi-gcc` derleyicisiyle
`-Wall -Wextra` seçenekleriyle uyarısız derlenmiştir.

*Source code of the Turkish book "Mikrodenetleyici Programlama: STM32 NUCLEO-G071RB
Uygulamaları" (DOI [10.5281/zenodo.23438009](https://doi.org/10.5281/zenodo.23438009)):
bare-metal, register-level C for the STM32 NUCLEO-G071RB board, no HAL. Comments are in
Turkish (written without Turkish characters).*

## Kullanım

1. STM32CubeIDE'de **File > New > STM32 Project** seçin, *Board Selector*'da
   **NUCLEO-G071RB**'yi bulun, *Targeted Project Type* olarak **Empty** seçin.
2. [`kod/g071.h`](kod/g071.h) dosyasını projenin `Inc/` klasörüne kopyalayın
   (Bölüm 3 ve sonrasındaki programlar bu dosyayı kullanır).
3. `Src/main.c` dosyasının içeriğini silip istediğiniz programın içeriğini yapıştırın.
4. **Ctrl+B** ile derleyin, **Run** ile karta yükleyin.

UART kullanan programların çıktısını görmek için ST-LINK sanal COM portunu
**115200 8-N-1** ayarıyla bir seri terminalde açın.

## Programlar

| Bölüm | Dosya | Konu |
|---|---|---|
| 1 | [k01_blink.c](kod/k01_blink.c) | LD4'ü yakıp söndürmek (ilk program) |
| 2 | [k02_kayan_isik.c](kod/k02_kayan_isik.c) | Üç LED'de kayan ışık |
| 2 | [k02_buton_led.c](kod/k02_buton_led.c) | B1 butonuyla LD4 |
| 2 | [k02_toggle_debounce.c](kod/k02_toggle_debounce.c) | Sıçramasız (debounce) aç/kapa |
| 2 | [k02_proje_isik_modlari.c](kod/k02_proje_isik_modlari.c) | **Proje:** butonla değişen ışık modları |
| 3 | [k03_systick_blink.c](kod/k03_systick_blink.c) | SysTick ile doğru zamanlama |
| 3 | [k03_buton_kesme.c](kod/k03_buton_kesme.c) | EXTI buton kesmesi |
| 3 | [k03_proje_coklu_gorev.c](kod/k03_proje_coklu_gorev.c) | **Proje:** birbirini beklemeyen LED'ler |
| 4 | [k04_adc_okuma.c](kod/k04_adc_okuma.c) | Potansiyometreyi ADC ile okumak |
| 4 | [k04_proje_led_bar.c](kod/k04_proje_led_bar.c) | **Proje:** histerezisli LED çubuk gösterge |
| 5 | [k05_pwm_sabit.c](kod/k05_pwm_sabit.c) | TIM2 PWM ile parlaklık seviyeleri |
| 5 | [k05_nefes_alan_led.c](kod/k05_nefes_alan_led.c) | Yavaşça parlayıp sönen LED |
| 5 | [k05_proje_ayarli_nefes.c](kod/k05_proje_ayarli_nefes.c) | **Proje:** hızı potansiyometreyle ayarlanan solma efekti |
| 6 | [k06_merhaba.c](kod/k06_merhaba.c) | UART ile yazı ve sayı göndermek |
| 6 | [k06_tek_harf_komut.c](kod/k06_tek_harf_komut.c) | Tek harfli komutlar |
| 6 | [k06_proje_komut_satiri.c](kod/k06_proje_komut_satiri.c) | **Proje:** komut satırı yorumlayıcı |
| 8 | [k08_proje_oled_voltmetre.c](kod/k08_proje_oled_voltmetre.c) | **Proje:** SPI OLED voltmetre |
| 9 | [k09_i2c_tarayici.c](kod/k09_i2c_tarayici.c) | I²C tarayıcı |
| 9 | [k09_bme280_uart.c](kod/k09_bme280_uart.c) | BME280 sıcaklık, basınç, nem |
| 9 | [k09_proje_hava_istasyonu.c](kod/k09_proje_hava_istasyonu.c) | **Proje:** OLED + UART hava istasyonu |
| 10 | [k10_mpu6050_uart.c](kod/k10_mpu6050_uart.c) | MPU6050 ivme ve açısal hız |
| 10 | [k10_proje_su_terazisi.c](kod/k10_proje_su_terazisi.c) | **Proje:** dijital su terazisi |
| 11 | [k11_step_tam_tur.c](kod/k11_step_tam_tur.c) | 28BYJ-48 step motor, bir tur ileri-geri |
| 11 | [k11_proje_doner_tabla.c](kod/k11_proje_doner_tabla.c) | **Proje:** butonla 90° dönen tabla |
| 12 | [k12_proje_havalandirma.c](kod/k12_proje_havalandirma.c) | **Bitirme projesi:** akıllı havalandırma kapağı |

Bölüm 7 (lojik analizör) yalnızca ölçüm içerdiği için ayrı bir programı yoktur.

## Pin haritası

Bağlantılarda kartın üzerinde yazan etiketler kullanılır; parantez içindeki MCU
pini yalnızca kodda geçer.

| Kart etiketi | MCU pini | Kullanım |
|---|---|---|
| A0 | PA0 | potansiyometre (ADC kanal 0) |
| SCK/D13 | PA5 | LD4 (yeşil LED), OLED SCK, TIM2_CH1 PWM |
| MISO/D12 | PA6 | LED 0 |
| PWM/MOSI/D11 | PA7 | LED 1, OLED MOSI |
| PWM/CS/D10 | PB0 | OLED CS |
| PWM/D9 | PC7 | OLED DC |
| D8 | PA9 | OLED RES |
| D7 | PA8 | LED 2 |
| PWM/D3, D4, PWM/D5, PWM/D6 | PB3, PB5, PB4, PB14 | ULN2003 IN1-IN4 |
| SCL/D15, SDA/D14 | PB8, PB9 | I²C1 (BME280, MPU6050) |
| (kart üzerinde) | PA2, PA3 | USART2 -> ST-LINK sanal COM portu |
| (kart üzerinde) | PC13 | B1 USER butonu |

> NUCLEO-G071RB'de **D6 = PB14** ve **D10 = PB0**'dır (diğer bazı NUCLEO-64
> kartlarında PB10 ve PB6). Sensör ve ekranları **3V3** ile besleyin; AVDD bir
> besleme pini değildir.

## Malzemeler

NUCLEO-G071RB, breadboard ve jumper kablolar, 5 mm LED ve 220 Ω direnç,
10 kΩ potansiyometre, 1.3" 128x64 OLED (7 pin, SPI, SSD1306/SH1106),
BME280, MPU6050 (GY-521), 28BYJ-48 step motor + ULN2003 sürücü,
8 kanallı USB lojik analizör.

## Kaynaklar

- STMicroelectronics: DS12232 (STM32G071 datasheet), RM0444 (reference manual),
  UM2324 (NUCLEO-64 user manual), PM0223 (Cortex-M0+ programming manual)
- Bosch Sensortec: BME280 datasheet (kompanzasyon formülleri)
- InvenSense: MPU-6000/MPU-6050 product specification ve register map
- Kitabın bölüm yapısı Jeremy Blum'un *Exploring Arduino* (Wiley, 2013)
  kitabındaki öğretim yaklaşımından esinlenmiştir.

## Atıf / Citation

Bu kodları ya da kitabı kullanırsanız lütfen kitaba atıf yapın:

> Tükel, D. (Bilgin). (2026). *Mikrodenetleyici Programlama: STM32 NUCLEO-G071RB
> Uygulamaları LED'den bitirme projesine* (v1). Zenodo.
> https://doi.org/10.5281/zenodo.23438009

```bibtex
@book{tukel2026mikrodenetleyici,
  author    = {Tükel, Dilek (Bilgin)},
  title     = {Mikrodenetleyici Programlama: STM32 NUCLEO-G071RB Uygulamaları LED'den bitirme projesine},
  year      = {2026},
  version   = {v1},
  publisher = {Zenodo},
  language  = {turkish},
  doi       = {10.5281/zenodo.23438009},
  url       = {https://doi.org/10.5281/zenodo.23438009}
}
```

GitHub'daki **"Cite this repository"** düğmesi de [CITATION.cff](CITATION.cff)
dosyasından aynı bilgiyi verir.

## Lisans

- **Kodlar:** [MIT lisansı](LICENSE). Telif bildirimini korumak koşuluyla serbestçe
  kullanabilir, değiştirebilir ve dağıtabilirsiniz.
- **Kitap:** Zenodo'da [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/)
  lisansıyla yayımlanmıştır.
