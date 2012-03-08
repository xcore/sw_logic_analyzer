#ifndef _XDBG_DEV_API_DEVICE_TYPES_H_
#define _XDBG_DEV_API_DEVICE_TYPES_H_

#define MAX_JTAG_DEVICES 64

typedef struct {
  char internalDevName[64];
  char externalDevName[64];
  int internalDevId;
  char serialNumber[16];
  char productIdCode[16];
  int xdk;
  int deviceVersion;
} xdbg_adapter;

typedef struct {
  int numDevices;
  xdbg_adapter device_records[MAX_JTAG_DEVICES];
}xdbg_adapter_list;

#endif
