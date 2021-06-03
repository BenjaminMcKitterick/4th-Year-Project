#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include "SPI.h"
#include "board.h"
#include "Commissioning.h"
#include "Mcu.h"
#include "utilities.h"
#include "board-config.h"
#include "LoRaMac.h"
#include "Commissioning.h"
#include "rtc-board.h"
#include "delay.h"
#include <U8x8lib.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

enum eDeviceState
{
    STATE_INIT,
    STATE_JOIN,
    STATE_TRANSMIT,
    STATE_CYCLE,
    STATE_SLEEP,
	STATE_DISPLAY
};


#define APP_TX_DUTYCYCLE_RND          1000

class LoRaWanClass{
public:
  void init(DeviceClass_t lorawan_class_mode,LoRaMacRegion_t region);
  void join();
  void send(DeviceClass_t lorawan_class_mode);
  void cycle(uint32_t dutyCycle);
  void sleep(DeviceClass_t lorawan_class_mode,uint8_t debugLevel);
  void wait();
  void displayAck(U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8);
};

extern enum eDeviceState deviceState;
extern uint8_t app_port;
extern uint32_t tx_duty_cycle_time;
extern uint32_t tx_duty_cycle;
extern uint8_t app_data[LORAWAN_APP_DATA_MAX_SIZE];
extern uint8_t data_size;
extern LoRaMacRegion_t lorawan_region;
extern uint8_t conf_num_trials;
extern DeviceClass_t  lorawan_class_mode;
extern bool isTxConfirmed;
extern uint8_t dev_eui[];
extern uint8_t app_eui[];
extern uint8_t app_key[];
extern uint8_t nwk_s_key[];
extern uint8_t app_s_key[];
extern uint32_t dev_address;
extern uint16_t channel_mask[6];
extern int ackrssi;
extern int acksnr;
extern int ackdr;

extern LoRaWanClass LoRaWAN;

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
