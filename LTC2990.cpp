#include "MicroBit.h"
#include "LTC2990.h"

LTC2990::LTC2990 (MicroBitI2C* i2c, char addr)
{
    _i2c = i2c;
    _addr = addr;
}

/* Configure LTC2990 by writing the control register */
void LTC2990::init (char cntrl)
{
    char cmd[1];
    cmd[0] = CONTROL;
    cmd[1] = cntrl;
    _i2c->write(_addr,cmd,2);
}    

/* Read status register for new data in polling system */
char LTC2990::status()
{
    char status;
    _i2c->write(_addr,STATUS,1,true);
    _i2c->read(_addr+1,&status,1);
    return status;
}

/* Triggers a conversion - data returned is status register */
char LTC2990::trigger()
{
	char status;
    char cmd[3];
    cmd[0] = TRIGGER;
    cmd[1] = 0x55;
    cmd[2] = 0xAA;
    _i2c->write(_addr,cmd,2);
	_i2c->read(_addr+1, &status, 1);
	return status;
}

char LTC2990::readRegister(char reg){
	char value;
	char cmd[1];
	cmd[0] = STATUS;
	cmd[1] = reg;
	_i2c->read(_addr, &value, 1, true);
	return value;
}

/* Returns voltage in float form after reading respective MSB & LSB registers */
float LTC2990::getVoltageFloat(V_SOURCE source)
{
    char cmd[2];
    char buff[2];
    float factor;
        
    switch (source)
    {
    case V1: cmd[0] = V1_MSB; factor = SINGLE; break;
    case V2: cmd[0] = V2_MSB; factor = SINGLE; break;
    case V3: cmd[0] = V3_MSB; factor = SINGLE; break;
    case V4: cmd[0] = V4_MSB; factor = SINGLE; break;
    case VCC: cmd[0] = VCC_MSB; factor = SINGLE; break;
    case V1_V2: cmd[0] = V1_MSB; factor = DIFF; break;
    case V3_V4: cmd[0] = V2_MSB; factor = DIFF; break;
    default: return 9999;
    }
    
    cmd[1] = cmd[0]+1;
    bool sign;
    short tmp;
    //MSB
    _i2c->write(_addr,&cmd[0],1,true);
    _i2c->read(_addr+1,&buff[0],1); 
    //LSB
    _i2c->write(_addr,&cmd[1],1,true); 
    _i2c->read(_addr+1,&buff[1],1);
    sign = buff[0] & 0x40;
    tmp = ((buff[0] & 0x3F) << 8) | buff[1];
    if (source != VCC)
    {
        if (!sign) return (float)tmp * factor;
        else return ((float)tmp - 16384) * factor;  
    }
    else
    {
        return ((float)tmp * factor) + 2.5;
    }
}

/* Check if busy bit is high */
bool LTC2990::isBusy()
{
    return (status() & 0x01);
}

/* Check the status of operation for all sources */
bool LTC2990::isReady(V_SOURCE source)
{
    switch (source)
    {
        case V1: return (status() & 0x04);
        case V2: return (status() & 0x08);
        case V3: return (status() & 0x10);
        case V4: return (status() & 0x20);
        case VCC: return (status() & 0x40);
        case V1_V2: return (status() & 0x04);
        case V3_V4: return (status() & 0x10);
        default: return 0;
    }      
}
 