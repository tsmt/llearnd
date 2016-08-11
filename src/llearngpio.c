#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <mcp3004.h>

int main(int argc, char* argv[]) {
    int i = 0;
    wiringPiSetup();
    mcp3004Setup(100, 0);
    mcp3004Setup(110, 1);
    while(1) {
        for(i = 0; i < 8; i++) {
            printf("%d - %d %d\n", i, analogRead(100+i), analogRead(110+i));
        }
        delay(1000);
    }
    return 0;
}
