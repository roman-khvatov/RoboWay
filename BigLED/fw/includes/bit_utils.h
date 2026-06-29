#pragma once

#include <stdint.h>

inline int total1(uint32_t value) {return __builtin_popcount(value);}
inline int first1(uint32_t value) {return value ? __builtin_ctz(value) : -1;}

class BitsScan {
    uint32_t value;
public:
    BitsScan(uint32_t v) : value(v) {}

    class iterator {
        uint32_t value;
    public:
        iterator(uint32_t v=0) : value(v) {}

        bool operator == (const iterator& other) const {return value == other.value;}
        bool operator != (const iterator& other) const {return value != other.value;}
        uint32_t operator* () const {return first1(value);}

        iterator& operator ++() {value &= value-1; return *this;}
        iterator operator ++(int) {auto result = *this; value &= value-1; return result;}
    };

    iterator begin() {return iterator(value);}
    iterator end() {return iterator();}
};
