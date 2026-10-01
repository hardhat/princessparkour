#include "main.h"
#include "game.h"
#include "../map/tutorial_map.h"

#define TILE_SIZE 16
#define PLAYER_MAX_HITBOX_HEIGHT 32
#define PLAYER_HITBOX_INSET 8   // arms stick out past the body

// Ledge grab: distance from feet up to the hands in the hang frame
#define HANG_HAND_OFFSET 44
#define GRAB_RANGE 8
#define LEDGE_HANG_TICKS 12
#define LEDGE_CLIMB_FRAME_TICKS 8
#define LEDGE_CLIMB_FRAMES 3
#define LEDGE_CLIMB_TICKS (LEDGE_HANG_TICKS + LEDGE_CLIMB_FRAMES * LEDGE_CLIMB_FRAME_TICKS)

// Physics values are in subpixels (1/16 pixel) per frame at 60 FPS
#define SUBPIXEL 16
#define MAX_RUN_SPEED 48
#define ROLL_SPEED 56
#define GROUND_ACCEL 4
#define GROUND_FRICTION 6
#define AIR_ACCEL 3
#define AIR_FRICTION 1
#define GRAVITY 8
#define MAX_FALL_SPEED 112
#define JUMP_VELOCITY 120
#define JUMP_CUT_VELOCITY 32
#define COYOTE_FRAMES 6
#define JUMP_BUFFER_FRAMES 6

struct Player player;

static bool held_left, held_right, held_down, held_jump;

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

const struct Frame hang_frames[] = {
    { .base = 0x64, .width = 2, .height = 3 },
    { .base = 0x4e, .width = 2, .height = 3 },
    { .base = 0x4c, .width = 2, .height = 3 },
    { .base = 0x4a, .width = 2, .height = 2 },
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
    },
    [PLAYER_ANIMATION_HANG] = {
        .frame_count = 4,
        .frames = hang_frames,
        .animation_timer = 256 // example value in ms
    }
}; 

void show_tutorial_map(void)
{
    show_map(map_data, MAP_WIDTH, MAP_HEIGHT);
}

static bool tile_solid(uint8_t tx, uint8_t ty) {
    uint8_t tile = map_data[ty * MAP_WIDTH + tx];
    return tile != 0 && tile != 255;
}

// Out-of-bounds counts as solid
static bool rect_collides(int left, int top, int width, int height) {
    int right = left + width - 1;
    int bottom = top + height - 1;
    if(left < 0 || top < 0 || right >= MAP_WIDTH * TILE_SIZE || bottom >= MAP_HEIGHT * TILE_SIZE) {
        return true;
    }
    for(uint8_t ty = top / TILE_SIZE; ty <= bottom / TILE_SIZE; ty++) {
        for(uint8_t tx = left / TILE_SIZE; tx <= right / TILE_SIZE; tx++) {
            if(tile_solid(tx, ty)) return true;
        }
    }
    return false;
}

// Player position is bottom-center of the current frame
static int hitbox_half_width(void) {
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    return f->width * (TILE_SIZE / 2) - PLAYER_HITBOX_INSET;
}

static bool player_collides(int x, int y) {
    const struct Frame* f = &player_animation[player.animation].frames[player.frame];
    int half = hitbox_half_width();
    int height = f->height * TILE_SIZE;
    // Top row of 3-tile-tall frames is empty space above the head
    if(height > PLAYER_MAX_HITBOX_HEIGHT) height = PLAYER_MAX_HITBOX_HEIGHT;
    return rect_collides(x - half, y - height, half * 2, height);
}

// Step one pixel at a time so the player stops flush against walls
static void move_player(void) {
    player.sx += player.vx;
    int dx = player.sx / SUBPIXEL;
    player.sx -= dx * SUBPIXEL;
    int step = dx > 0 ? 1 : -1;
    for(; dx != 0; dx -= step) {
        if(player_collides(player.x + step, player.y)) {
            player.vx = 0;
            player.sx = 0;
            break;
        }
        player.x += step;
    }
    // Sub-pixel motion into a wall would otherwise leave vx nonzero for a few frames
    if(player.vx != 0 && player_collides(player.x + (player.vx > 0 ? 1 : -1), player.y)) {
        player.vx = 0;
        player.sx = 0;
    }

    player.sy += player.vy;
    int dy = player.sy / SUBPIXEL;
    player.sy -= dy * SUBPIXEL;
    step = dy > 0 ? 1 : -1;
    for(; dy != 0; dy -= step) {
        if(player_collides(player.x, player.y + step)) {
            player.vy = 0;
            player.sy = 0;
            break;
        }
        player.y += step;
    }

    player.on_ground = player_collides(player.x, player.y + 1);
}

