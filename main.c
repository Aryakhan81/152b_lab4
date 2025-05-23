#include "xbasic_types.h"
#include "xgpio.h"
#include "xparameters.h"
#include "stdbool.h"

#define BRICK_HEIGHT 40
#define BRICK_WIDTH 80
#define BALL_WIDTH 10
#define BALL_HEIGHT BALL_WIDTH
#define INTERSECTION_CHECK_DEPTH 10

Xuint32* baseaddr_p_vga = (Xuint32*)XPAR_VGA_CONTROLLER_0_S00_AXI_BASEADDR;
Xuint32* baseaddr_p_tc  = (Xuint32*)XPAR_TIMER_COUNTER_0_S00_AXI_BASEADDR;

int abs(int x) {
	return x < 0 ? -x : x;
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


	*reg0 = 0xFFFFFFFF; // Initially, all bricks are set
	*reg1 = 320;        // Set initial ball x to halfway through the screen
	*reg2 = 320;        // Set initial ball y near the bottom
	*reg3 = 320;        // Set initial paddle x to halfway through the screen
	//*divide_const = 10000000; // 100MHz / 10000000 = divided_clk increases by 10 per second
	*divide_const = 2000000;

	short int velX = -1;
	short int velY = -1;

	// Game controls
	bool hasDied = false;
	bool hasWon = false;
	unsigned long int last_counter = *divided_clk;

	// Main game loop
	while (!hasDied && !hasWon) {
		// TODO: Collision detection logic
		const int ballX = *reg1;
		const int ballY = *reg2;
		for (int i = 0; i < 32; i++) {
			if (!(*reg0 & (1 << i))) {
				continue;
			}
			int brickX = (i % 8) * BRICK_WIDTH;
			int brickY = (i / 8) * BRICK_HEIGHT;
			bool destroyed = false;
			// bottom edge of brick
			if (brickX <= ballX + BALL_WIDTH && ballX <= brickX + BRICK_WIDTH && ballY <= brickY + BRICK_HEIGHT && ballY >= brickY + BRICK_HEIGHT - INTERSECTION_CHECK_DEPTH) {
				destroyed = true;
				velY = abs(velY);
			}
			// TODO other brick edges

			if (destroyed) {
				*reg0 &= ~(1 << i);
			}
		}
		// TODO paddle collision

		hasWon = !*reg0;

		// TODO: Velocity change logic
		unsigned long int counter = *divided_clk;
		unsigned long int counts_increased = counter - last_counter; // this should be correct even if divided_clk has overflowed
		last_counter = counter;

		btn = XGpio_DiscreteRead(&xgpio, 1);

		// This should run 10 times per second
		if (counts_increased) {
			*reg1 += velX;
			*reg2 += velY;

			if (btn & 0b0100) {
				*reg3 += 5;
			}
			if (btn & 0b0010) {
				*reg3 -= 5;
			}
		}

		if ((int)*reg1 > 480) {
			velX = -1;
		}
		else if ((int)*reg1 < 0) {
			velX = 1;
		}
		if ((int)*reg2 > 640) {
			velY = -1;
		}
		else if ((int)*reg2 < 0) {
			velY = 1;
		}


		// Update ball positions with current velocity
		//if((int)(*reg1) + velX < 0) velX = -velX;
		//if((int)(*reg2) + velY < 0) velY = -velY;


		hasDied = *reg2 > 450;
	}

	// Lose logic
	if (hasDied) {

	}

	// Win logic
	if (hasWon) {

	}

	return 0;
}
