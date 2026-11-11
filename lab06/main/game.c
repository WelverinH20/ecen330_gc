
#include <stdio.h>
#include <stdlib.h> // rand

#include "hw.h"
#include "lcd.h"
#include "cursor.h"
#include "sound.h"
#include "pin.h"
#include "missile.h"
#include "plane.h"
#include "game.h"
#include "config.h"

// sound support
#include "missileLaunch.h"

// M2: Define stats constants
#define STATS_TEXT_Y     2
#define STATS_SHOT_X     10
#define STATS_IMPACT_X   120
#define GAME_VOLUME 50
static int LAST_BTN_STATE = 1;

// All missiles
missile_t missiles[CONFIG_MAX_TOTAL_MISSILES];

// Alias into missiles array
missile_t *enemy_missiles = missiles+0;
missile_t *player_missiles = missiles+CONFIG_MAX_ENEMY_MISSILES;
missile_t *plane_missile = missiles+CONFIG_MAX_ENEMY_MISSILES+
									CONFIG_MAX_PLAYER_MISSILES;

// M2: Declare stats variables
static uint32_t PLAYER_SHOT_COUNT = 0;
static uint32_t ENEMY_IMPACT_COUNT = 0;

// Initialize the game control logic.
// This function initializes all missiles, planes, stats, etc.
void game_init(void)
{
	// Initialize missiles
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
		missile_init(missiles+i);

	// Initialize plane
	plane_init(plane_missile);

	// M2: Initialize stats
	PLAYER_SHOT_COUNT = 0;
	ENEMY_IMPACT_COUNT = 0;

	// M2: Set sound volume
	sound_set_volume(GAME_VOLUME);
}

// Update the game control logic.
// This function calls the missile & plane tick functions, relaunches
// idle enemy missiles, handles button presses, launches player missiles,
// detects collisions, and updates statistics.
void game_tick(void)
{
	// Tick missiles in one batch
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
		missile_tick(missiles+i);

	// Tick plane
	plane_tick(); // Move plane and launch plane missile

	// Relaunch idle enemy missiles
	for (uint32_t i = 0; i < CONFIG_MAX_ENEMY_MISSILES; i++)
		if (missile_is_idle(enemy_missiles+i))
			missile_launch_enemy(enemy_missiles+i);


	/*
	// M1: Relaunch idle player missiles, !!! remove after Milestone 1 !!!
	for (uint32_t i = 0; i < CONFIG_MAX_PLAYER_MISSILES; i++)
		if (missile_is_idle(player_missiles+i))
			missile_launch_player(player_missiles+i, rand()%LCD_W, rand()%LCD_H);*/

	// M2: Check for button press. If so, launch a free player missile.

	int CURRENT_BTN_STATE = pin_get_level(HW_BTN_A);
	if (CURRENT_BTN_STATE == 0 && LAST_BTN_STATE != 0) {
        // Find an idle player missile
        for (uint32_t i = 0; i < CONFIG_MAX_PLAYER_MISSILES; i++) {
            if (missile_is_idle(player_missiles + i)) {
                coord_t cursor_x, cursor_y;
                cursor_get_pos(&cursor_x, &cursor_y);

                // Launch player missile towards cursor position
                missile_launch_player(player_missiles + i, cursor_x, cursor_y);

                // Play launch sound and increment stats
                sound_start(missileLaunch, sizeof(missileLaunch), false);
                PLAYER_SHOT_COUNT++;
                break; // Only launch one per button press tick
            }
        }
    }
	LAST_BTN_STATE = CURRENT_BTN_STATE;
	// M2: Check for moving non-player missile collision with an explosion.
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++) { // Includes enemy + plane missile
        missile_t *m = missiles + i;
        if (missile_is_moving(m) && (m->type == MISSILE_TYPE_ENEMY || m->type == MISSILE_TYPE_PLANE)) {
            coord_t mx, my;
            missile_get_pos(m, &mx, &my);

            // Check against all active explosions (from player missiles or other exploding items)
            for (uint32_t j = 0; j < CONFIG_MAX_TOTAL_MISSILES; j++) {
                missile_t *exp = missiles + j;
                if (missile_is_exploding(exp)) {
                    if (missile_is_colliding(exp, mx, my)) {
                        missile_explode(m);
                        break;
                    }
                }
            }
        }
    }
	// M2: Check for flying plane collision with an explosion.
	if (plane_is_flying()){
		coord_t plane_x, plane_y;
        plane_get_pos(&plane_x, &plane_y);
        for (uint32_t j = 0; j < CONFIG_MAX_TOTAL_MISSILES; j++) {
            missile_t *exp = missiles + j;
            if (missile_is_exploding(exp)) {
                if (missile_is_colliding(exp, plane_x, plane_y)) {
                    plane_explode();
                    break;
                }
            }
        }
	}
	// M2: Count non-player impacted missiles
	for (uint32_t i = 0; i < CONFIG_MAX_ENEMY_MISSILES; i++) {
        if (missile_is_impacted(enemy_missiles + i)) {
            ENEMY_IMPACT_COUNT++;
        }
    }
	if (missile_is_impacted(plane_missile)) {
        ENEMY_IMPACT_COUNT++;
    }
	// M2: Draw stats
	char stats_buf[64];
    snprintf(stats_buf, sizeof(stats_buf), "Shots: %u  Impacted: %u", (unsigned int)PLAYER_SHOT_COUNT, (unsigned int)ENEMY_IMPACT_COUNT);
    lcd_drawString(STATS_SHOT_X, STATS_TEXT_Y, stats_buf, CONFIG_COLOR_STATUS);
}
