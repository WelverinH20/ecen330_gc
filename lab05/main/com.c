#include "com.h"

uint8_t src[NET_ALEN];

int32_t com_init(void) {
    // 1. Initialize the Wi-Fi network wrapper
    if (net_init() != 0) {
        return -1;
    }

    // 2. Open group registration with group ID 1 for lab compatibility
    if (net_group_open(1) != 0) {
        return -1;
    }

    // 3. Print the required status message to the LCD
    graphics_drawMessage("Waiting to join group...", CONFIG_MESS_CLR, CONFIG_BACK_CLR);

    // 4. Wait 4 seconds to allow controllers to pair
    vTaskDelay(pdMS_TO_TICKS(4000));

    // 5. Close group registration
    net_group_close();

    return 0;
}

int32_t com_deinit(void) {
    // Free network resources
    return net_deinit();
}

int32_t com_write(const void *buf, uint32_t size) {
    // Non-blocking write using the underlying net send function
    return net_send(NULL, buf, size, 0);
}

int32_t com_read(void *buf, uint32_t size) {
    // Non-blocking read using the underlying net receive function
    return net_recv(src, buf, size, 0);
}