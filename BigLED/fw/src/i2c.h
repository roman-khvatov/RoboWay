#pragma once

bool I2CAcceptedAddress();
bool isI2CRead();

bool I2CWriteReady();
bool I2CReadReady();
bool I2CAborted();

uint8_t I2CReadData();
void I2CWriteData(uint8_t data);

void I2CInit(); // Placed in hardware.h
