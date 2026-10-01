#include "main.h"
#include "game.h"

struct Player player;

struct Frame {
    uint8_t base;   // base tile index
    uint8_t width;  // in tiles
    uint8_t height;  // in tiles
};
struct Animation {
    uint8_t frame_count;  // number of frames in the animation
    const struct Frame* frames;  // pointer to array of frames in the animation
    int animation_timer; // timer for animation transitions in ms
};

const struct Frame walk_frames[] = {
    { .base = 16, .width = 2, .height = 3 },
    { .base = 18, .width = 2, .height = 3 },
    { .base = 20, .width = 2, .height = 3 },
    { .base = 22, .width = 2, .height = 3 },
    { .base = 24, .width = 2, .height = 3 },
    { .base = 26, .width = 2, .height = 3 },
    { .base = 28, .width = 2, .height = 3 },
    { .base = 30, .width = 2, .height = 3 },
};

const struct Frame idle_frames[] = {
    { .base = 0x40, .width = 2, .height = 3 },
    { .base = 0x42, .width = 2, .height = 3 },
};

const struct Frame jump_frames[] = {
    { .base = 0x4c, .width = 2, .height = 3 },
    { .base = 0x4e, .width = 2, .height = 3 },
};

const struct Frame roll_frames[] = {
    { .base = 0x44, .width = 3, .height = 2 },
    { .base = 0x47, .width = 3, .height = 2 },
    { .base = 0x4a, .width = 2, .height = 2 },
    { .base = 0x4c, .width = 2, .height = 3 }, 
};

const struct Animation player_animation[] = {
    [PLAYER_ANIMATION_IDLE] = {
        .frame_count = 2,
        .frames = idle_frames,
        .animation_timer = 256 // example value in ms
    },
    [PLAYER_ANIMATION_RUN] = {
        .frame_count = 8,
        .frames = walk_frames,
        .animation_timer = 64 // example value in ms
    },
    [PLAYER_ANIMATION_JUMP] = {
        .frame_count = 2,
        .frames = jump_frames,
        .animation_timer = 256 // example value in ms
    },
    [PLAYER_ANIMATION_ROLL] = {
        .frame_count = 4,
        .frames = roll_frames,
        .animation_timer = 64 // example value in ms
    }
}; 

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
    player.animation = PLAYER_ANIMATION_IDLE;
    player.facing_right = true;
    player.animation_timer = player_animation[player.animation].animation_timer;
}

static void set_animation(enum PlayerAnimation animation) {
    if(player.animation != animation) {
        player.animation = animation;
        player.frame = 0;
        player.animation_timer = player_animation[animation].animation_timer;
    }
}

void game_update() {
    // Update game logic here
    player.x += player.dx;
    player.y += player.dy;
    // Additional game update code here
    // Update animation timer
    if(player.animation_timer > 0) {
        player.animation_timer-=16; // assuming a 60 FPS update rate, ~16ms per frame
        if(player.animation_timer <= 0) {
            player.animation_timer = player_animation[player.animation].animation_timer;
            player.frame = (player.frame + 1) % player_animation[player.animation].frame_count;
        }
    }
}

void game_render() {
    reset_sprite();
    // Render game graphics here (anchor in bottom center)
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    bool flip = !player.facing_right;
    int left = player.x - f->width * 8; // Calculate left position based on sprite width
    int top = player.y - f->height * 16; // Calculate top position based on sprite height
    for(uint8_t row = 0; row < f->height; row++) {
        for(uint8_t col = 0; col < f->width; col++) {
            // Tilesheet rows are 16 tiles wide
            uint8_t tile = f->base + (row << 4) + (flip ? f->width - 1 - col : col);
            add_sprite(left + (col << 4), top + (row << 4), tile, flip ? SPRITE_FLAG_FLIP_X : 0);
        }
    }
    render_sprites();
}

void game_handle_input(uint8_t input, bool down) {
    // Handle game input here
    if(input == INPUT_UP && down) {
        // Move player up
        player.dy = -player.speed; // Move player up
        set_animation(PLAYER_ANIMATION_JUMP);
    } else if(input == INPUT_DOWN && down) {
        // Move player down
        player.dy = 0;
        player.dx = player.facing_right ? player.speed : -player.speed;
        set_animation(PLAYER_ANIMATION_ROLL);
    } else if(input == INPUT_UP && !down) {
        // Stop moving up
        player.dy = 0;
        set_animation(PLAYER_ANIMATION_IDLE);
    } else if(input == INPUT_DOWN && !down) {
        // Stop moving down
        player.dy = 0;
        player.dx = 0;
        set_animation(PLAYER_ANIMATION_IDLE);
    } else if(input == INPUT_LEFT && down) {
        // Move player left
        player.dx = -player.speed;
        set_animation(PLAYER_ANIMATION_RUN);
        player.facing_right = false;
        debug_log("left");
    } else if(input == INPUT_RIGHT && down) {
        // Move player right
        player.dx = player.speed;
        set_animation(PLAYER_ANIMATION_RUN);
        player.facing_right = true;
        debug_log("right");
    } else if(input == INPUT_LEFT && !down) {
        // Stop moving left
        player.dx = 0;
        set_animation(PLAYER_ANIMATION_IDLE);
        debug_log("stop left");
    } else if(input == INPUT_RIGHT && !down) {
        // Stop moving right
        player.dx = 0;
        set_animation(PLAYER_ANIMATION_IDLE);
        debug_log("stop right");
    }
}