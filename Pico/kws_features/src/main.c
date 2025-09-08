#include "pico/stdlib.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    sleep_ms(2000); // wait for USB CDC to enumerate

    puts("=== Pico Serial Test ===");

    int counter = 0;
    while (true) {
        printf("Heartbeat %d\n", counter++);
        sleep_ms(1000);
    }
}