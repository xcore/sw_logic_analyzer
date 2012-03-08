#include <stdio.h>
#include <string.h>
#include "device_if.h"

int main(void) {
  xdbg_adapter_list *deviceList;

  // Get the current active device list (Any devices not in JTAG mode programmed here)
  device_get_list(&deviceList);

  int x = device_create(0, 0, 0);

  if (x < 0) {
    return 1;
  }
  // Close the device
  device_destroy();

  printf(" ... Disconnected from device 0\n");

  return 0;
}
