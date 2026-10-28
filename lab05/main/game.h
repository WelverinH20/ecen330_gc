#ifndef GAME_H_
#define GAME_H_


/* Include nessisary headers */
#include "board.h"
#include "graphics.h"
#include "nav.h"
#include "pin.h"
#include "hw.h"
#include "config.h"
#include "lcd.h"
#include "com.h"


// Define constants for game center and button press level
#define GAME_CENTER_ROW (CONFIG_BOARD_R / 2)
#define GAME_CENTER_COL (CONFIG_BOARD_C / 2)
#define GAME_PIN_PRESSED_LEVEL 0

/* Define game and text colors */
#define GAME_GRID_COLOR WHITE
#define GAME_X_COLOR RED
#define GAME_O_COLOR BLUE
#define GAME_TEXT_COLOR WHITE
#define GAME_BG_COLOR BLACK

// Initialize the game logic.
void game_init(void);

// Update the game logic.
void game_tick(void);

#endif // GAME_H_
