#ifndef _DEVICE_IF_H_
#define _DEVICE_IF_H_

#include "xdbg_dev_api_device_types.h"

#define DEVICE_ACCESS_SUCCESS 0
#define DEVICE_ACCESS_TIMEOUT -1
#define DEVICE_ACCESS_DISCONNECTED -2
#define DEVICE_ACCESS_ERROR -3

int device_read(char *data, unsigned int length, unsigned int timeout = 1000);
int device_write(char *data, unsigned int length, unsigned int timeout = 1000);
int device_create(int device, int chip, int uart);
int device_destroy();
void device_get_list(xdbg_adapter_list **deviceList);
int device_extract_serial_info(char *device_serial, xdbg_adapter *adapter_info);

#endif

