#include "missile.h"
#include "config.h"
#include "lcd.h"
#include <math.h>
#include <stdlib.h>

// Define the states for the missile state machine
enum missile_st_t {
    init_st,
    idle_st,
    moving_st,
    exploding_growing_st,
    exploding_shrinking_st,
    impacted_st
};

// Helper function to finalize launch parameters
static void finalize_launch(missile_t *missile, coord_t x_orig, coord_t y_orig, coord_t x_dest, coord_t y_dest) {
    missile->x_origin = x_orig;
    missile->y_origin = y_orig;
    missile->x_dest = x_dest;
    missile->y_dest = y_dest;
    missile->x_current = x_orig;
    missile->y_current = y_orig;
    missile->length = 0.0f;
    missile->radius = 0.0f;
    missile->launch = false;
    missile->explode_me = false;

    // Calculate total length using Pythagorean theorem
    float dx = (float)(x_dest - x_orig);
    float dy = (float)(y_dest - y_orig);
    missile->total_length = sqrtf((dy * dy) + (dx * dx));
}

// Initialize the missile and set it to idle
void missile_init(missile_t *missile) {
    missile->currentState = init_st;
    missile->type = MISSILE_TYPE_PLAYER; // Default type
    missile->launch = false;
    missile->explode_me = false;
    missile->length = 0.0f;
    missile->total_length = 0.0f;
    missile->radius = 0.0f;
    missile->x_origin = 0;
    missile->y_origin = 0;
    missile->x_dest = 0;
    missile->y_dest = 0;
    missile->x_current = 0;
    missile->y_current = 0;
}

// Launch as a player missile
void missile_launch_player(missile_t *missile, coord_t x_dest, coord_t y_dest) {
    missile->type = MISSILE_TYPE_PLAYER;

    // Silo Positions
    coord_t site1_x = LCD_W / 4; // Silo 1
    coord_t site2_x = LCD_W / 2; // Silo 2
    coord_t site3_x = (3 * LCD_W) / 4; // Silo 3
    coord_t y_bottom = LCD_H; // Ground/bottom level

    // Find the closest firing location based on which 3rd of the screen youre in
    // If Silo 1
    coord_t best_x = site1_x;
    int32_t min_dist_sq = (x_dest - site1_x) * (x_dest - site1_x);

    // If Silo 2
    int32_t dist2 = (x_dest - site2_x) * (x_dest - site2_x);
    if (dist2 < min_dist_sq) {
        min_dist_sq = dist2;
        best_x = site2_x;
    }

    // If Silo 3
    int32_t dist3 = (x_dest - site3_x) * (x_dest - site3_x);
    if (dist3 < min_dist_sq) {
        best_x = site3_x;
    }

    finalize_launch(missile, best_x, y_bottom, x_dest, y_dest);
    missile->launch = true;
}

// Launch as an enemy missile
void missile_launch_enemy(missile_t *missile) {
    missile->type = MISSILE_TYPE_ENEMY;

    // Random origin near the top quarter of the screen, random destination along the bottom
    coord_t x_orig = rand() % LCD_W;
    coord_t y_orig = rand() % (LCD_H / 4);
    coord_t x_dest = rand() % LCD_W;
    coord_t y_dest = LCD_H;

    finalize_launch(missile, x_orig, y_orig, x_dest, y_dest);
    missile->launch = true;
}

// Launch as a plane missile
void missile_launch_plane(missile_t *missile, coord_t x_orig, coord_t y_orig) {
    missile->type = MISSILE_TYPE_PLANE;

    // Destination chosen randomly along the bottom of the screen
    coord_t x_dest = rand() % LCD_W;
    coord_t y_dest = LCD_H;

    finalize_launch(missile, x_orig, y_orig, x_dest, y_dest);
    missile->launch = true;
}

// Trigger explosion flag when hit by another explosion
void missile_explode(missile_t *missile) {
    missile->explode_me = true;
}

