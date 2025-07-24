#pragma once

#include "interface.h"

// Sprite definition. 8 bytes
struct SpriteDef {
    uint32_t pixels;
    uint8_t width:4;
    uint8_t height:4;
    uint8_t group_size:2; // 0 to 3 next sprites forms Group of sprites (switched by 'rotate' parameter)
    uint8_t is_gs:1; // This is GrayScale sprite. Occupied 2 SpriteDef sells (this one for BR of 1, next for BR of 2)
    uint8_t reserved: 5;
    uint16_t reserved2;
};

extern const SpriteDef sprites[];
extern const int tetris_figures[];

enum SprColor {
    SC_Off, // Turn off
    SC_1,   // Turn 1/3 of full brightness
    SC_2,   // Turn 2/3 of full brightness
    SC_Full,  // Turn fully on
    SC_On, // Use brigtness info from SpriteDef (same as SC_Full for non-GS sprites)
    SC_NoChange // Do not change color status (used for 'place' call of active sprite)
};

class Sprite {
    int base_index;
    int index;
    int spr_x=0, spr_y=0, spr_rotation=0;
    SprColor spr_color = SC_Off;

    const SpriteDef& spr2() const { return sprites[index+1]; }

    bool check_collitions();
    void clear_sprite();
    void draw_sprite(SprColor color);
    void set_rotation(int);

    uint32_t combined_spr_mask() const {
        uint32_t spr_mask = spr().pixels;
        if (spr().is_gs) spr_mask |= spr2().pixels;
        return spr_mask;
    }

    using Functor = int(uint8_t& b1, uint8_t& b2, uint8_t, uint8_t d1, uint8_t d2);

    int process(uint32_t, uint32_t, Functor);

public:
    Sprite(int sprite_index) : base_index(sprite_index), index(sprite_index) {}

    const SpriteDef& spr() const { return sprites[index]; }
    const SpriteDef& bspr() const { return sprites[base_index]; }

    bool place(int x, int y, int rotation, SprColor color = SC_On);
    bool move(int dx, int dy, int drot, SprColor color = SC_NoChange) 
    {
        return place(spr_x+dx, spr_y+dy, spr_rotation+drot, color);
    }
};
