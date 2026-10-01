#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

enum PlayerAnimation {
    PLAYER_ANIMATION_IDLE,
    PLAYER_ANIMATION_RUN,
    PLAYER_ANIMATION_JUMP,
    PLAYER_ANIMATION_ROLL
};

struct Player {
    int x;
    int y;
    int dx,dy;
    bool facing_right;
    enum PlayerAnimation animation;
    int speed;
    int health;
    int score;
    int frame; // current frame of the animation
    int animation_timer; // timer for animation transitions
};
void game_init();
void game_update();
void game_render();
void game_handle_input(uint8_t input, bool down);

#endif // GAME_H