#include "game.h"

/* Define the game states matching the state diagram */
typedef enum {
    init_st,
    new_game_st,
    wait_mark_st,
    mark_st,
    draw_message_st,
    wait_release_st,
    wait_restart_st,
    wait_start_release_st
} game_state_t;

/* Pending message buffer to defer heavy text draws */
typedef enum {
    MSG_NONE,
    MSG_X_WINS,
    MSG_O_WINS,
    MSG_DRAW,
    MSG_NEXT_O,
    MSG_NEXT_X
} pending_msg_t;

/* Static variables for the game state machine */
static game_state_t currentState = init_st;
static mark_t current_player = X_m;
static int8_t prev_row = -1;
static int8_t prev_col = -1;
static pending_msg_t pending_msg = MSG_NONE;

/**
 * @brief Initialize the game state machine and reset state variables.
 */
void game_init(void) {
    currentState = init_st;
    current_player = X_m;
    prev_row = -1;
    prev_col = -1;
    pending_msg = MSG_NONE;
}

/**
 * @brief Execute one tick cycle of the game state machine.
 */
void game_tick(void) {
    int8_t row = 0;
    int8_t col = 0;

    /* 1. State transitions & Discrete Actions */
    switch (currentState) {
        case init_st: {
            currentState = new_game_st;
            break;
        }
        case new_game_st: {
            // Mealy Action: Clear buffer, reset screen, board, and player turn
            uint8_t dummy;
            while (com_read(&dummy, 1) > 0) {}

            lcd_fillScreen(CONFIG_BACK_CLR);
            graphics_drawGrid(GAME_GRID_COLOR);
            board_clear();
            current_player = X_m;
            nav_set_loc(GAME_CENTER_ROW, GAME_CENTER_COL);
            prev_row = GAME_CENTER_ROW;
            prev_col = GAME_CENTER_COL;
            
            pending_msg = MSG_NEXT_X;
            currentState = draw_message_st;
            break;
        }
        case wait_mark_st: {
            // Check inputs for transition: Local Button A or Network Receive
            if (pin_get_level(HW_BTN_A) == GAME_PIN_PRESSED_LEVEL) {
                nav_get_loc(&row, &col);
                if (board_get(row, col) == no_m) {
                    uint8_t move_byte = ((row & 0x0F) << 4) | (col & 0x0F);
                    com_write(&move_byte, 1);
                    currentState = mark_st;
                }
            } 
            else {
                uint8_t rx_byte = 0;
                if (com_read(&rx_byte, 1) > 0) {
                    int8_t rx_row = (rx_byte >> 4) & 0x0F;
                    int8_t rx_col = rx_byte & 0x0F;

                    if (board_get(rx_row, rx_col) == no_m) {
                        nav_set_loc(rx_row, rx_col);
                        currentState = mark_st;
                    }
                }
            }
            break;
        }
        case mark_st: {
            nav_get_loc(&row, &col);
            board_set(row, col, current_player);
            
            // Draw piece on display for this tick
            if (current_player == X_m) {
                graphics_drawX(row, col, GAME_X_COLOR);
            } else {
                graphics_drawO(row, col, GAME_O_COLOR);
            }

            // Determine winner/draw and schedule text message for NEXT tick
            if (board_winner(X_m)) {
                pending_msg = MSG_X_WINS;
            } else if (board_winner(O_m)) {
                pending_msg = MSG_O_WINS;
            } else if (board_mark_count() == CONFIG_BOARD_SPACES) {
                pending_msg = MSG_DRAW;
            } else {
                if (current_player == X_m) {
                    current_player = O_m;
                    pending_msg = MSG_NEXT_O;
                } else {
                    current_player = X_m;
                    pending_msg = MSG_NEXT_X;
                }
            }
            currentState = draw_message_st;
            break;
        }
        case draw_message_st: {
            // Render text on its own dedicated tick cycle
            switch (pending_msg) {
                case MSG_X_WINS:
                    graphics_drawMessage("Player X Wins!", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
                    currentState = wait_restart_st;
                    break;
                case MSG_O_WINS:
                    graphics_drawMessage("Player O Wins!", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
                    currentState = wait_restart_st;
                    break;
                case MSG_DRAW:
                    graphics_drawMessage("Draw Game!", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
                    currentState = wait_restart_st;
                    break;
                case MSG_NEXT_O:
                    graphics_drawMessage("Player O Turn", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
                    currentState = wait_release_st;
                    break;
                case MSG_NEXT_X:
                    graphics_drawMessage("Player X Turn", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
                    currentState = wait_release_st;
                    break;
                default:
                    currentState = wait_mark_st;
                    break;
            }
            pending_msg = MSG_NONE;
            break;
        }
        case wait_release_st: {
            if (pin_get_level(HW_BTN_A) != GAME_PIN_PRESSED_LEVEL) {
                currentState = wait_mark_st;
            }
            break;
        }
        case wait_restart_st: {
            if (pin_get_level(HW_BTN_START) == GAME_PIN_PRESSED_LEVEL) {
                currentState = wait_start_release_st;
            }
            break;
        }
        case wait_start_release_st: {
            if (pin_get_level(HW_BTN_START) != GAME_PIN_PRESSED_LEVEL) {
                currentState = new_game_st;
            }
            break;
        }
        default: {
            currentState = init_st;
            break;
        }
    }

    /* 2. Continuous / Passive State Actions */
    switch (currentState) {
        case wait_mark_st: {
            // Joystick tracking: Only erase old cursor & draw new cursor.
            // DO NOT redraw the entire board grid or pieces.
            nav_tick();
            nav_get_loc(&row, &col);
            
            if (row != prev_row || col != prev_col) {
                graphics_drawHighlight(prev_row, prev_col, CONFIG_BACK_CLR);
                graphics_drawHighlight(row, col, CONFIG_HIGH_CLR);
                prev_row = row;
                prev_col = col;
            }
            break;
        }
        default:
            break;
    }
}