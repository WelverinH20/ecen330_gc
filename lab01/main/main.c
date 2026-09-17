#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include "pac.h"

static const char *TAG = "lab01";

#define DELAY_MS(ms) \
	vTaskDelay(((ms)+(portTICK_PERIOD_MS-1))/portTICK_PERIOD_MS)

// Main display constants
#define BACKGROUND_CLR rgb565(0,60,90)
#define TITLE_CLR GREEN
#define STATUS_CLR WHITE
#define STR_BUF_LEN 12 // string buffer length
#define FONT_SIZE 2
#define FONT_W (LCD_CHAR_W*FONT_SIZE)
#define FONT_H (LCD_CHAR_H*FONT_SIZE)
#define STATUS_W (FONT_W*3)

#define WAIT 2000 // milliseconds
#define DELAY_EX3 20 // milliseconds

// Object position and movement
#define OBJ_X 100
#define OBJ_Y 100
#define OBJ_MOVE 3 // pixels



//----------------------------------------------------------------------------//
// Car Implementation - Begin
//----------------------------------------------------------------------------//

// Car constants
#define CAR_CLR rgb565(220,30,0)
#define WINDOW_CLR rgb565(180,210,238)
#define TIRE_CLR BLACK
#define HUB_CLR GRAY

#define CAR_W 60
#define CAR_H 32

#define BODY_X0 0
#define BODY_Y0 12
#define BODY_X1 59
#define BODY_Y1 24

#define UPPER_BODY_X0 1
#define UPPER_BODY_Y0 0
#define UPPER_BODY_X1 39
#define UPPER_BODY_Y1 11

#define WINDOW_RADIUS 2

#define WINDOW_LEFT_X0 3
#define WINDOW_LEFT_Y0 1
#define WINDOW_LEFT_X1 18
#define WINDOW_LEFT_Y1 8

#define WINDOW_RIGHT_X0 21
#define WINDOW_RIGHT_Y0 1
#define WINDOW_RIGHT_X1 37
#define WINDOW_RIGHT_Y1 8

#define HOOD_X0 40
#define HOOD_Y0 9
#define HOOD_X1 40
#define HOOD_Y1 11
#define HOOD_X2 59
#define HOOD_Y2 11

#define TIRE_RADIUS 7
#define HUB_RADIUS 4

#define TIRE_LEFT_X0 11
#define TIRE_LEFT_Y0 24
#define TIRE_RIGHT_X0 48
#define TIRE_RIGHT_Y0 24

/**
 * @brief Draw a car at the specified location.
 * @param x      Top left corner X coordinate.
 * @param y      Top left corner Y coordinate.
 * @details Draw the car components relative to the anchor point (top, left).
 */
void drawCar(coord_t x, coord_t y)
{
	// TODO: Implement car procedurally with lcd geometric primitives.

	// Create Lower Body
	lcd_fillRect(x + BODY_X0, y + BODY_Y0, (BODY_X1-BODY_X0 + 1), (BODY_Y1-BODY_Y0 + 1), CAR_CLR);
	// Create Upper Body
	lcd_fillRect(x + UPPER_BODY_X0, y + UPPER_BODY_Y0, (UPPER_BODY_X1-UPPER_BODY_X0 + 1), (UPPER_BODY_Y1-UPPER_BODY_Y0 + 1), CAR_CLR);
	// Create Windows
	lcd_fillRoundRect(x + WINDOW_LEFT_X0, y + WINDOW_LEFT_Y0,(WINDOW_LEFT_X1 - WINDOW_LEFT_X0 + 1), (WINDOW_LEFT_Y1-WINDOW_LEFT_Y0 + 1),WINDOW_RADIUS, WINDOW_CLR);
	lcd_fillRoundRect(x + WINDOW_RIGHT_X0, y + WINDOW_RIGHT_Y0, (WINDOW_RIGHT_X1 - WINDOW_RIGHT_X0 + 1), (WINDOW_RIGHT_Y1-WINDOW_RIGHT_Y0 + 1), WINDOW_RADIUS, WINDOW_CLR);
	// Create Hood
	lcd_fillTriangle(x + HOOD_X0, y + HOOD_Y0, x + HOOD_X1, y + HOOD_Y1, x + HOOD_X2, y + HOOD_Y2, CAR_CLR);
	// Create Tires
	lcd_fillCircle(x + TIRE_LEFT_X0, y + TIRE_LEFT_Y0, TIRE_RADIUS, TIRE_CLR);
	lcd_fillCircle(x + TIRE_RIGHT_X0, y + TIRE_RIGHT_Y0, TIRE_RADIUS, TIRE_CLR);
	lcd_fillCircle(x + TIRE_LEFT_X0, y + TIRE_LEFT_Y0, HUB_RADIUS, HUB_CLR);
	lcd_fillCircle(x + TIRE_RIGHT_X0, y + TIRE_RIGHT_Y0, HUB_RADIUS, HUB_CLR);

}

