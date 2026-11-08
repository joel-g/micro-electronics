#pragma once
#include <cstddef>
#include <cstdint>
class Print {
public:
    virtual ~Print()=default;
    virtual size_t write(uint8_t)=0;
    size_t write(const uint8_t *data,size_t size) {size_t n=0;while(size--) n+=write(*data++);return n;}
    size_t print(const char *s) {size_t n=0;while(*s) n+=write(uint8_t(*s++));return n;}
};
