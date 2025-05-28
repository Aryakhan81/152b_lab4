#include "xbasic_types.h"
#include "xgpio.h"
#include "xparameters.h"
#include "stdbool.h"
#include "stdlib.h"

#define BRICK_HEIGHT 40
#define BRICK_WIDTH 80
#define BALL_WIDTH 10
#define BALL_HEIGHT BALL_WIDTH
#define INTERSECTION_CHECK_DEPTH 10

#define BALL_MAX_SPEED 10
#define BALL_PADDLE_BOUNCE_SPEED_CHANGE 3

#define PADDLE_HEIGHT 10
#define PADDLE_WIDTH 120
#define PADDLE_SPEED 6

#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640

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
	u32 btn;

	float ballX;
	float ballY;
	float velX;
	float velY;
	float paddleX;
	float unit;

setup:
	*reg0 = 0xFFFFFFFF; // Initially, all bricks are set

	ballX = 320;
	ballY = 320;
	*reg1 = ballX;        // Set initial ball x to halfway through the screen
	*reg2 = ballY;        // Set initial ball y near the bottom
	paddleX = 320;
	*reg3 = paddleX;        // Set initial paddle x to halfway through the screen
	//*divide_const = 10000000; // 100MHz / 10000000 = divided_clk increases by 10 per second
	*divide_const = 500000;
    unit = (float)*divide_const / 2000000;

	velX = -3 * unit;
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
				velY = -abs(velY);
			}
			// bottom edge of brick
			if (brickX <= ballX + BALL_WIDTH && ballX <= brickX + BRICK_WIDTH && ballY <= brickY + BRICK_HEIGHT && ballY >= brickY + BRICK_HEIGHT - INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velY = abs(velY);
			}
			// left edge of brick
			if (brickY <= ballY + BALL_HEIGHT && ballY <= brickY + BRICK_HEIGHT && ballX + BALL_WIDTH >= brickX && ballX + BALL_WIDTH <= brickX + INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velX = -abs(velX);
			}
			// right edge of brick
			if (brickY <= ballY + BALL_HEIGHT && ballY <= brickY + BRICK_HEIGHT && ballX <= brickX + BRICK_WIDTH && ballX >= brickX + BRICK_WIDTH - INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velX = abs(velX);
			}

			if (destroyed) {
				*reg0 &= ~(1 << i);
			}
		}
		// paddle collision
		if (velY > 0 && paddleX <= ballX + BALL_WIDTH && ballX <= paddleX + PADDLE_WIDTH && ballY + BALL_HEIGHT >= paddleY && ballY <= paddleY + PADDLE_HEIGHT) {
			velY = -abs(velY);
			// Make the ball bounce in a way that depends on where on the paddle the collision happened.
			// This ranges from -1 on a left-edge bounce to 1 on a right-edge bounce
			float relativeBouncePosition = clamp(ballX + BALL_WIDTH / 2 - paddleX, 0, PADDLE_WIDTH) / PADDLE_WIDTH * 2 - 1;
			velX = velX + relativeBouncePosition * BALL_PADDLE_BOUNCE_SPEED_CHANGE;
			velX = clamp(velX, -BALL_MAX_SPEED, BALL_MAX_SPEED);
			velX *= unit;
		}

		hasWon = !*reg0;

		unsigned long int counter = *divided_clk;
		unsigned long int counts_increased = counter - last_counter; // this should be correct even if divided_clk has overflowed
		last_counter = counter;

		btn = XGpio_DiscreteRead(&xgpio, 1);

		// Logic for applying velocity
		// This should run 10 times per second
		if (counts_increased) {
			ballX += velX;
			ballY += velY;

			if (btn & 0b0100) {
				paddleX = min(SCREEN_WIDTH - PADDLE_WIDTH, paddleX + (PADDLE_SPEED * unit));
			}
			if (btn & 0b0010) {
				paddleX = max(0, paddleX - (PADDLE_SPEED * unit));
			}
		}

		if (ballX > SCREEN_WIDTH - BALL_WIDTH) {
			ballX = SCREEN_WIDTH - BALL_WIDTH;
			velX = -abs(velX);
		}
		else if (ballX < 0) {
			ballX = 0;
			velX = abs(velX);
		}
		if (ballY > SCREEN_HEIGHT - BALL_HEIGHT) {
			ballY = SCREEN_HEIGHT - BALL_HEIGHT;
			velY = -abs(velY);
		}
		else if (ballY < 0) {
			ballY = 0;
			velY = abs(velY);
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
	}

	return 0;
}
