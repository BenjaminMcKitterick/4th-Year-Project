
/* Basic application for testing received signal strength indication (RSSI) of ESP32 over LoRaWAN  */

#include <ESP32_LoRaWAN.h>
#include <U8x8lib.h>
#include <U8g2lib.h>
#include "Arduino.h"

/* Code ID for chip */
uint32_t code_id[4] = {0x7E007672,0x7EC4F079,0xF82BB1FF,0xC8455010};

/* Parameteres required for over-the-air activation (OTAA) */
uint8_t dev_eui[] = { 0x53, 0x10, 0x4b, 0x75, 0x46, 0xef, 0x80, 0x1d };
uint8_t app_eui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t app_key[] = { 0x4C, 0x78, 0xAD, 0x96, 0x45, 0x75, 0x15, 0xF7, 0x20, 0x94, 0xBE, 0xB8, 0x23, 0x28, 0xEE, 0x6A };

/* Parameters required for activation by personalization (ABP) */
uint8_t nwk_s_key[] = { 0x70, 0x24, 0x42, 0x5A, 0xA1, 0x01, 0x1E, 0x9F, 0x42, 0x3F, 0x61, 0x79, 0xD5, 0x0C, 0x23, 0x82 };
uint8_t app_s_key[] = { 0xB1, 0x98, 0x40, 0xD5, 0xAF, 0x29, 0xF4, 0x42, 0xB9, 0x4F, 0x01, 0x7A, 0x2D, 0x99, 0xAE, 0xF0 };
uint32_t dev_address =  ( uint32_t )0x00cf9e9f;

uint16_t channel_mask[6] = {0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000};

bool isTxConfirmed = true;

/* Select European LoRaWAN region */
LoRaMacRegion_t lorawan_region = ACTIVE_REGION;

/*LoraWAN device class */
DeviceClass_t  lorawan_class_mode = CLASS_A;

/* transmission duty cycle in ms */
uint32_t tx_duty_cycle = 15000;

/* Number of trials in case no ack received */
uint8_t conf_num_trials = 8;

/* Application port */ 
uint8_t app_port = 2;

uint8_t debugLevel = LoRaWAN_DEBUG_LEVEL;

/* Intialize the dsiplay */
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

static void tx_data_frame( uint8_t port )
{
    data_size = 4;      // max value is 64
    app_data[0] = 0x00;
    app_data[1] = 0x01;
    app_data[2] = 0x02;
    app_data[3] = 0x03;
}

void setup() {
   Serial.begin(115200);
   SPI.begin(SCK,MISO,MOSI,SS);
   u8x8.begin();
   u8x8.setFont(u8x8_font_amstrad_cpc_extended_f);    
   u8x8.clear();
   Mcu.init(SS,RST_LoRa,DIO0,DIO1,code_id);
   deviceState = STATE_INIT;
}

void loop() {
  switch( deviceState )
  {
    case STATE_INIT:
    {
      LoRaWAN.init(lorawan_class_mode, lorawan_region);
      break;
    }
    case STATE_JOIN:
    {
      Serial.println("joining");
      LoRaWAN.join();
      break;
    }
    case STATE_TRANSMIT:
    {
      Serial.println("Sending");
      //u8x8.drawString(0, 0, "STATE: Sending...");
      tx_data_frame(app_port);
      LoRaWAN.send(lorawan_class_mode);
      deviceState = STATE_CYCLE;
      break;
    }
    case STATE_CYCLE:
    {
      Serial.println("cycling");
      // Schedule next packet
      tx_duty_cycle_time = tx_duty_cycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(tx_duty_cycle_time);
      deviceState = STATE_SLEEP;
      break;
    }
    case STATE_SLEEP:
    {
      //u8x8.drawString(0, 0, "STATE: Receiving");
      //u8x8.drawString(0, 2, "rssi: "+ackrssi);
      LoRaWAN.sleep(lorawan_class_mode, lorawan_region);
      break;
    }
    case STATE_DISPLAY:
    {
      LoRaWAN.displayAck(u8x8);
      deviceState = STATE_CYCLE;
    }
    default:
    {
      deviceState = STATE_INIT;
      break;
    }
  }
}
