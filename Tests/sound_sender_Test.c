
//Local includes:
#include "initialize.h"
#include "usb.h"
#include "receiver.h"

#include <libmaple/libmaple.h>
#include <libmaple/gpio.h>
#include <libmaple/exti.h>



// setup() and loop():
void setup(void) {
    usb_start();
    receiver_start();
}

void loop(void) {
    usb_printlnString("This is a loop!");
    delay_us(100000);
}


__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
    setup();

    while (1) {
        loop();
    }

    return 0;
}
