#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "xdbg_dev_api_device_types.h"

#define XMOS_DEVICE_XTAG_2 				0x1
#define XMOS_DEVICE_XMP64 				0x2

static void xmos_device_id_to_str(unsigned int id, char *internal, char *external) {
  switch (id) {
  case XMOS_DEVICE_XTAG_2:
	  sprintf(internal, "XTAG2");
	  sprintf(external, "XMOS XTAG-2");
	  break;
  case XMOS_DEVICE_XMP64:
	  sprintf(internal, "XMP64");
	  sprintf(external, "XMOS XMP64");
	  break;
  default:
	  break;
  }
}

// Specification of serial number format
// XMOS X|D/d|00|00|00|00000000
// Custom D/d|00|0000000000000

static int extract_xmos_device_info(char *device_serial, xdbg_adapter *adapter_info) {

	if (strcmp(device_serial, "XXXXXXXXXXXXXXXX") == 0) {
		// Unprogrammed device
		sprintf(adapter_info->internalDevName, "UNPROGRAMMED");
		sprintf(adapter_info->externalDevName, "UNPROGRAMMED");
		strcpy(adapter_info->serialNumber, device_serial);
		return 1;
	}

	if (device_serial[1] == 'D' || device_serial[1] == 'd') {
		//unsigned short capability = device_serial[2] || device_serial[3] << 8;
		unsigned char cap[2];
		unsigned int type = 0;
		cap[0] = device_serial[5];
		cap[1] = '\0';
		type = atoi((const char *)cap);
		xmos_device_id_to_str(type, adapter_info->internalDevName,  adapter_info->externalDevName);
		strncpy(adapter_info->serialNumber, &device_serial[8], 8);
	} else {
		char *timestamp_only = NULL;

		// Default to XTAG-2
		sprintf(adapter_info->internalDevName, "XTAG2");
		sprintf(adapter_info->externalDevName, "XMOS XTAG-2");

		if (!device_serial) {
			strcpy(adapter_info->serialNumber, "N/A");
			return 1;
		} else {
			if (strlen(device_serial)==17) {
				timestamp_only = &device_serial[9];
			} else {
				timestamp_only = &device_serial[8];
			}
		}

		// XMP-64 device name
		if (strstr(device_serial, "XMP64")) {
			sprintf(adapter_info->internalDevName, "XMP-64");
			sprintf(adapter_info->externalDevName, "XMOS XMP-64");
		}

		strcpy(adapter_info->serialNumber, timestamp_only);
	}

	return 1;
}

static int extract_custom_device_info(char *device_serial, xdbg_adapter *adapter_info) {
	//unsigned short capability = device_serial[1] || device_serial[2] << 8;
	sprintf(adapter_info->internalDevName, "JTAG");
    sprintf(adapter_info->externalDevName, "Custom Adapter");
	strncpy(adapter_info->serialNumber, &device_serial[3], 13);
    return 1;
}



int device_extract_serial_info(char *device_serial, xdbg_adapter *adapter_info) {

    if (device_serial[0] == 'x' || device_serial[0] == 'X') {
    	return extract_xmos_device_info(device_serial, adapter_info);
    } else if (device_serial[0] == 'd' || device_serial[0] == 'D') {
    	return extract_custom_device_info(device_serial, adapter_info);
    } else {
      sprintf(adapter_info->internalDevName, "UNKNOWN");
      sprintf(adapter_info->externalDevName, "UNKNOWN");
      return 1;
    }

    // Not recognised as accepted device serial number
    return 0;
}
