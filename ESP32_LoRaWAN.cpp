#include <ESP32_LoRaWAN.h>



#if defined( WIFI_LoRa_32 ) || defined( WIFI_LoRa_32_V2 ) || defined( Wireless_Stick )
SSD1306  Display(0x3c, SDA_OLED, SCL_OLED, RST_OLED);
#endif


#ifdef REGION_EU868
#include "region/RegionEU868.h"
#endif

#ifdef REGION_EU433
#include "region/RegionEU433.h"
#endif
/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_4

/*!
 * User application data size
 */
uint8_t data_size = 4;

/*!
 * User application data
 */
uint8_t app_data[LORAWAN_APP_DATA_MAX_SIZE];


/*!
 * Defines the application data transmission duty cycle
 */
uint32_t tx_duty_cycle_time ;

//int ackrssi = 0;

//int acksnr = 0;

//int ackdr = 0;

/*!
 * Timer to handle the application data transmission duty cycle
 */
TimerEvent_t TxNextPacketTimer;

bool overTheAirActivation = true;
//bool isTxConfirmed = true;

/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

enum eDeviceState deviceState;

static void lwan_dev_params_update( void );

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
bool SendFrame( void )
{
	McpsReq_t mcpsReq;
	LoRaMacTxInfo_t txInfo;

	lwan_dev_params_update();
	
	if( LoRaMacQueryTxPossible( data_size, &txInfo ) != LORAMAC_STATUS_OK )
	{
		// Send empty frame in order to flush MAC commands
		mcpsReq.Type = MCPS_UNCONFIRMED;
		mcpsReq.Req.Unconfirmed.fBuffer = NULL;
		mcpsReq.Req.Unconfirmed.fBufferSize = 0;
		mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
	}
	else
	{
		if( isTxConfirmed == true )
		{
			lora_printf("confirmed uplink sending ...\r\n");

			mcpsReq.Type = MCPS_CONFIRMED;
			mcpsReq.Req.Confirmed.fPort = app_port;
			mcpsReq.Req.Confirmed.fBuffer = app_data;
			mcpsReq.Req.Confirmed.fBufferSize = data_size;
			mcpsReq.Req.Confirmed.NbTrials = conf_num_trials;
			mcpsReq.Req.Confirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
		}
		else
		{
			lora_printf("unconfirmed uplink sending ...\r\n");

			mcpsReq.Type = MCPS_UNCONFIRMED;
			mcpsReq.Req.Unconfirmed.fPort = app_port;
			mcpsReq.Req.Unconfirmed.fBuffer = app_data;
			mcpsReq.Req.Unconfirmed.fBufferSize = data_size;
			mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
		}
	}
	if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
	{
		return false;
	}
	return true;
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
static void OnTxNextPacketTimerEvent( void )
{
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;

	TimerStop( &TxNextPacketTimer );

	mibReq.Type = MIB_NETWORK_JOINED;
	status = LoRaMacMibGetRequestConfirm( &mibReq );

	if( status == LORAMAC_STATUS_OK )
	{
		if( mibReq.Param.IsNetworkJoined == true )
		{
			deviceState = STATE_TRANSMIT;
			NextTx = true;
		}
		else
		{
			// Network not joined yet. Try to join again
			MlmeReq_t mlmeReq;
			mlmeReq.Type = MLME_JOIN;
			mlmeReq.Req.Join.DevEui = dev_eui;
			mlmeReq.Req.Join.AppEui = app_eui;
			mlmeReq.Req.Join.AppKey = app_key;
			mlmeReq.Req.Join.NbTrials = 1;

			if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
			{
				deviceState = STATE_SLEEP;
			}
			else
			{
				deviceState = STATE_CYCLE;
			}
		}
	}
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
	if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
	{
		switch( mcpsConfirm->McpsRequest )
		{
			case MCPS_UNCONFIRMED:
			{
				// Check Datarate
				// Check TxPower
				break;
			}
			case MCPS_CONFIRMED:
			{
				// Check Datarate
				// Check TxPower
				// Check AckReceived
				// Check NbTrials
				break;
			}
			case MCPS_PROPRIETARY:
			{
				break;
			}
			default:
				break;
		}
	}
	NextTx = true;
}

/*  get the BatteryVoltage in mV. */
uint16_t GetBatteryVoltage(void)
{
	return 0;
}

void __attribute__((weak)) downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
	lora_printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
	lora_printf("+REV DATA:");

	for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
	{
		lora_printf("%02X",mcpsIndication->Buffer[i]);
	}
	lora_printf("\r\n");
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
int ackrssi;
static void McpsIndication( McpsIndication_t *mcpsIndication )
{
	if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK )
	{
		return;
	}
	ackrssi=mcpsIndication->Rssi;
	lora_printf( "Data received: rssi = %d, datarate = %d\r\n", mcpsIndication->Rssi,(int)mcpsIndication->RxDatarate);
	delay(10);
	switch( mcpsIndication->McpsIndication )
	{
		case MCPS_UNCONFIRMED:
		{
			break;
		}
		case MCPS_CONFIRMED:
		{
			break;
		}
		case MCPS_PROPRIETARY:
		{
			break;
		}
		case MCPS_MULTICAST:
		{
			break;
		}
		default:
			break;
	}

	// Check Multicast
	// Check Port
	// Check Datarate
	// Check FramePending
	if( mcpsIndication->FramePending == true )
	{
		deviceState = STATE_DISPLAY;
	}
	// Check Buffer
	// Check BufferSize
	// Check Rssi
	// Check Snr
	// Check RxSlot
	if( mcpsIndication->RxData == true )
	{
		downLinkDataHandle(mcpsIndication);
	}
	deviceState = STATE_DISPLAY;
}

