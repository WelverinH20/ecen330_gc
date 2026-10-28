#include "game.h"

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
            //move to next state
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
            // clear com buffer
            uint8_t dummy;
            while (com_read(&dummy, 1) > 0) {

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
            /* 1. Check if local Button A is pressed */
            if (pin_get_level(HW_BTN_A) == GAME_PIN_PRESSED_LEVEL) {
                nav_get_loc(&row, &col);
                /* Verify if the selected location is empty/valid before transitioning */
                if (board_get(row, col) == no_m) {
                    // Encode location: row in upper 4 bits, col in lower 4 bits
                    uint8_t move_byte = ((row & 0x0F) << 4) | (col & 0x0F);
                    com_write(&move_byte, 1);
                    
                    current_state = mark_st;
                }
            } 
            else {
                /* 2. Check if a move byte has been received from the remote controller */
                uint8_t rx_byte = 0;
                if (com_read(&rx_byte, 1) > 0) {
                    // Unpack row and column
                    row = (rx_byte >> 4) & 0x0F;
                    col = rx_byte & 0x0F;

                    // Check validity of the mark location on the receiving side
                    if (board_get(row, col) == no_m) {
                        // Temporarily update navigator location so mark_st uses the received coords
                        nav_set_loc(row, col);
                        current_state = mark_st;
                    }
                }
            }
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
}}