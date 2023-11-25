#ifndef PINS_H_
#define PINS_H_

/*
#undef OLED_SDA
#undef OLED_SCL
#undef OLED_RST

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16
*/
#define BUTTON_PIN 38 // The middle button GPIO on the T-Beam

#ifdef TTGO_T_Beam_V0_7
#define GPS_RX 15
#define GPS_TX 12
#endif

#if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_2)
#define GPS_RX 12
#define GPS_TX 34
#endif

#ifdef TTGO_T_Beam_Supreme
#undef OLED_SDA
#undef OLED_SCL
#undef OLED_RST

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 16

#define GPS_RX 9
#define GPS_TX 8

#define LORA_SCK       12
#define LORA_MISO      13
#define LORA_MOSI      11
#define LORA_CS        10
#define RADIO_DIO0_PIN -1
#define LORA_RST       5
#define LORA_IRQ       1
#define RADIO_BUSY_PIN 4
#endif

#endif
