#pragma once

union Arena {
    struct {
        uint16_t body[512]; // 8*16 pixels of snake body
    } snake;
    struct {
        uint8_t bullets[16];
        uint8_t spsheeps[16];
    } invation;
};

extern Arena arena;
