#include "xbasic_types.h"
#include "xgpio.h"
#include "xparameters.h"
#include "stdbool.h"
#include "math.h"

#define BRICK_HEIGHT 40
#define BRICK_WIDTH 80
#define BALL_WIDTH 10
#define BALL_HEIGHT BALL_WIDTH
#define INTERSECTION_CHECK_DEPTH 10

#define BALL_MIN_SPEED 1
#define BALL_MAX_SPEED 10
#define BALL_PADDLE_BOUNCE_SPEED_CHANGE 5

#define PADDLE_HEIGHT 10
#define PADDLE_WIDTH 120
#define PADDLE_SPEED 6

#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640

#define POINTS_FOR_BREAKING_BRICK 1

Xuint32* baseaddr_p_vga = (Xuint32*)XPAR_VGA_CONTROLLER_0_S00_AXI_BASEADDR;
Xuint32* baseaddr_p_tc  = (Xuint32*)XPAR_TIMER_COUNTER_0_S00_AXI_BASEADDR;

float max(float a, float b) {
	return a > b ? a : b;
}

float min(float a, float b) {
	return a < b ? a : b;
}

float clamp(float x, float a, float b) {
	return x < a ? a : x > b ? b : x;
}

int sign(float x) {
	 return x < 0 ? -1 : x > 0 ? 1 : 0;
}

int getDigit(int x, int digit) {
	while (digit > 0) {
		x /= 10;
		digit--;
	}
	return x % 10;
}

int sevenSegmentBitsForDisplay(int digit) {
	switch (digit) {
	case 0: return 0b11000000;
	case 1: return 0b11111001;
	case 2: return 0b10100100;
	case 3: return 0b10110000;
	case 4: return 0b10011001;
	case 5: return 0b10010010;
	case 6: return 0b10000010;
	case 7: return 0b11111000;
	case 8: return 0b10000000;
	case 9: return 0b10010000;
	default: return 0x00;
	}
}

