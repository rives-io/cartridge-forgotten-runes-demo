#include <riv.h>
#include <math.h>

// Constants
enum {
    MAP_SIZE = 16,
    TARGET_FPS = 30,
    TILE_SIZE = 24,
    WIZARD_SIZE = 50,
    WIZARD_WALK_FRAMES = 4,
    SCREEN_SIZE = TILE_SIZE*MAP_SIZE,
};
enum {
    MAP_PLAYER_GROUND = 0,
    MAP_LAYER_BLOCKABLE = 1,
    MAP_LAYER_ITEMS = 2,
    MAP_LAYER_DECORATIONS = 3,
    MAP_LAYERS = 4,
};
enum {
    DIR_DOWN,
    DIR_LEFT,
    DIR_UP,
    DIR_RIGHT,
};

// Sprite sheets
int wizard_sps;
int tileset_sps;

// State
int score = 0;
riv_vec2f player_pos = {100, 128};
double player_speed = 2;
int player_direction = DIR_DOWN;
bool player_walking = false;
bool player_casting = false;
bool player_boost = false;
int16_t map[MAP_LAYERS][MAP_SIZE][MAP_SIZE] =
#include "map.h"
;

int16_t collides_with_layer(riv_vec2f pos, int l) {
  for (int y = pos.y / TILE_SIZE; y < (pos.y + TILE_SIZE - 1) / TILE_SIZE; ++y) {
    for (int x = pos.x / TILE_SIZE; x < (pos.x + TILE_SIZE - 1) / TILE_SIZE; ++x) {
        if (map[l][y][x] > 0) {
            return map[l][y][x];
        }
    }
  }
  return 0;
}

void player_move() {
    // Update animation state
    player_walking = (riv->keys[RIV_GAMEPAD_RIGHT].down || riv->keys[RIV_GAMEPAD_LEFT].down ||
                      riv->keys[RIV_GAMEPAD_DOWN].down || riv->keys[RIV_GAMEPAD_UP].down);
    player_boost = riv->keys[RIV_GAMEPAD_A1].down;
    player_casting = riv->keys[RIV_GAMEPAD_A2].down;
    // Compute walk speed, delta and direction
    double speed = player_boost ? player_speed*2 : player_speed;
    riv_vec2f delta = {0,0};
    if (riv->keys[RIV_GAMEPAD_RIGHT].down) {
        delta.x += speed;
        player_direction = DIR_RIGHT;
    }
    if (riv->keys[RIV_GAMEPAD_LEFT].down) {
        delta.x -= speed;
        player_direction = DIR_LEFT;
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].down) {
        delta.y += speed;
        player_direction = DIR_DOWN;
    }
    if (riv->keys[RIV_GAMEPAD_UP].down) {
        delta.y -= speed;
        player_direction = DIR_UP;
    }
    if (delta.x != 0 && delta.y != 0) { // slowdown diagonal walking
        delta.x *= 0.70710678118655;
        delta.y *= 0.70710678118655;
    }
    // Move
    if (delta.x != 0) { // move x
        riv_vec2f start_pos = player_pos;
        for (int64_t d = 0; d <= ceil(fabs(delta.x)); ++d) { // move pixel by pixel until it collides
            riv_vec2f pos = {start_pos.x+copysign(fmin(d, fabs(delta.x)), delta.x), start_pos.y};
            if (collides_with_layer(pos, MAP_LAYER_BLOCKABLE)) {
                break;
            }
            player_pos.x = pos.x;
        }
    }
    if (delta.y != 0) { // move y
        riv_vec2f start_pos = player_pos;
        for (int64_t d = 0; d <= ceil(fabs(delta.y)); ++d) { // move pixel by pixel until it collides
            riv_vec2f pos = {start_pos.x, start_pos.y+copysign(fmin(d, fabs(delta.y)), delta.y)};
            if (collides_with_layer(pos, MAP_LAYER_BLOCKABLE)) {
                break;
            }
            player_pos.y = pos.y;
        }
    }
}

