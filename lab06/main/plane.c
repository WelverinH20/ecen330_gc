#include "plane.h"
#include "config.h"
#include "lcd.h"
#include "missile.h"
#include <stdio.h>
#include <stddef.h>

// Plane State Machine States
enum plane_state_t {
    INIT_ST,
    IDLE_ST,
    FLYING_ST
};


// Plane State Machine Variables
struct plane_t{
    enum plane_state_t current_state;
    coord_t x;
    coord_t y;
    uint32_t  idle_counter; //track respawn ticks
    bool missile_launched;   //Ensures 1 missle per flight
    bool explode_me;        //Flag to  indicate plane should explode
};

// Plane State Machine Instance, File scope only
static struct plane_t plane;
static missile_t *plane_missile_pointer = NULL;

// Plane State machine init function
void plane_init(missile_t *plane_missile){
    plane_missile_pointer =  plane_missile;
    plane.current_state = INIT_ST;
    plane.x = LCD_W;
    plane.y = LCD_H/4;
    plane.idle_counter = 0;
    plane.missile_launched = false;
    plane.explode_me = false;
}

// Plane explode helper functin

void plane_explode(void){
    plane.explode_me = true;
}

//  Plane tick function
void plane_tick(void){
    /* State Transition Processing */
    switch (plane.current_state){
        
        // Transition out of init on first tick
        case INIT_ST:
            plane.current_state = IDLE_ST;
            break;

        // Wait for respawn timer to expire (4 seconds default)
        case IDLE_ST:
            if (plane.idle_counter >= CONFIG_PLANE_IDLE_TIME_SECONDS){
                plane.current_state =  FLYING_ST;
            };
            break;
        
        // Transition back to idle if plane is destoryed or off screen
        case FLYING_ST:
            if (plane.explode_me || plane.x < 0){
                plane.current_state = IDLE_ST;
            };
            break;
        
        // basic error handler.
        default:
            plane.current_state =  INIT_ST;
            break;
    }

    /* State Actions Processor */
    switch (plane.current_state){
        // No actions in init yet
        case INIT_ST:
            break;
        
        // Handle idle timing
        //TODO verify idle counter is  incramenting propperly
        case IDLE_ST:
            if (plane.idle_counter ==  0){
                plane.x = LCD_W;
                plane.y = LCD_H/4;
                plane.missile_launched = false;
                plane.explode_me = false;
            }
            plane.idle_counter++;
            break;

        // Flying logic
        case FLYING_ST:
            // Reset idle counter
            plane.idle_counter = 0;
            //Move plane position based on speed set in config
            plane.x -= CONFIG_PLANE_DISTANCE_PER_TICK;

            //draw plane
            lcd_fillTriangle(plane.x, plane.y,
                             plane.x + CONFIG_PLANE_WIDTH, plane.y - CONFIG_PLANE_HEIGHT / 2, 
                             plane.x + CONFIG_PLANE_WIDTH, plane.y + CONFIG_PLANE_HEIGHT / 2, 
                             CONFIG_COLOR_PLANE);

            // Launch the single plane missile when reaching roughly halfway across the screen[cite: 9, 15]
            if (!plane.missile_launched && plane.x <= LCD_W / 2) {
                if (plane_missile_pointer != NULL && missile_is_idle(plane_missile_pointer)) {
                    missile_launch_plane(plane_missile_pointer, plane.x, plane.y);
                    plane.missile_launched = true;
                }
            }
            break;
        default:
            break;
    }

}


void plane_get_pos(coord_t *x, coord_t *y) {
    *x = plane.x;
    *y = plane.y;
}

bool plane_is_flying() {
    return plane.current_state == FLYING_ST;
}