// Missile Tick
void missile_tick(missile_t *missile) {

    // State Transition Processing, Passing the missle pointer changes sytax from "standard"
    switch (missile->currentState) {
        //Move directly to idle state when in init state
        case init_st:
            missile->currentState = idle_st;
            break;

        // Check if the missle has been lauched, transition to moving state
        case idle_st:
            if (missile->launch) {
                missile->currentState = moving_st;
            }
            break;
        
        // Handles the different ways missiles can be stopped while moving    
        case moving_st:
            if (missile->explode_me) {
                // Hit by an explosion while moving -> start exploding
                missile->currentState = exploding_growing_st;
            } else if (missile->length >= missile->total_length) {
                // Reached destination. If player missile, explode
                if (missile->type == MISSILE_TYPE_PLAYER) {
                    missile->currentState = exploding_growing_st;
                } else {
                    missile->currentState = impacted_st;
                }
            }
            break;

        // Transition to shrinking if explosion reaches max size    
        case exploding_growing_st:
            if (missile->radius >= CONFIG_EXPLOSION_MAX_RADIUS) {
                missile->currentState = exploding_shrinking_st;
            }
            break;

        // Tranitions to idle after explosion gets small enough
        case exploding_shrinking_st:
            if (missile->radius <= 0.0f) {
                missile->currentState = idle_st;
            }
            break;

        case impacted_st:
            // Impacted state lasts for one tick before returning to idle
            missile->currentState = idle_st;
            break;

        default:
            missile->currentState = init_st;
            break;
    }

    
    // State Action Processing
    switch (missile->currentState) {
        case init_st:
            break;

        // Set everthing to 0
        case idle_st:
            missile->launch = false;
            missile->explode_me = false;
            missile->length = 0.0f;
            missile->radius = 0.0f;
            break;

        // Determine distance per tick based on missile type
        case moving_st: {
            // Defaults
            float distance_per_tick = 0.0f;
            uint16_t color = CONFIG_COLOR_PLAYER_MISSILE;

            if (missile->type == MISSILE_TYPE_PLAYER) {
                distance_per_tick = CONFIG_PLAYER_MISSILE_DISTANCE_PER_TICK;
                color = CONFIG_COLOR_PLAYER_MISSILE;
            } else if (missile->type == MISSILE_TYPE_ENEMY) {
                distance_per_tick = CONFIG_ENEMY_MISSILE_DISTANCE_PER_TICK;
                color = CONFIG_COLOR_ENEMY_MISSILE;
            } else if (missile->type == MISSILE_TYPE_PLANE) {
                // Plane missiles behave like enemy missiles
                distance_per_tick = CONFIG_ENEMY_MISSILE_DISTANCE_PER_TICK;
                color = CONFIG_COLOR_PLANE_MISSILE;
            }

            // Update flight length progress
            missile->length += distance_per_tick;
            if (missile->length > missile->total_length) {
                missile->length = missile->total_length;
            }

            // Calculate current (x, y) position along the path
            float fraction = missile->length / missile->total_length;
            missile->x_current = missile->x_origin + (coord_t)(fraction * (float)(missile->x_dest - missile->x_origin));
            missile->y_current = missile->y_origin + (coord_t)(fraction * (float)(missile->y_dest - missile->y_origin));

            // Draw the missile trail line
            lcd_drawLine(missile->x_origin, missile->y_origin, missile->x_current, missile->y_current, color);
            break;
        }

        // Grow explosion radius 
        case exploding_growing_st: {
            missile->radius += CONFIG_EXPLOSION_RADIUS_CHANGE_PER_TICK;
            if (missile->radius > CONFIG_EXPLOSION_MAX_RADIUS) {
                missile->radius = CONFIG_EXPLOSION_MAX_RADIUS;
            }
            if (missile->type == MISSILE_TYPE_ENEMY){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_ENEMY_MISSILE);
            }else if (missile->type == MISSILE_TYPE_PLANE){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_PLANE_MISSILE);
            }else if (missile->type == MISSILE_TYPE_PLAYER){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_PLAYER_MISSILE);
            }
            break;
        }

        case exploding_shrinking_st: {
            missile->radius -= CONFIG_EXPLOSION_RADIUS_CHANGE_PER_TICK;
            if (missile->radius < 0.0f) {
                missile->radius = 0.0f;
            }
            if (missile->type == MISSILE_TYPE_ENEMY){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_ENEMY_MISSILE);
            }else if (missile->type == MISSILE_TYPE_PLANE){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_PLANE_MISSILE);
            }else if (missile->type == MISSILE_TYPE_PLAYER){
                lcd_fillCircle(missile->x_current, missile->y_current, (int16_t)missile->radius, CONFIG_COLOR_PLAYER_MISSILE);
            }
            break;
        }

        case impacted_st:
            // Handled by game.c
            break;

        default:
            break;
    }
}

// Status & Helper Functions
void missile_get_pos(missile_t *missile, coord_t *x, coord_t *y) {
    *x = missile->x_current;
    *y = missile->y_current;
}

missile_type_t missile_get_type(missile_t *missile) {
    return missile->type;
}

bool missile_is_moving(missile_t *missile) {
    return missile->currentState == moving_st;
}

bool missile_is_exploding(missile_t *missile) {
    return (missile->currentState == exploding_growing_st || missile->currentState == exploding_shrinking_st);
}

bool missile_is_idle(missile_t *missile) {
    return missile->currentState == idle_st;
}

bool missile_is_impacted(missile_t *missile) {
    return missile->currentState == impacted_st;
}

bool missile_is_colliding(missile_t *missile, coord_t x, coord_t y) {
    // Must be exploding to cause a collision[cite: 14]
    if (!missile_is_exploding(missile)) {
        return false;
    }

    // Check distance squared against radius squared (avoid sqrtf for efficiency)[cite: 14]
    int32_t dx = (int32_t)x - (int32_t)missile->x_current;
    int32_t dy = (int32_t)y - (int32_t)missile->y_current;
    int32_t dist_sq = (dx * dx) + (dy * dy);
    int32_t radius_sq = (int32_t)(missile->radius * missile->radius);

    return dist_sq <= radius_sq;
}