void game_init() {
    // Initialize player
    player.x = 32;
    player.y = 240-64;
    player.vx = 0;
    player.vy = 0;
    player.sx = 0;
    player.sy = 0;
    player.on_ground = false;
    player.coyote_timer = 0;
    player.jump_buffer = 0;
    player.climb_timer = 0;
    held_left = held_right = held_down = held_jump = false;
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

// Grab a ledge when falling against a wall whose top corner is within reach of the hands
static bool try_grab_ledge(void) {
    int dir;
    if(player.on_ground || player.vy < 0) return false;
    if(held_right && player.facing_right) dir = 1;
    else if(held_left && !player.facing_right) dir = -1;
    else return false;
    if(!player_collides(player.x + dir, player.y)) return false;

    int half = hitbox_half_width();
    int wall_x = dir > 0 ? player.x + half : player.x - half - 1;
    if(wall_x < 0 || wall_x >= MAP_WIDTH * TILE_SIZE) return false;
    int hand_y = player.y - HANG_HAND_OFFSET;
    if(hand_y + GRAB_RANGE < TILE_SIZE) return false;
    uint8_t tx = wall_x / TILE_SIZE;
    uint8_t ty = (hand_y + GRAB_RANGE) / TILE_SIZE;
    if(ty >= MAP_HEIGHT || !tile_solid(tx, ty) || tile_solid(tx, ty - 1)) return false;

    int ledge_y = ty * TILE_SIZE;
    int hang_y = ledge_y + HANG_HAND_OFFSET;
    int ledge_x = dir > 0 ? tx * TILE_SIZE + half : (tx + 1) * TILE_SIZE - half;
    if(player_collides(player.x, hang_y) || player_collides(ledge_x, ledge_y)) return false;

    player.y = hang_y;
    player.vx = player.vy = 0;
    player.sx = player.sy = 0;
    player.climb_timer = 0;
    player.climb_from_x = player.x;
    player.ledge_x = ledge_x;
    player.ledge_y = ledge_y;
    set_animation(PLAYER_ANIMATION_HANG);
    return true;
}

// Hang briefly, then step through the climb frames while easing onto the ledge
static void update_climb(void) {
    player.climb_timer++;
    if(player.climb_timer >= LEDGE_CLIMB_TICKS) {
        player.x = player.ledge_x;
        player.y = player.ledge_y;
        player.on_ground = true;
        player.jump_buffer = 0;
        set_animation(PLAYER_ANIMATION_IDLE);
        return;
    }
    uint8_t frame = player.climb_timer < LEDGE_HANG_TICKS ? 0 :
        1 + (player.climb_timer - LEDGE_HANG_TICKS) / LEDGE_CLIMB_FRAME_TICKS;
    int from_y = player.ledge_y + HANG_HAND_OFFSET;
    player.frame = frame;
    player.x = player.climb_from_x + (player.ledge_x - player.climb_from_x) * frame / LEDGE_CLIMB_FRAMES;
    player.y = from_y + (player.ledge_y - from_y) * frame / LEDGE_CLIMB_FRAMES;
}

void game_update() {
    if(player.animation == PLAYER_ANIMATION_HANG) {
        update_climb();
        return;
    }
    bool rolling = held_down && player.on_ground;
    int accel = player.on_ground ? GROUND_ACCEL : AIR_ACCEL;
    int friction = player.on_ground ? GROUND_FRICTION : AIR_FRICTION;

    if(rolling) {
        player.vx = player.facing_right ? ROLL_SPEED : -ROLL_SPEED;
    } else if(held_left && !held_right) {
        player.facing_right = false;
        player.vx -= accel;
        if(player.vx < -MAX_RUN_SPEED) player.vx = -MAX_RUN_SPEED;
    } else if(held_right && !held_left) {
        player.facing_right = true;
        player.vx += accel;
        if(player.vx > MAX_RUN_SPEED) player.vx = MAX_RUN_SPEED;
    } else if(player.vx > 0) {
        player.vx = player.vx > friction ? player.vx - friction : 0;
    } else if(player.vx < 0) {
        player.vx = player.vx < -friction ? player.vx + friction : 0;
    }

    if(player.on_ground) {
        player.coyote_timer = COYOTE_FRAMES;
    } else if(player.coyote_timer > 0) {
        player.coyote_timer--;
    }
    if(player.jump_buffer > 0) {
        player.jump_buffer--;
        if(player.coyote_timer > 0) {
            player.vy = -JUMP_VELOCITY;
            player.jump_buffer = 0;
            player.coyote_timer = 0;
        }
    }
    // Releasing jump early gives a shorter hop
    if(!held_jump && player.vy < -JUMP_CUT_VELOCITY) {
        player.vy = -JUMP_CUT_VELOCITY;
    }
    player.vy += GRAVITY;
    if(player.vy > MAX_FALL_SPEED) player.vy = MAX_FALL_SPEED;

    move_player();
    if(try_grab_ledge()) return;

    if(!player.on_ground) {
        set_animation(PLAYER_ANIMATION_JUMP);
    } else if(rolling) {
        set_animation(PLAYER_ANIMATION_ROLL);
    } else if(player.vx != 0) {
        set_animation(PLAYER_ANIMATION_RUN);
    } else {
        set_animation(PLAYER_ANIMATION_IDLE);
    }

    if(player.animation == PLAYER_ANIMATION_JUMP) {
        player.frame = player.vy < 0 ? 0 : 1;  // rising / falling
    } else if(player.animation_timer > 0) {
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
    switch(input) {
        case INPUT_UP:
            if(down && !held_jump) player.jump_buffer = JUMP_BUFFER_FRAMES;
            held_jump = down;
            break;
        case INPUT_DOWN:
            held_down = down;
            break;
        case INPUT_LEFT:
            held_left = down;
            break;
        case INPUT_RIGHT:
            held_right = down;
            break;
    }
}