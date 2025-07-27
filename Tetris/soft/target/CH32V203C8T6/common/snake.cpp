#include "interface.h"
#include "arena.h"
#include "timer.h"

class Snake {
    static constexpr int max_level = 10;
    static constexpr int items_per_level = 8;

    // Functional configuration:
    // 
    // How many bricks expected on screen on current level
    uint8_t expected_bricks() const
    {
        if (level < 2) return 0;
        return (level-2)*2;
    }

    // Do we have to put a brick on screen?
    bool need_put_brick() const
    {
        auto expect = expected_bricks();
        if (expect <= bricks) return false;
        auto delta = expect - bricks;
        if (delta >= 5) return true;
        return get_random() % ((5-delta)*10) == 0;
    }

    enum Direction {
        D_Up,
        D_Down,
        D_Right,
        D_Left
    };

    enum Color {
        C_Off = 0,
        C_Brick = 1,
        C_Snake = 2,
        C_Food = 3
    };

    struct Coord {
        uint16_t x, y;

        Coord(uint16_t val=0) : x(val&7), y(val >> 3) {}
        Coord(uint16_t x, uint16_t y) : x(x), y(y) {}
        
        bool move(Direction dir)
        {
            switch (dir)
            {
                case D_Up: if (y) --y; else return false; break;
                case D_Down: if (y<15) ++y; else return false; break;
                case D_Right: if (x<7) ++x; else return false; break;
                case D_Left: if (x) --x; else return false; break;
            }
            return true;
        }

        uint16_t combine() const {return x+y*8;}
    };

    int level = 1; // Freqency in Hz

    uint16_t snake_head=0, snake_tail=0;
    int body_len_increment = 0;
    Timer timer = 1;
    uint8_t bricks=0;
    Direction dir = D_Up;
    Coord coord = {4, 8};

    static uint16_t inc(uint16_t& idx) {++idx; idx &= 511; return idx;}

    void snake_push_head(Coord coord)
    {
        arena.snake.body[inc(snake_head)] = coord.combine();
    }

    Coord snake_pop_tail()
    {
        return arena.snake.body[inc(snake_tail)];
    }

    static void draw(Coord coord, Color color)
    {
        uint8_t mask = 1 << coord.x;
        if (color & 1) pixs.br1[coord.y] |= mask; else pixs.br1[coord.y] &= ~mask;
        if (color & 2) pixs.br2[coord.y] |= mask; else pixs.br2[coord.y] &= ~mask;
    }

    static Color fetch(Coord coord)
    {
        uint8_t p1 = (pixs.br1[coord.y] >> coord.x) & 1;
        uint8_t p2 = (pixs.br2[coord.y] >> coord.x) & 1;
        return Color(p1 + p2*2);
    }

    enum CanMove {
        CM_Yes,
        CM_No,
        CM_Food
    };
    CanMove move_head()
    {
        if (!coord.move(dir)) return CM_No;
        switch (fetch(coord))
        {
            case C_Off: return CM_Yes;
            case C_Food: return CM_Food;
            default: return CM_No;
        }
    }

    // Move snake by 1 pixel. Check for collisitions and Food eating
    // New Food will be put (if need to). Rocks will not updated
    // Return true if snake can move
    bool move_snake()
    {
        auto status = move_head();
        if (status == CM_No) return false;
        snake_push_head(coord);
        draw(coord, C_Snake);
        if (status == CM_Food)
        {
            ++body_len_increment;
            if (body_len_increment >= items_per_level)
            {
                body_len_increment = 0;
                if (level < max_level)
                {
                    ++level;
                    timer.reinit(level);
                }
            }
            put_in_random(C_Food);
        }
        else
        {
            draw(snake_pop_tail(), C_Off);
        }
        return true;
    }

    // Put 1 pixel in random position
    // Do not put in any occupied position and in position of head movement
    void put_in_random(Color color)
    {
        for (;;)
        {
            uint16_t x = get_random() & 7;
            uint16_t y = get_random() & 15;
            if (fetch(Coord(x,y))) continue; // Occupied
            if (x == coord.x) // Check up/down
            {
                switch (dir)
                {
                    case D_Up: if (y<=coord.y) continue; break;
                    case D_Down: if (y>=coord.y) continue; break;
                }
            }
            else if (y == coord.y) // Check left/right
            {
                switch (dir)
                {
                    case D_Left: if (x <= coord.x) continue; break;
                    case D_Right: if (x >= coord.x) continue; break;
                }
            }
            draw(Coord(x, y), color);
            break;
        }
    }

public:
    Snake()
    {
        snake_push_head(coord);
        draw(coord, C_Snake);
        put_in_random(C_Food);
    }

    void run()
    {
        for (;;)
        {
            auto key = read_key();
            clr_keys(key);
            if (key & K_3) return;
            key &= K_Up | K_Down | K_Left | K_Right;
            switch (key)
            {
                case K_Up:      if (dir != D_Down) dir = D_Up; break;
                case K_Down:    if (dir != D_Up) dir = D_Down; break;
                case K_Left:    if (dir != D_Right) dir = D_Left; break;
                case K_Right:   if (dir != D_Left) dir = D_Right; break;
            }
            if (timer.tick())
            {
                if (!move_snake()) return;
                if (need_put_brick()) {++bricks; put_in_random(C_Brick);}
            }
        }
    }
};

void snake_game()
{
    Snake().run();
}