int main() {
	Xuint32* reg0 = baseaddr_p_vga + 0;
	Xuint32* reg1 = baseaddr_p_vga + 1;
	Xuint32* reg2 = baseaddr_p_vga + 2;
	Xuint32* reg3 = baseaddr_p_vga + 3;

	Xuint32* divide_const = baseaddr_p_tc + 0;
	Xuint32* divided_clk  = baseaddr_p_tc + 1;

	XGpio xgpio;
	XGpio_Initialize(&xgpio, 0);
	XGpio_SetDataDirection(&xgpio, 1, 0x0F);

	XGpio xgpio_7seg;
	XGpio_Initialize(&xgpio_7seg, 1);
	XGpio_SetDataDirection(&xgpio_7seg, 1, 0x0); // 4 anodes
	XGpio_SetDataDirection(&xgpio_7seg, 2, 0x00); // 8 segments including the dot
	u32 btn;

	float ballX;
	float ballY;
	float velX;
	float velY;
	float paddleX;
	float unit;
	int gameScore;
	int strobingDigit = 0;

setup:
	*reg0 = 0xFFFFFFFF; // Initially, all bricks are set

	gameScore = 0;
	ballX = 320;
	ballY = 320;
	*reg1 = ballX;        // Set initial ball x to halfway through the screen
	*reg2 = ballY;        // Set initial ball y near the bottom
	paddleX = 320;
	*reg3 = paddleX;        // Set initial paddle x to halfway through the screen
	//*divide_const = 10000000; // 100MHz / 10000000 = divided_clk increases by 10 per second
	*divide_const = 200000;
    unit = (float)*divide_const / 2000000;

	velX = 3 * unit;
	velY = -5 * unit;

	// Game controls
	bool hasDied = false;
	bool hasWon = false;
	unsigned long int last_counter = *divided_clk;

	// Main game loop
	while (!hasDied && !hasWon) {
		const float paddleY = 420;
		for (int i = 0; i < 32; i++) {
			if (!(*reg0 & (1 << i))) {
				continue;
			}
			int brickX = (i % 8) * BRICK_WIDTH;
			int brickY = (i / 8) * BRICK_HEIGHT;
			bool destroyed = false;
			// top edge of brick
			if (brickX <= ballX + BALL_WIDTH && ballX <= brickX + BRICK_WIDTH && ballY + BALL_HEIGHT >= brickY && ballY + BALL_HEIGHT <= brickY + INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velY = -fabs(velY);
			}
			// bottom edge of brick
			if (brickX <= ballX + BALL_WIDTH && ballX <= brickX + BRICK_WIDTH && ballY <= brickY + BRICK_HEIGHT && ballY >= brickY + BRICK_HEIGHT - INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velY = fabs(velY);
			}
			// left edge of brick
			if (brickY <= ballY + BALL_HEIGHT && ballY <= brickY + BRICK_HEIGHT && ballX + BALL_WIDTH >= brickX && ballX + BALL_WIDTH <= brickX + INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velX = -fabs(velX);
			}
			// right edge of brick
			if (brickY <= ballY + BALL_HEIGHT && ballY <= brickY + BRICK_HEIGHT && ballX <= brickX + BRICK_WIDTH && ballX >= brickX + BRICK_WIDTH - INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velX = fabs(velX);
			}

			if (destroyed) {
				gameScore += POINTS_FOR_BREAKING_BRICK * (4 - i / 8);
				*reg0 &= ~(1 << i);
			}
		}
		// paddle collision
		if (velY > 0 && paddleX <= ballX + BALL_WIDTH && ballX <= paddleX + PADDLE_WIDTH && ballY + BALL_HEIGHT >= paddleY && ballY <= paddleY + PADDLE_HEIGHT) {
			velY = -fabs(velY);
			// Make the ball bounce in a way that depends on where on the paddle the collision happened.
			// This ranges from -1 on a left-edge bounce to 1 on a right-edge bounce
			float relativeBouncePosition = clamp(ballX + BALL_WIDTH / 2 - paddleX, 0, PADDLE_WIDTH) / PADDLE_WIDTH * 2 - 1;
			velX = velX + relativeBouncePosition * BALL_PADDLE_BOUNCE_SPEED_CHANGE;
			if (fabs(velX) < BALL_MIN_SPEED) {
				velX = velX <= 0 ? -BALL_MIN_SPEED : BALL_MIN_SPEED;
			}
			velX = clamp(velX, -BALL_MAX_SPEED, BALL_MAX_SPEED);
			velX *= unit;
		}

		hasWon = !*reg0;

		unsigned long int counter = *divided_clk;
		unsigned long int counts_increased = counter - last_counter; // this should be correct even if divided_clk has overflowed
		last_counter = counter;

		btn = XGpio_DiscreteRead(&xgpio, 1);

		// Logic for applying velocity
		if (counts_increased) {
			ballX += velX;
			ballY += velY;

			if (btn & 0b0100) {
				paddleX = min(SCREEN_WIDTH - PADDLE_WIDTH, paddleX + (PADDLE_SPEED * unit));
			}
			if (btn & 0b0010) {
				paddleX = max(0, paddleX - (PADDLE_SPEED * unit));
			}

			// Showing the score on the 7-segment display
			strobingDigit = (strobingDigit + 1) % 4;
			XGpio_DiscreteWrite(&xgpio_7seg, 1, 0b1111 ^ (1 << strobingDigit));
			XGpio_DiscreteWrite(&xgpio_7seg, 2, sevenSegmentBitsForDisplay(getDigit(gameScore, strobingDigit)));
		}

		if (ballX > SCREEN_WIDTH - BALL_WIDTH) {
			ballX = SCREEN_WIDTH - BALL_WIDTH;
			velX = -fabs(velX);
		}
		else if (ballX < 0) {
			ballX = 0;
			velX = fabs(velX);
		}
		if (ballY > SCREEN_HEIGHT - BALL_HEIGHT) {
			ballY = SCREEN_HEIGHT - BALL_HEIGHT;
			velY = -fabs(velY);
		}
		else if (ballY < 0) {
			ballY = 0;
			velY = fabs(velY);
		}

		hasDied = *reg2 > 450;
		*reg1 = (int)ballX;
		*reg2 = (int)ballY;
		*reg3 = (int)paddleX;
	}

	// Win or lose logic
	while (hasDied || hasWon) {
		btn = XGpio_DiscreteRead(&xgpio, 1);
		if (btn & 0b1001) {
			goto setup;
		}

		unsigned long int counter = *divided_clk;
		unsigned long int counts_increased = counter - last_counter; // this should be correct even if divided_clk has overflowed
		last_counter = counter;
		if (counts_increased) {
			// Showing the score on the 7-segment display
			strobingDigit = (strobingDigit + 1) % 4;
			XGpio_DiscreteWrite(&xgpio_7seg, 1, 0b1111 ^ (1 << strobingDigit));
			XGpio_DiscreteWrite(&xgpio_7seg, 2, sevenSegmentBitsForDisplay(getDigit(gameScore, strobingDigit)));
		}
	}

	return 0;
}
