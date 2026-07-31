#include "main.h"
#include "game.h"

struct Player player;
    
void game_init() {
    // Initialize player
    player.x = 32;
    player.y = 240-64;
    player.dx = 0;
    player.dy = 0;
    player.speed = 4;
    player.health = 100;
    player.score = 0;
    player.frame = 0;
}

void game_update() {
    // Update game logic here
    player.x += player.dx;
    player.y += player.dy;
    // Additional game update code here
    player.frame = (player.frame+1) % 8; // Cycle through animation frames
}

void game_render() {
    reset_sprite();
    // Render game graphics here (anchor in bottom center)
    int base_frame = 16 + (player.dx==0 ? 0 : 8) + 2*(player.frame>>1);  // Idle or running frames
    if(player.dx < 0) {
        // 2x3 sprite frames for running left
        add_sprite(player.x-16, player.y-48, base_frame+1, SPRITE_FLAG_FLIP_X);
        add_sprite(player.x, player.y-48, base_frame, SPRITE_FLAG_FLIP_X);
        add_sprite(player.x-16, player.y-32, base_frame+17, SPRITE_FLAG_FLIP_X);
        add_sprite(player.x, player.y-32, base_frame+16, SPRITE_FLAG_FLIP_X);
        add_sprite(player.x-16, player.y-16, base_frame+33, SPRITE_FLAG_FLIP_X);
        add_sprite(player.x, player.y-16, base_frame+32, SPRITE_FLAG_FLIP_X);
    } else {
        add_sprite(player.x-16, player.y-48, base_frame, 0);
        add_sprite(player.x, player.y-48, base_frame+1, 0);
        add_sprite(player.x-16, player.y-32, base_frame+16, 0);
        add_sprite(player.x, player.y-32, base_frame+17, 0);
        add_sprite(player.x-16, player.y-16, base_frame+32, 0);
        add_sprite(player.x, player.y-16, base_frame+33, 0);
    }
    render_sprites();
}

void game_handle_input(uint8_t input, bool down) {
    // Handle game input here
    if(input == INPUT_UP && down) {
        // Move player up
    } else if(input == INPUT_DOWN && down) {
        // Move player down
    } else if(input == INPUT_LEFT && down) {
        // Move player left
        player.dx = -player.speed;
        debug_log("left");
    } else if(input == INPUT_RIGHT && down) {
        // Move player right
        player.dx = player.speed;
        debug_log("right");
    } else if(input == INPUT_LEFT && !down) {
        // Stop moving left
        player.dx = 0;
        debug_log("stop left");
    } else if(input == INPUT_RIGHT && !down) {
        // Stop moving right
        player.dx = 0;
        debug_log("stop right");
    }
}