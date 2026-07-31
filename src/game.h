#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

struct Player {
    int x;
    int y;
    int dx,dy;
    int speed;
    int health;
    int score;
    int frame;
};
void game_init();
void game_update();
void game_render();
void game_handle_input(uint8_t input, bool down);

#endif // GAME_H