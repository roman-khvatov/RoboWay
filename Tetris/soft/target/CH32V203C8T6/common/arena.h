#pragma once

union Arena {
    struct {
        uint16_t body[512]; // 8*16 pixels of snake body
    } snake;
};

extern Arena arena;
