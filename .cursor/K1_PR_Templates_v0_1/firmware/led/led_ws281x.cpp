// firmware/led/led_ws281x.cpp (PR TEMPLATE)
#include "led_ws281x.h"
bool LedDriver::begin(int pin0, int pin1, uint16_t edgeLen){ return true; }
void LedDriver::start_frame(){}
void LedDriver::set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b){}
void LedDriver::submit(){}
void LedDriver::wait_done(){}
