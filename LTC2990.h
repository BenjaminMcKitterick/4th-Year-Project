
#ifndef LTC2990_H
#define LTC2990_h

#include "MicroBit.h"

#define STATUS 0x00
#define CONTROL 0x01
#define TRIGGER 0x02
 
#define TINT_MSB 0x04
#define TINT_LSB 0x05
 
#define V1_MSB 0x06
#define V1_LSB 0x07
 
#define V2_MSB 0x08
#define V2_LSB 0x09
 
#define V3_MSB 0x0A
#define V3_LSB 0x0B
 
#define V4_MSB 0x0C
#define V4_LSB 0x0D
 
#define VCC_MSB 0x0E
#define VCC_LSB 0x0F
 
#define SINGLE 0.00030518 //LSB for single-ended measurments
#define DIFF 0.00001942 //LSB for diff measurments

enum V_SOURCE {
    V1,
    V2,
    V3,
    V4,
    VCC,
    V1_V2,
    V3_V4,
};

/** LTC2990 class 
 */
class LTC2990 {
public:
    /** Init LTC2990 class
     * @param i2c I2C serial interface
     * @param addr I2C address
     */
    LTC2990 (MicroBitI2C* i2c, char addr); 
 
    /** Init the device through control register
     * @param control control register bits
     */
    void init (char control);
    
    /** Return STATUS register
     */  
    char status();
 
    /** Trigger a conversion
     */
    char trigger();
    
    /** Get voltage
    * @param source voltage to measure and return in float format
    */
    float getVoltageFloat(V_SOURCE source);


	char readRegister(char reg);
    
    /** 
	*	Check the busy bit
    */
    bool isBusy();
    
    /** 
	* 	Check if register is ready to read again
    *@param source register to check (V1, V2, V3, V4, VCC, V1-V2, V3-V4)
    */
    bool isReady(V_SOURCE source);
    
    
protected:
 
 
private:
    char _addr;
    MicroBitI2C *_i2c;
    
};
    
#endif  