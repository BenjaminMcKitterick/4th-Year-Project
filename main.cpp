/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "MicroBit.h"
#include "LTC2990.h"

#define DATA_SIZE			1
#define I2C_ADDRESS			0x98

MicroBit uBit;
bool flag;

/**
 * prints double to uBit.serial
 * @param number number to print
 * @param digits number of digits after decimal point
 * @return number chars printed
 */
size_t printFloat(double number, uint8_t digits) {
    size_t n = 0;

    if (isnan(number)) return uBit.serial.printf("nan");
    if (isinf(number)) return uBit.serial.printf("inf");
    if (number > 4294967040.0) return uBit.serial.printf("ovf"); // constant determined empirically
    if (number <-4294967040.0) return uBit.serial.printf("ovf"); // constant determined empirically

    // Handle negative numbers
    if (number < 0.0) {
        n += uBit.serial.printf("-");
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i) {
        rounding /= 10.0;
    }

    number += rounding;

    // Extract the integer part of the number and print it
    unsigned long int_part = (unsigned long) number;
    double remainder = number - (double) int_part;
    n += uBit.serial.printf("%lu", int_part);

    // Print the decimal point, but only if there are digits beyond
    if (digits > 0) {
        n += uBit.serial.printf(".");
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
        remainder *= 10.0;
        int toPrint = int(remainder);
        n += uBit.serial.printf("%d", toPrint);
        remainder -= toPrint;
    }

    return n;
}

const char* byteToBinary(int x)
{
    static char b[9];
    b[0] = '\0';

    for (int i = 128; i > 0; i >>= 1) {
        strcat(b, ((x & i) == i) ? "1" : "0");
    }
    return b;
}

float voltToAmp(float v){
	return v / 0.01;
}

void readVoltage(MicroBitEvent ev){
	LTC2990 volt_monitor(&uBit.i2c, I2C_ADDRESS);
	
	float voltage;

	volt_monitor.init(0xFF);
	volt_monitor.trigger();

	while(true){
		//if(volt_monitor.isReady(V1)){
		voltage = volt_monitor.getVoltageFloat(V2);
		printFloat(voltage, 4);
		uBit.serial.printf("mV, ");
		char test = volt_monitor.readRegister(0x06);
		uBit.serial.printf(" status = %c ", byteToBinary(test));
		uBit.sleep(100);
		//}
	}
}

int main()
{
    // Initialise the micro:bit runtime.
    uBit.init();

	uBit.messageBus.listen(MICROBIT_ID_BUTTON_B, MICROBIT_BUTTON_EVT_CLICK, readVoltage);

    // If main exits, there may still be other fibers running or registered event handlers etc.
    // Simply release this fiber, which will mean we enter the scheduler. Worse case, we then
    // sit in the idle task forever, in a power efficient sleep.
    //release_fiber();
	while(true){
		uBit.sleep(10000);
	}
}


