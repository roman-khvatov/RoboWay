#include "sprite.h"

bool Sprite::place(int x, int y, int rotation, SprColor color)
{
    int sv_x = spr_x, sv_y = spr_y, sv_rot = spr_rotation;
    if (color == SC_NoChange) color = spr_color;
    if (spr_color) clear_sprite();
    spr_x = x; spr_y = y;
    set_rotation(rotation);
    if (!color) { spr_color = SC_Off; return true; }
    bool collition = check_collitions();
    if (collition)
    {
        spr_x = sv_x; spr_y = sv_y; set_rotation(sv_rot);
        color = spr_color;
    }
    if (color) draw_sprite(color);
    spr_color = color;
    return !collition;
}

int Sprite::process(uint32_t spr1_data, uint32_t spr2_data, Sprite::Functor func)
{    
    const SpriteDef& S = spr();
    uint8_t* b1 = &pixs.br1[spr_y - S.height / 2];
    uint8_t* b2 = &pixs.br2[spr_y - S.height / 2];
    int result = 0;
    uint8_t mask = (1 << S.width) - 1;
    uint8_t shift = spr_x - S.width / 2;
    uint8_t data_mask = mask << shift;
    for (int row = 0; row < S.height; ++row, ++b1, ++b2)
    {
        result |= func(*b1, *b2, data_mask, (spr1_data & mask) << shift, (spr2_data & mask) << shift);
        spr1_data >>= S.width;
        spr2_data >>= S.width;
    }
    return result;
}


bool Sprite::check_collitions()
{
    const SpriteDef& S = spr();
    if (spr_x < int(S.width/2) || spr_y < int(S.height/2) || spr_x+S.width-S.width/2 > 8 || spr_y + S.height - S.height/2 > 16) return true;
    uint32_t spr_mask = combined_spr_mask();

    return process(spr_mask, spr_mask, [](uint8_t& b1, uint8_t& b2, uint8_t, uint8_t d1, uint8_t d2) ->int {
        return (b1|b2) & (d1|d2);
    }) != 0;
}

void Sprite::clear_sprite()
{
    uint32_t spr_mask = combined_spr_mask();
    process(spr_mask, spr_mask, [](uint8_t& b1, uint8_t& b2, uint8_t, uint8_t d1, uint8_t d2) ->int {
        b1 &= ~d1;
        b2 &= ~d2;
        return 0;
    });
}

void Sprite::draw_sprite(SprColor color)
{
    const SpriteDef& S = spr();
    if (color == SC_On && !S.is_gs) color = SC_Full;
    uint32_t data1=0, data2=0;
    switch (color)
    {
        case SC_1: data1 = S.pixels; break;
        case SC_2: data2 = S.pixels; break;
        case SC_Full: data1 = data2 = S.pixels; break;
        case SC_On: data1 = S.pixels; data2 = spr2().pixels; break;
        default: assert(false); break; // Should never happened!
    }
    process(data1, data2, [](uint8_t& b1, uint8_t& b2, uint8_t mask, uint8_t d1, uint8_t d2) ->int {
        mask &= d1 | d2;
        b1 = (b1 & ~mask) | d1;
        b2 = (b2 & ~mask) | d2;
        return 0;
    });
}

void Sprite::set_rotation(int new_rot)
{
    const SpriteDef& S = bspr();
    int max_sprites = S.group_size+1;
    if (new_rot < 0) new_rot += max_sprites;
    new_rot %= max_sprites;
    spr_rotation = new_rot;
    index = base_index + (S.is_gs ? spr_rotation*2 : spr_rotation);
}
