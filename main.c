#include "xbasic_types.h"
#include "xparameters.h"
#include "stdbool.h"

Xuint32* baseaddr_p_vga = (Xuint32*)XPAR_VGA_CONTROLLER_0_S00_AXI_BASEADDR;
Xuint32* baseaddr_p_tc  = (Xuint32*)XPAR_TIMER_COUNTER_0_S00_AXI_BASEADDR;

int main() {
	Xuint32* reg0 = baseaddr_p_vga + 0;
	Xuint32* reg1 = baseaddr_p_vga + 1;
	Xuint32* reg2 = baseaddr_p_vga + 2;
	Xuint32* reg3 = baseaddr_p_vga + 3;

	Xuint32* divide_const = baseaddr_p_tc + 0;
	Xuint32* divided_clk  = baseaddr_p_tc + 1;

    *reg0 = 0xFFFFFFFF; // Initially, all bricks are set
    *reg1 = 320;        // Set initial ball x to halfway through the screen
    *reg2 = 320;        // Set initial ball y near the bottom
    *reg3 = 320;        // Set initial paddle x to halfway through the screen
    *divide_const = 10000000; // 100MHz / 10000000 = divided_clk increases by 10 per second

    short int velX = -6;
    short int velY = -8;

    // Game controls
    bool hasDied = false;
    bool hasWon = false;
    unsigned long int last_counter = *divided_clk;

	// Main game loop
    while (!hasDied && !hasWon) {
        // TODO: Collision detection logic

        hasWon = !*reg0;

        // TODO: Velocity change logic
        unsigned long int counter = *divided_clk;
        unsigned long int counts_increased = counter - last_counter; // this should be correct even if divided_clk has overflowed
		
        // This should run 10 times per second
        if (counts_increased) {
        	*reg1 += velX;
        	*reg2 += velY;
        }

        if (*reg1 > 400) {
        	velX = -1;
        }
        else if (*reg1 < 200) {
        	velX = 1;
        }
        if (*reg2 > 400) {
        	velY = -1;
        }
        else if (*reg2 < 200) {
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