//----------------------------------------------------------------------------//
// Car Implementation - End
//----------------------------------------------------------------------------//


// Application main
void app_main(void)
{
	ESP_LOGI(TAG, "Start up");
	lcd_init();
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_setFontSize(FONT_SIZE);
	lcd_drawString(0, 0, "Hello World! (lcd)", TITLE_CLR);
	printf("Hello World! (terminal)\n");
	DELAY_MS(WAIT);
	// TODO: Exercise 1 - Draw car in one location.
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_drawString(2,5, "Exercise 1", TITLE_CLR);
	drawCar(OBJ_X, OBJ_Y);

	// TODO: Exercise 2 - Draw moving car (Method 1), one pass across display.
	// Clear the entire display and redraw all objects each iteration.
	// Use a loop and increment x by OBJ_MOVE each iteration.
	// Start x off screen (negative coordinate).
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		// Clear entire dipslay
		lcd_fillScreen(BACKGROUND_CLR);
		// Draw title string
		lcd_drawString(2,5, "Exercise 2", TITLE_CLR);
		char pos_str[20];
		snprintf(pos_str, sizeof(pos_str), "%ld", x);
		lcd_drawString(2, LCD_H - 24, pos_str, STATUS_CLR);
		// Draw car at current x and y cord
		drawCar(x, OBJ_Y);
		// Animation delay
		DELAY_MS(DELAY_EX3);
	}
	// TODO: Exercise 3 - Draw moving car (Method 2), one pass across display.
	// Move by erasing car at old position, then redrawing at new position.
	// Objects that don't change or move are drawn once.
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_drawString(2,5, "Exercise 3", TITLE_CLR);
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		// Draw car at current x and y cord
		char pos_str[20];
		snprintf(pos_str, sizeof(pos_str), "%ld", x);
		lcd_fillRect(0,LCD_H-30,50,50,BACKGROUND_CLR);
		lcd_drawString(2, LCD_H - 24, pos_str, STATUS_CLR);
		drawCar(x, OBJ_Y);
		
		// Animation delay
		DELAY_MS(DELAY_EX3);
		// Erase car old position
		if (x < LCD_W)
		{
			lcd_fillRect(x, OBJ_Y, CAR_W, CAR_H, BACKGROUND_CLR);
		}
	}

	// TODO: Exercise 4 - Draw moving car (Method 3), one pass across display.
	// First, draw all objects into a cleared, off-screen frame buffer.
	// Then, transfer the entire frame buffer to the screen.
	lcd_frameEnable();
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		// clear the frame, set back ground color
		lcd_fillScreen(BACKGROUND_CLR);
		// write required string
		lcd_drawString(2,5, "Exercise 4", TITLE_CLR);
		// draw car to frame buffer at current x and y cord
		drawCar(x, OBJ_Y);
		// write buffer to display
		char pos_str[20];
		snprintf(pos_str, sizeof(pos_str), "%ld", x);
		lcd_drawString(2, LCD_H - 24, pos_str, STATUS_CLR);
		lcd_writeFrame();
	
		// delay for animation frame
		DELAY_MS(DELAY_EX3);
	}
	lcd_frameDisable();

	// TODO: Exercise 5 - Draw an animated Pac-Man moving across the display.
	// Use Pac-Man sprites instead of the car object.
	// Cycle through each sprite when moving the Pac-Man character.
	lcd_frameEnable();
	uint16_t i = 0;
	const uint8_t pidx[] = {0, 1, 2, 1};
	while (1)
	{
		for (coord_t x = -PAC_W; x <= LCD_W; x += OBJ_MOVE)
		{
			// Set background color and clear the screen
            lcd_fillRect(0, 0, LCD_W, LCD_H, BACKGROUND_CLR);
            // Draw required string
            lcd_drawString(2, 5, "Exercise 5", TITLE_CLR);
            // Get current sprite index
            uint8_t current_sprite_idx = pidx[i++ % sizeof(pidx)];
            // Draw Pac-Man sprite using the current sprite bitmap
            lcd_drawBitmap(x, OBJ_Y, pac[current_sprite_idx], PAC_W, PAC_H, YELLOW);
            // Display position string at bottom left
            char pos_str[20];
            snprintf(pos_str, sizeof(pos_str), "%ld", x);
            lcd_drawString(2, LCD_H - 24, pos_str, STATUS_CLR);
            // Write frame buffer to display
            lcd_writeFrame();
            // Animation frame delay
            DELAY_MS(DELAY_EX3);
		}
	}

}