void player_check_pickup() {
    int tile_x = (player_pos.x + TILE_SIZE/2) / TILE_SIZE;
    int tile_y = (player_pos.y + TILE_SIZE/2) / TILE_SIZE;
    if (map[MAP_LAYER_ITEMS][tile_y][tile_x] > 0) {
        map[MAP_LAYER_ITEMS][tile_y][tile_x] = 0;
        // Increase score
        score++;
        // Quit when reaching max score
        if (score >= 7) {
            riv->quit_frame = riv->frame + riv->target_fps * 3;
        }
        // Play pickup sound effect
        riv_waveform(&(riv_waveform_desc){
            .type = RIV_WAVEFORM_TRIANGLE,
            .attack = 0.025f, .decay = 0.075f, .sustain = 0.075f, .release = 0.075f,
            .start_frequency = 1260.0f,
            .sustain_level = 0.1,
        });
    }
}

void player_draw() {
    // Draw casting effect
    if (player_casting) {
        for (int i = 0.0;i<6;i++) {
            double x = player_pos.x + 12 + sin(riv->time*4 + i*1.0471975511966) * 24;
            double y = player_pos.y + 12 + cos(riv->time*4 + i*1.0471975511966) * 24;
            riv_draw_circle_fill(x, y, 4, RIV_COLOR_LIGHTRED + (int)i);
        }
    }
    // Draw wizard sprite
    riv->draw.pal_enabled = true;
    if (player_boost) { // Colorize hat
        riv->draw.pal[189] = 16 + (riv->frame / 2) % 16;
        riv->draw.pal[154] = 16 + (riv->frame / 2) % 16 + 1;
        riv->draw.pal[151] = 16 + (riv->frame / 2) % 16 + 2;
        riv->draw.pal[179] = 16 + (riv->frame / 2) % 16 + 3;
    }
    int base_frame = player_direction * WIZARD_WALK_FRAMES;
    int anim_frame = player_walking ? ((riv->frame / 4) % WIZARD_WALK_FRAMES) : 0;
    riv_draw_sprite(base_frame + anim_frame, wizard_sps, player_pos.x - 13, player_pos.y - 21, 1, 1, 1, 1);
    if (player_boost) { // Reset color palette swap
        riv->draw.pal[189] = 189;
        riv->draw.pal[154] = 154;
        riv->draw.pal[151] = 151;
        riv->draw.pal[179] = 179;
    }
    riv->draw.pal_enabled = false;
}

void map_draw() {
    // Draw every layer
    for (int l=0;l<MAP_LAYERS;++l) {
        riv->draw.color_key_disabled = l == 0; // Optimize rendering for first layer
        // Draw tile grid
        for (int y=0;y<MAP_SIZE;++y) {
            for (int x=0;x<MAP_SIZE;++x) {
                int id = map[l][y][x];
                if (id > 0) {
                    // Draw object sprite
                    riv_draw_sprite(id, tileset_sps, x*24, y*24, 1, 1, 1, 1);
                }
            }
        }
    }
}

void save_score() {
    riv->outcard_len = riv_snprintf((char*)riv->outcard, RIV_SIZE_OUTCARD, "JSON{"
      "\"score\":%d,"
      "\"frames\":%ld"
    "}", score, riv->frame);
}

void update() {
    player_move();
    player_check_pickup();
    save_score();
}

void draw() {
    map_draw();
    player_draw();
    if (score >= 7) {
        int col = ((riv->frame / 4) % 2 == 0) ? RIV_COLOR_WHITE : RIV_COLOR_RED;
        riv_draw_text("GAME COMPLETED!", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, SCREEN_SIZE/2, SCREEN_SIZE/2, 2, col);
    } else {
        int col = ((riv->frame / 4) % 2 == 0) ? RIV_COLOR_WHITE : RIV_COLOR_YELLOW;
        riv_draw_text("COLLECT THE GEMS", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, SCREEN_SIZE/2, SCREEN_SIZE-32, 2, col);
    }
}

int main() {
    // Setup resolution and refresh rate
    riv->target_fps = TARGET_FPS;
    riv->width = SCREEN_SIZE;
    riv->height = SCREEN_SIZE;
    // Load sprites
    riv_load_palette("palette.png", 32);
    wizard_sps = riv_make_spritesheet(riv_make_image("wizard-spritesheet-53.png", 255), WIZARD_SIZE, WIZARD_SIZE);
    tileset_sps = riv_make_spritesheet(riv_make_image("tileset.png", 255), TILE_SIZE, TILE_SIZE);
    // Main loop
    do {
        update();
        draw();
    } while(riv_present());
    return 0;
}
