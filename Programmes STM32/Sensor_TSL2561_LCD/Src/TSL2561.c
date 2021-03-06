/*
 * TSL2561.c
 *
 *  Created on: 15 f�vr. 2020
 *      Author: YAHMI & ARHAB
 */

/*
    Digital_Light_TSL2561.c
    A library for TSL2561
*/
#include <TSL2561.h>
#include "i2c.h"
#include "usart.h"





uint8_t readRegister( I2C_HandleTypeDef* hi2c, uint8_t address) {

	uint8_t value;

	HAL_I2C_Master_Transmit(hi2c,TSL2561_Address<<1,&address,1,1000);
    HAL_I2C_Master_Receive(hi2c,TSL2561_Address<<1,&value,1,1000);

    return value;
}

void writeRegister( I2C_HandleTypeDef* hi2c, uint8_t address, uint8_t val) {

	uint8_t dataraw[2];

	dataraw[0]=address<<1;
	dataraw[1]=val;

	HAL_I2C_Master_Transmit(hi2c,TSL2561_Address<<1,(uint8_t *)dataraw,2,1000);
}

void getLux(I2C_HandleTypeDef* hi2c) {

	//read bytes from registers
    CH0_LOW = readRegister(hi2c, TSL2561_Channal0L);
    CH0_HIGH = readRegister(hi2c, TSL2561_Channal0H);
    CH1_LOW = readRegister( hi2c, TSL2561_Channal1L);
    CH1_HIGH = readRegister( hi2c, TSL2561_Channal1H);

    ch0 = (CH0_HIGH << 8) | CH0_LOW;
    ch1 = (CH1_HIGH << 8) | CH1_LOW;
}

// initialization of the registers
void init_light(I2C_HandleTypeDef* hi2c) {

	writeRegister( hi2c,TSL2561_Control, 0x03); // POWER UP
    writeRegister( hi2c,TSL2561_Timing, 0x00); //No High Gain (1x), integration time of 13ms
    writeRegister( hi2c,TSL2561_Interrupt, 0x00); //No interruption
    writeRegister( hi2c,TSL2561_Control, 0x00); // POWER Down

}

// read Infrared channel value only, not convert to lux.
uint16_t readIRLuminosity(I2C_HandleTypeDef* hi2c) {

	writeRegister( hi2c,TSL2561_Control, 0x03); // POWER UP

    getLux(hi2c);

    writeRegister( hi2c,TSL2561_Control, 0x00); // POWER Down

    if (ch1 == 0) {
        return 0;
    }

    if (ch0 / ch1 < 2 && ch0 > 4900) {
        return -1;  //ch0 out of range, but ch1 not. the lux is not valid in this situation.
    }
    return ch1;
}

//read Full Spectrum channel value only,  not convert to lux.
uint16_t readFSpecLuminosity(I2C_HandleTypeDef* hi2c) {

    writeRegister(hi2c,TSL2561_Control, 0x03); // POWER UP

    getLux(hi2c);

    writeRegister(hi2c, TSL2561_Control, 0x00); // POWER Down

    if (ch1 == 0) {
        return 0;
    }
    if (ch0 / ch1 < 2 && ch0 > 4900) {
        return -1;  //ch0 out of range, but ch1 not. the lux is not valid in this situation.
    }
    return ch0;
}

//read Visible convert to lux.
signed long readVisibleLux(I2C_HandleTypeDef* hi2c) {

    writeRegister( hi2c,TSL2561_Control, 0x03); // POWER UP

    getLux(hi2c);

    writeRegister( hi2c,TSL2561_Control, 0x00); // POWER Down

    if (ch1 == 0) {
        return 0;
    }
    if (ch0 / ch1 < 2 && ch0 > 4900) {
        return -1;  //ch0 out of range, but ch1 not. the lux is not valid in this situation.
    }
    return calculateLux(0, 0, 0);  //T package, no gain, 13ms
}

// explain in the datasheet
unsigned long calculateLux(unsigned int iGain, unsigned int tInt, int iType) {
    switch (tInt) {
        case 0:  // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case 1: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }
    if (!iGain) {
        chScale = chScale << 4;    // scale 1X to 16X
    }
    // scale the channel values
    channel0 = (ch0 * chScale) >> CH_SCALE;
    channel1 = (ch1 * chScale) >> CH_SCALE;

    ratio1 = 0;
    if (channel0 != 0) {
        ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
    }
    // round the ratio value
    unsigned long ratio = (ratio1 + 1) >> 1;

    switch (iType) {
        case 0: // T package
            if ((ratio >= 0) && (ratio <= K1T)) {
                b = B1T;
                m = M1T;
            } else if (ratio <= K2T) {
                b = B2T;
                m = M2T;
            } else if (ratio <= K3T) {
                b = B3T;
                m = M3T;
            } else if (ratio <= K4T) {
                b = B4T;
                m = M4T;
            } else if (ratio <= K5T) {
                b = B5T;
                m = M5T;
            } else if (ratio <= K6T) {
                b = B6T;
                m = M6T;
            } else if (ratio <= K7T) {
                b = B7T;
                m = M7T;
            } else if (ratio > K8T) {
                b = B8T;
                m = M8T;
            }
            break;
        case 1:// CS package
            if ((ratio >= 0) && (ratio <= K1C)) {
                b = B1C;
                m = M1C;
            } else if (ratio <= K2C) {
                b = B2C;
                m = M2C;
            } else if (ratio <= K3C) {
                b = B3C;
                m = M3C;
            } else if (ratio <= K4C) {
                b = B4C;
                m = M4C;
            } else if (ratio <= K5C) {
                b = B5C;
                m = M5C;
            } else if (ratio <= K6C) {
                b = B6C;
                m = M6C;
            } else if (ratio <= K7C) {
                b = B7C;
                m = M7C;
            }
    }
    temp = ((channel0 * b) - (channel1 * m));
    if (temp < 0) {
        temp = 0;
    }
    temp += (1 << (LUX_SCALE - 1));
    // strip off fractional portion
    lux = temp >> LUX_SCALE;

    return (lux);
}



