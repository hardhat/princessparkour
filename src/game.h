#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>

enum PlayerAnimation {
    PLAYER_ANIMATION_IDLE,
    PLAYER_ANIMATION_RUN,
    PLAYER_ANIMATION_JUMP,
    PLAYER_ANIMATION_ROLL,
    PLAYER_ANIMATION_HANG,
};

struct Player {
    int x;
    int y;
    int vx, vy; // velocity in subpixels (1/16 pixel) per frame
    int sx, sy; // subpixel remainder of position
    bool on_ground;
    uint8_t coyote_timer; // frames left to jump after walking off a ledge
    uint8_t jump_buffer;  // frames left to honor an early jump press
    bool facing_right;
    enum PlayerAnimation animation;
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