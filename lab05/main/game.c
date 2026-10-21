#include "game.h"
#include "board.h"
#include "graphics.h"
#include "nav.h"
#include "pin.h"
#include "hw.h"
#include "config.h"
#include "lcd.h"

#define GAME_CENTER_ROW (CONFIG_BOARD_R / 2)
#define GAME_CENTER_COL (CONFIG_BOARD_C / 2)
#define GAME_PIN_PRESSED_LEVEL 0

/* Define game and text colors */
#define GAME_GRID_COLOR WHITE
#define GAME_X_COLOR RED
#define GAME_O_COLOR BLUE
#define GAME_TEXT_COLOR WHITE
#define GAME_BG_COLOR BLACK

/* Define the game states matching the state diagram */
typedef enum {
    init_st,
    new_game_st,
    wait_mark_st,
    mark_st,
    wait_release_st,
    wait_restart_st,
    wait_start_release_st
} game_state_t;

/* Static variables for the game state machine */
static game_state_t current_state = init_st;
static mark_t current_player = X_m;
static int8_t prev_row = -1;
static int8_t prev_col = -1;

/**
 * @brief Initialize the game state machine and reset state variables.
 * @param None
 * @return None
 */
void game_init(void) {
    /* Set the initial state to init_st */
    current_state = init_st;

    /* Reset the current player to X */
    current_player = X_m;
    prev_row = -1;
    prev_col = -1;
}

/**
 * @brief Execute one tick cycle of the game state machine handling transitions and actions.
 * @param None
 * @return None
 */
void game_tick(void) {
    int8_t row = 0;
    int8_t col = 0;

    /* Handle state transitions based on current state and inputs */
    switch (current_state) {
        /* Transition from init to new game */
        case init_st: {
            current_state = new_game_st;
            break;
        }
        /* Transition from new game to waiting for a mark */
        case new_game_st: {
            current_state = wait_mark_st;
            break;
        }
        /* Transition from waiting for a mark to marking the board */
        case wait_mark_st: {
            /* Check if Button A is pressed using pin level reading */
            if (pin_get_level(HW_BTN_A) == GAME_PIN_PRESSED_LEVEL) {
                nav_get_loc(&row, &col);
                /* Verify if the selected location is empty/valid before transitioning */
                if (board_get(row, col) == no_m) {
                    current_state = mark_st;
                }
            }
            break;
        }
        /* Transition from marking to restart, wait release, or back to game */
        case mark_st: {
            /* Check if either player won or if the board is full (draw) */
            if (board_winner(X_m) || board_winner(O_m) || (board_mark_count() == CONFIG_BOARD_SPACES)) {
                current_state = wait_restart_st;
            } else {
                current_state = wait_release_st;
            }
            break;
        }
        /* Transition from wait release to waiting for next mark */
        case wait_release_st: {
            /* Wait for Button A to be released before allowing the next turn */
            if (pin_get_level(HW_BTN_A) != GAME_PIN_PRESSED_LEVEL) {
                current_state = wait_mark_st;
            }
            break;
        }
        /* Transition from waiting for restart to waiting for start button release */
        case wait_restart_st: {
            /* Check if the Start Button is pressed using pin level reading */
            if (pin_get_level(HW_BTN_START) == GAME_PIN_PRESSED_LEVEL) {
                current_state = wait_start_release_st;
            }
            break;
        }
        /* Transition from wait start release to new game */
        case wait_start_release_st: {
            /* Wait for Start Button release before triggering new game setup */
            if (pin_get_level(HW_BTN_START) != GAME_PIN_PRESSED_LEVEL) {
                current_state = new_game_st;
            }
            break;
        }
        /* Default case to catch unexpected states */
        default: {
            current_state = init_st;
            break;
        }
    }

    /* Handle state actions based on the current state */
    switch (current_state) {
        /* No action in init state */
        case init_st: {
            break;
        }
        /* Clear screen, draw grid, clear board, reset navigator, and show next player */
        case new_game_st: {
            lcd_fillScreen(GAME_BG_COLOR);
            graphics_drawGrid(GAME_GRID_COLOR);
            board_clear();
            current_player = X_m;
            nav_set_loc(GAME_CENTER_ROW, GAME_CENTER_COL);
            prev_row = GAME_CENTER_ROW;
            prev_col = GAME_CENTER_COL;
            graphics_drawMessage("Player X Turn", GAME_TEXT_COLOR, GAME_BG_COLOR);
            break;
        }
        /* Update joystick navigator position, clear old highlight, and draw new highlight */
        case wait_mark_st: {
            nav_tick();
            nav_get_loc(&row, &col);
            
            /* If position changed, clear the old highlight box by redrawing grid/background */
            if (row != prev_row || col != prev_col) {
                graphics_drawGrid(GAME_GRID_COLOR);
                // Redraw any existing marks that might have been crossed by grid lines
                for (int8_t r = 0; r < CONFIG_BOARD_R; r++) {
                    for (int8_t c = 0; c < CONFIG_BOARD_C; c++) {
                        if (board_get(r, c) == X_m) {
                            graphics_drawX(r, c, GAME_X_COLOR);
                        } else if (board_get(r, c) == O_m) {
                            graphics_drawO(r, c, GAME_O_COLOR);
                        }
                    }
                }
                prev_row = row;
                prev_col = col;
            }
            
            graphics_drawHighlight(row, col, GAME_GRID_COLOR);
            break;
        }
        /* Place mark on board, draw X/O, check win/draw, and update player turn */
        case mark_st: {
            nav_get_loc(&row, &col);
            board_set(row, col, current_player);
            
            /* Draw appropriate mark based on current player */
            if (current_player == X_m) {
                graphics_drawX(row, col, GAME_X_COLOR);
            } else {
                graphics_drawO(row, col, GAME_O_COLOR);
            }

            /* Check if game is over and display winner or draw, otherwise switch player */
            if (board_winner(current_player)) {
                if (current_player == X_m) {
                    graphics_drawMessage("Player X Wins!", GAME_TEXT_COLOR, GAME_BG_COLOR);
                } else {
                    graphics_drawMessage("Player O Wins!", GAME_TEXT_COLOR, GAME_BG_COLOR);
                }
            } else if (board_mark_count() == CONFIG_BOARD_SPACES) {
                graphics_drawMessage("Draw Game!", GAME_TEXT_COLOR, GAME_BG_COLOR);
            } else {
                /* Alternate player turn between X and O */
                if (current_player == X_m) {
                    current_player = O_m;
                    graphics_drawMessage("Player O Turn", GAME_TEXT_COLOR, GAME_BG_COLOR);
                } else {
                    current_player = X_m;
                    graphics_drawMessage("Player X Turn", GAME_TEXT_COLOR, GAME_BG_COLOR);
                }
            }
            break;
        }
        /* No active actions while waiting for button release */
        case wait_release_st: {
            break;
        }
        /* No active actions required while waiting for restart */
        case wait_restart_st: {
            break;
        }
        /* No active actions while waiting for start button release */
        case wait_start_release_st: {
            break;
        }
        /* Default case for safety */
        default: {
            break;
        }
    }
}