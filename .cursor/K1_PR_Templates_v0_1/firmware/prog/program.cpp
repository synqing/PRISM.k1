// firmware/prog/program.cpp (PR TEMPLATE)
#include "program.h"
bool Program::load(const uint8_t* blob, size_t len){ return false; }
void Program::set_param(uint16_t id, float v){}
void Program::begin_frame(const Uniforms& u){}
void Program::eval_pixel(uint16_t idx, uint8_t& r, uint8_t& g, uint8_t& b) const{
  r=g=b=0;
}
