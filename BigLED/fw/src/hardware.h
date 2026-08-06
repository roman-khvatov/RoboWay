#pragma once

///////////////// Low level Hardware interface

// Initialize all hardware
void hardware_init();

// Engage Int line - turn it to output and set appropriate Int level
void EnableInterrupt();

// Signal interrupt (turn it on or off)
void SignalInterrupt(bool activate);

// Connect OLED to I2C bus
void TurnOLEDOn();

// Disconnect OLED from I2C bus
void TurnOLEDOff();

/////// Low level Buttons interface

// Return 2 bits of A/B signals of QEncoder
uint8_t QuadEncoderButtons();

// Read raw button state as bitset (13 bits)
uint16_t ReadButtons();
