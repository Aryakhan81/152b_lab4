#include "xbasic_types.h"
#include "xparameters.h"
 
Xuint32* baseaddr_p = (Xuint32*)XPAR_VGA_CONTROLLER_0_S00_AXI_BASEADDR;
 
int main() {
    *(baseaddr_p + 0) = 0xFFFFFFFF; // Initially, all bricks are set
    *(baseaddr_p + 1) = 320;        // Set initial ball x to halfway through the screen
    *(baseaddr_p + 2) = 320;        // Set initial ball y near the bottom
    *(baseaddr_p + 3) = 320;        // Set initial paddle x to halfway through the screen

    short int velX = -6;
    short int velY = -8;

    // Game controls
    bool hasDied = false;
    bool hasWon = false;
    
    while (!hasDied && !hasWon) {
        // TODO: Collision detection logic

        hasWon = !*(baseaddr_p + 0);

        // TODO: Velocity change logic

        // Update ball positions with current velocity
        if((int)(*(baseaddr_p + 1)) + velX < 0) velX = -velX;
        if((int)(*(baseaddr_p + 2)) + velY < 0) velY = -velY;
        *(baseaddr_p + 1) += velX;
        *(baseaddr_p + 2) += velY;

        hasDied = *(baseaddr_p + 2) > 400;
    }

    // Lose logic
    if (hasDied) {

    }

    // Win logic
    if (hasWon) {

    }
 
    return 0;
}