int GetCurrentRssi(void) { return ackrssi; }
int GetCurrentSnr(void) {return acksnr; }
int GetCurrentDr(void) { return ackdr; }


/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
	switch( mlmeConfirm->MlmeRequest )
	{
		case MLME_JOIN:
		{
			if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
			{
				lora_printf("joined\r\n");

				// Status is OK, node has joined the network
				deviceState = STATE_TRANSMIT;
			}
			else
			{
				uint32_t rejoin_delay = 30000;
				lora_printf("join failed, rejoin at %d ms later\r\n",rejoin_delay);
				TimerSetValue( &TxNextPacketTimer, rejoin_delay );
				TimerStart( &TxNextPacketTimer );
			}
			break;
		}
		case MLME_LINK_CHECK:
		{
			if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK )
			{
				// Check DemodMargin
				// Check NbGateways
			}
			break;
		}
		default:
			break;
	}
	NextTx = true;
}

/*!
 * \brief   MLME-Indication event function
 *
 * \param   [IN] mlmeIndication - Pointer to the indication structure.
 */
static void MlmeIndication( MlmeIndication_t *mlmeIndication )
{
	switch( mlmeIndication->MlmeIndication )
	{
		case MLME_SCHEDULE_UPLINK:
		{// The MAC signals that we shall provide an uplink as soon as possible
			OnTxNextPacketTimerEvent( );
			break;
		}
		default:
			break;
	}
}


static void lwan_dev_params_update( void )
{
#ifdef REGION_EU868
	LoRaMacChannelAdd( 3, ( ChannelParams_t )EU868_LC4 );
	LoRaMacChannelAdd( 4, ( ChannelParams_t )EU868_LC5 );
	LoRaMacChannelAdd( 5, ( ChannelParams_t )EU868_LC6 );
	LoRaMacChannelAdd( 6, ( ChannelParams_t )EU868_LC7 );
	LoRaMacChannelAdd( 7, ( ChannelParams_t )EU868_LC8 );
#endif

#ifdef REGION_EU433
		LoRaMacChannelAdd( 3, ( ChannelParams_t )EU433_LC4 );
		LoRaMacChannelAdd( 4, ( ChannelParams_t )EU433_LC5 );
		LoRaMacChannelAdd( 5, ( ChannelParams_t )EU433_LC6 );
		LoRaMacChannelAdd( 6, ( ChannelParams_t )EU433_LC7 );
		LoRaMacChannelAdd( 7, ( ChannelParams_t )EU433_LC8 );
#endif

	MibRequestConfirm_t mibReq;

	mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
	mibReq.Param.ChannelsMask = channel_mask;
	LoRaMacMibSetRequestConfirm(&mibReq);

	mibReq.Type = MIB_CHANNELS_MASK;
	mibReq.Param.ChannelsMask = channel_mask;
	LoRaMacMibSetRequestConfirm(&mibReq);
}


LoRaMacPrimitives_t LoRaMacPrimitive;
LoRaMacCallback_t LoRaMacCallback;

void LoRaWanClass::init(DeviceClass_t lorawan_class_mode,LoRaMacRegion_t region)
{
	if(lorawan_class_mode == CLASS_B)
	{
		Serial.println("Class B is not supported, switch to Class A");
		lorawan_class_mode = CLASS_A;
	}

	MibRequestConfirm_t mibReq;

	LoRaMacPrimitive.MacMcpsConfirm = McpsConfirm;
	LoRaMacPrimitive.MacMcpsIndication = McpsIndication;
	LoRaMacPrimitive.MacMlmeConfirm = MlmeConfirm;
	LoRaMacPrimitive.MacMlmeIndication = MlmeIndication;
	LoRaMacCallback.GetBatteryLevel = BoardGetBatteryLevel;
	LoRaMacCallback.GetTemperatureLevel = NULL;
	LoRaMacInitialization( &LoRaMacPrimitive, &LoRaMacCallback,region);
	TimerStop( &TxNextPacketTimer );
	TimerInit( &TxNextPacketTimer, OnTxNextPacketTimerEvent );

    if(IsLoRaMacNetworkJoined==false)
    {
	  mibReq.Type = MIB_ADR;
	  mibReq.Param.AdrEnable = true;
	  LoRaMacMibSetRequestConfirm( &mibReq );

	  mibReq.Type = MIB_PUBLIC_NETWORK;
	  mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
	  LoRaMacMibSetRequestConfirm( &mibReq );

      mibReq.Type = MIB_DEVICE_CLASS;
      mibReq.Param.Class = lorawan_class_mode;
      LoRaMacMibSetRequestConfirm( &mibReq );


  	  Serial.print("\r\nLoRaWAN ");
  	  switch(region)
  	  {
  	  	  case LORAMAC_REGION_AS923:
  	  		Serial.print("AS923");
  	  		break;
  	  	  case LORAMAC_REGION_AU915:
  	  		Serial.print("AU915");
  	  		break;
  	  	  case LORAMAC_REGION_CN470:
  	  		Serial.print("CN470");
  	  		break;
  	  	  case LORAMAC_REGION_CN779:
  	  		Serial.print("CN779");
  	  		break;
  	  	  case LORAMAC_REGION_EU433:
  	  		Serial.print("EU433");
  	  		break;
  	  	  case LORAMAC_REGION_EU868:
  	  		Serial.print("EU868");
  	  		break;
  	  	  case LORAMAC_REGION_KR920:
  	  		Serial.print("KR920");
  	  		break;
  	  	  case LORAMAC_REGION_IN865:
  	  		Serial.print("IN865");
  	  		break;
  	  	  case LORAMAC_REGION_US915:
  	  		Serial.print("US915");
  	  		break;
  	  	  case LORAMAC_REGION_US915_HYBRID:
  	  		Serial.print("US915_HYBRID ");
  	  		break;
  	  }
  	  Serial.printf(" Class %X start!\r\n\r\n",lorawan_class_mode+10);

  	  lwan_dev_params_update();
  	  deviceState = STATE_JOIN;
    }
    else
    {
  	  deviceState = STATE_TRANSMIT;
    }
}


void LoRaWanClass::join()
{
	if( overTheAirActivation == true )
	{
		Serial.println("joining...");
		MlmeReq_t mlmeReq;

		mlmeReq.Type = MLME_JOIN;

		mlmeReq.Req.Join.DevEui = dev_eui;
		mlmeReq.Req.Join.AppEui = app_eui;
		mlmeReq.Req.Join.AppKey = app_key;
		mlmeReq.Req.Join.NbTrials = 1;

		if( LoRaMacMlmeRequest( &mlmeReq ) == LORAMAC_STATUS_OK )
		{
			deviceState = STATE_SLEEP;
		}
		else
		{
			deviceState = STATE_CYCLE;
		}
	}
	else
	{
		MibRequestConfirm_t mibReq;

		mibReq.Type = MIB_NET_ID;
		mibReq.Param.NetID = LORAWAN_NETWORK_ID;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_DEV_ADDR;
		mibReq.Param.DevAddr = dev_address;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_NWK_SKEY;
		mibReq.Param.NwkSKey = nwk_s_key;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_APP_SKEY;
		mibReq.Param.AppSKey = app_s_key;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_NETWORK_JOINED;
		mibReq.Param.IsNetworkJoined = true;
		LoRaMacMibSetRequestConfirm( &mibReq );

		deviceState = STATE_TRANSMIT;
	}
}

void LoRaWanClass::send(DeviceClass_t lorawan_class_mode)
{
	if( NextTx == true )
	{ 	
		NextTx = SendFrame( );
	}
}

void LoRaWanClass::cycle(uint32_t dutyCycle)
{
	TimerSetValue( &TxNextPacketTimer, dutyCycle );
	TimerStart( &TxNextPacketTimer );
}

void LoRaWanClass::sleep(DeviceClass_t lorawan_class_mode,uint8_t debugLevel)
{
	Radio.IrqProcess( );
	Mcu.sleep(lorawan_class_mode,debugLevel);
}

void LoRaWanClass::displayAck(U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8)
{
	u8x8.clear();
	u8x8.drawString(0, 0, "ACK received");
	u8x8.setCursor(5, 2);
	u8x8.drawString(0, 2, "RSSI: ");
	u8x8.print(ackrssi);
	delay(1000);
	// The server signals that it has pending data to be sent.
	// We schedule an uplink as soon as possible to flush the server.
	OnTxNextPacketTimerEvent( );
}

LoRaWanClass LoRaWAN;

