#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "device_if.h"
#include "xdbg_dev_api_device_types.h"
//#include "l1_jtag.h"

/* the device's vendor and product id */
#define XMOS_VID 0x20b1
#define JTAG_PID 0xf7d1

/* the device's endpoints */
#define EP_IN 0x82
#define EP_OUT 0x01

#ifdef _WIN32
#include "usb.h"
static usb_dev_handle *devh = NULL; 
static usb_dev_handle *listdevh = NULL; 
static usb_dev_handle *saveddevh = NULL; 
#else 
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
static void (*ofunc) (int);
#include "libusb.h"
static libusb_device_handle *devh = NULL;
static libusb_device_handle *listdevh = NULL;
static libusb_device_handle *saveddevh = NULL;
#endif

xdbg_adapter_list jtag_devices;
int current_jtag_device = 0;
int device_open = 0;

// USB LOADER CMD CODE

#define LOADER_BUF_SIZE 512
#define LOADER_CMD_SUCCESS 0
#define LOADER_CMD_FAIL -1

enum loader_cmd_type {
  LOADER_CMD_NONE,
  LOADER_CMD_WRITE_MEM,
  LOADER_CMD_WRITE_MEM_ACK,
  LOADER_CMD_GET_VERSION,
  LOADER_CMD_GET_VERSION_ACK,
  LOADER_CMD_JUMP,
  LOADER_CMD_JUMP_ACK
};

char l1_jtag_bin[65536];

static void loader_to_jtag(unsigned int firmware_version) {
  unsigned int i = 0;
  unsigned int address = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 0;
  unsigned int remainder = 0;
  unsigned int data_ptr = 0;
  int cmd_buf[LOADER_BUF_SIZE/4];
  int fd, l1_jtag_bin_len;

  saveddevh = devh;
  devh = listdevh;

  fd = open("image_n0c0.bin", 0);
  l1_jtag_bin_len = read(fd, l1_jtag_bin, 65536);

  memset(cmd_buf, 0, LOADER_BUF_SIZE);

  cmd_buf[0] = LOADER_CMD_GET_VERSION;

  device_write((char *)cmd_buf, 4, 1000);
  device_read((char *)cmd_buf, 8, 1000);

  address = 0x10000;
  block_size = LOADER_BUF_SIZE - 12; 
  num_blocks = l1_jtag_bin_len / block_size;
  remainder = l1_jtag_bin_len - (num_blocks * block_size);

  for (i = 0; i < num_blocks; i++) {
    cmd_buf[0] = LOADER_CMD_WRITE_MEM;
    cmd_buf[1] = address;
    cmd_buf[2] = block_size;
    memcpy(&cmd_buf[3], &l1_jtag_bin[data_ptr], block_size);
    device_write((char *)cmd_buf, LOADER_BUF_SIZE, 1000);
    device_read((char *)cmd_buf, 8, 1000);

    address += block_size;
    data_ptr += block_size;
  }

  if (remainder) {
    cmd_buf[0] = LOADER_CMD_WRITE_MEM;
    cmd_buf[1] = address;
    cmd_buf[2] = remainder;
    memcpy(&cmd_buf[3], &l1_jtag_bin[data_ptr], remainder);
    device_write((char *)cmd_buf, LOADER_BUF_SIZE, 1000);
    device_read((char *)cmd_buf, 8, 1000);
  }

  cmd_buf[0] = LOADER_CMD_JUMP;
  device_write((char *)cmd_buf, 4, 1000);
  device_read((char *)cmd_buf, 8, 1000);

  devh = saveddevh;
  printf("Len: %d done\n", l1_jtag_bin_len);

}


#ifdef _WIN32
static usb_dev_handle *open_dev(unsigned int id) {
  struct usb_bus *bus;
  struct usb_device *dev;
  unsigned int i = 0;

  for(bus = usb_get_busses(); bus; bus = bus->next) {
    for(dev = bus->devices; dev; dev = dev->next) {
      if(dev->descriptor.idVendor == XMOS_VID && dev->descriptor.idProduct == JTAG_PID) {
        if (i == id) {
          return usb_open(dev);
        }
        i++;
      }
    }
  }

  return NULL;
}

static void find_connected_devices(void) {
  struct usb_bus *bus;
  struct usb_device *dev;
  unsigned int device_enumerating = 0;
  unsigned int num_devs = 0;

  usb_init(); /* initialize the library */
  usb_find_busses(); /* find all busses */
  num_devs = usb_find_devices(); /* find all connected devices */

  jtag_devices.numDevices = 0;
  memset(jtag_devices.device_records, 0, sizeof(xdbg_adapter) * MAX_JTAG_DEVICES);
  
  for (bus = usb_get_busses(); bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if (dev->descriptor.idVendor == XMOS_VID && dev->descriptor.idProduct == JTAG_PID && !(dev->descriptor.bcdDevice & 0xff00)) {
        // Switch loaders to become JTAG devices
        listdevh = usb_open(dev);
        usb_set_configuration(listdevh, 1);
        usb_claim_interface(listdevh, 0);
        loader_to_jtag(dev->descriptor.bcdDevice);
        usb_release_interface(listdevh, 0);
        usb_reset(listdevh);
        device_enumerating = 1;
        Sleep(2000);
        usb_close(listdevh);
      }
    }
  }

  usb_find_busses(); /* find all busses */
  num_devs = usb_find_devices(); /* find all connected devices */

  for (bus = usb_get_busses(); bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if (dev->descriptor.idVendor == XMOS_VID && dev->descriptor.idProduct == JTAG_PID && (dev->descriptor.bcdDevice & 0xff00)) {
        char serial_number[64];
        // Default to XTAG-2
        sprintf(jtag_devices.device_records[jtag_devices.numDevices].internalDevName, "XTAG2");
        sprintf(jtag_devices.device_records[jtag_devices.numDevices].externalDevName, "XMOS XTAG-2");
        jtag_devices.device_records[jtag_devices.numDevices].internalDevId = jtag_devices.numDevices;
        sprintf(jtag_devices.device_records[jtag_devices.numDevices].productIdCode, "0xf7d1");
        jtag_devices.device_records[jtag_devices.numDevices].xdk = 0;
        jtag_devices.device_records[jtag_devices.numDevices].deviceVersion = dev->descriptor.bcdDevice;

        //Open device to get serial number
        listdevh = usb_open(dev);
        usb_get_string_simple(listdevh, dev->descriptor.iSerialNumber, serial_number, sizeof(serial_number));


        if (device_extract_serial_info(serial_number, &jtag_devices.device_records[jtag_devices.numDevices])) {
          // Add device if successfully recognized.
          jtag_devices.numDevices++;
        }

        usb_close(listdevh);
        listdevh = NULL;
      }
    }
  }
}

int device_read(char *data, unsigned int length, unsigned int timeout) {
  int result = 0;

  result = usb_bulk_read(devh, EP_IN, data, length, timeout);
 
  if (result < 0) { 
    return DEVICE_ACCESS_TIMEOUT;
  } else {
    return 0;
  }
}

int device_write(char *data, unsigned int length, unsigned int timeout) {
  int result = 0;

  result = usb_bulk_write(devh, EP_OUT, data, length, timeout);

  if (result < 0) { 
    return DEVICE_ACCESS_TIMEOUT;
  } else {
    return 0;
  }
}

static int inited = 0;

int device_create(int device, int chip, int uart) {
  int num_libusb_devices = 0;
  usb_init(); /* initialize the library */
  usb_find_busses(); /* find all busses */
  usb_find_devices(); /* find all connected devices */

  if (jtag_devices.numDevices == 0) {
    return false;
  }

  if(!(devh = open_dev(device))) {
    printf("error: device not found!\n");
    return 0;
  }

  if(usb_set_configuration(devh, 1) < 0) {
    printf("error: setting config 1 failed\n");
    usb_close(devh);
    return 0;
  }

  if((usb_claim_interface(devh, 0)) < 0) {
    printf("error: claiming interface 0 failed\n");
    usb_close(devh);
    return 0;
  }

  device_open = 1;

  return true;
}

int device_destroy() {
  usb_release_interface(devh, 0);
  usb_close(devh);

  device_open = 0;

  return true;
}

#else

static void find_connected_devices() {
  libusb_device *dev = NULL;
  libusb_device **devs = NULL;
  unsigned int device_enumerating = 0;
  int i = 0;

  libusb_init(NULL); /* initialize the library */

  libusb_get_device_list(NULL, &devs);
  
  jtag_devices.numDevices = 0;

  memset(jtag_devices.device_records, 0, sizeof(xdbg_adapter) * MAX_JTAG_DEVICES);

  if (devs == NULL)
	  return;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);
    if (desc.idVendor == XMOS_VID && desc.idProduct == JTAG_PID && !(desc.bcdDevice & 0xff00)) {
      if (libusb_open(dev, &listdevh) < 0) {
        libusb_free_device_list(devs, 1);
        return;
      }
      libusb_set_configuration(listdevh, 1);
      libusb_claim_interface(listdevh, 0);
      loader_to_jtag(desc.bcdDevice);
      device_enumerating = 1;
      libusb_release_interface(listdevh, 0);
      libusb_reset_device(listdevh);
      system("sleep 2");
      libusb_close(listdevh);
    }
  }

  libusb_free_device_list(devs, 1);
  return;
  libusb_get_device_list(NULL, &devs);

  i = 0;

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);
    if (desc.idVendor == XMOS_VID && desc.idProduct == JTAG_PID) {
      char serial_number[64];
      memset(serial_number, 0, 64);

      // Device ID
      jtag_devices.device_records[jtag_devices.numDevices].internalDevId = jtag_devices.numDevices;
      // USB product ID
      sprintf(jtag_devices.device_records[jtag_devices.numDevices].productIdCode, "0x%x", desc.idProduct);
      // Device firmware revision
      jtag_devices.device_records[jtag_devices.numDevices].deviceVersion = desc.bcdDevice;
      // Not an XDK (legacy)
      jtag_devices.device_records[jtag_devices.numDevices].xdk = 0;

      // Get serial number from device
      libusb_open(dev, &listdevh);
      libusb_get_string_descriptor_ascii(listdevh, 5, (unsigned char *)serial_number, sizeof(serial_number));

      if (device_extract_serial_info(serial_number, &jtag_devices.device_records[jtag_devices.numDevices])) {
    	  // Add device if successfully recognized.
          jtag_devices.numDevices++;
      }

      libusb_close(listdevh);
      listdevh = NULL;
    }
  }

  libusb_free_device_list(devs, 1);

  if (!device_open) {
    libusb_exit(NULL);
  }
}

static int dbg_usb_bulk_io(int ep, char *bytes, int size, int timeout) {
  int actual_length;
  int r;
  ofunc = signal(SIGINT, SIG_IGN);
  r = libusb_bulk_transfer(devh, ep & 0xff, (unsigned char*)bytes, size, &actual_length, timeout);
  signal(SIGINT, ofunc);

  if (r == 0) {
    return DEVICE_ACCESS_SUCCESS;
  }
 
  if (r == LIBUSB_ERROR_TIMEOUT) {
    return DEVICE_ACCESS_TIMEOUT;
  } 

  if (r == LIBUSB_ERROR_NO_DEVICE) {
    return DEVICE_ACCESS_DISCONNECTED;
  }
    
  return DEVICE_ACCESS_ERROR;
}

int device_read(char *data, unsigned int length, unsigned int timeout) {
  int result = 0;
  result = dbg_usb_bulk_io(EP_IN, data, length, timeout);
  return result;
}

int device_write(char *data, unsigned int length, unsigned int timeout) {
  int result = 0;
  result = dbg_usb_bulk_io(EP_OUT, data, length, timeout);
  return result;
}

static int find_xmos_device(unsigned int id) {
  libusb_device *dev;
  libusb_device **devs;
  int i = 0;
  unsigned found = 0;
  
  libusb_get_device_list(NULL, &devs);

  while ((dev = devs[i++]) != NULL) {
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(dev, &desc);
    if (desc.idVendor == XMOS_VID && desc.idProduct == JTAG_PID) {
      if (found == id) {
        if (libusb_open(dev, &devh) < 0) {
          return -1;
        }
        break;
      }
      found++;
    }
  }

  libusb_free_device_list(devs, 1);

  return devh ? 0 : -1;
}

int device_create(int device, int chip, int uart) {

  int r = 1;

  r = libusb_init(NULL);
  if (r < 0) {
    fprintf(stderr, "failed to initialise libusb\n");
    return -1;
  }

  r = find_xmos_device(device);
  if (r < 0) {
    fprintf(stderr, "Could not find/open device\n");
    return -1;
  }

  r = libusb_claim_interface(devh, 0);
  if (r < 0) {
    fprintf(stderr, "usb_claim_interface error %d\n", r);
    return -1;
  }
  
  device_open = 1;

  return true;
}

int device_destroy() {
  libusb_release_interface(devh, 0);
  libusb_close(devh);
  libusb_exit(NULL);

  device_open = 0;

  return true;
}

#endif

void device_get_list(xdbg_adapter_list **deviceList) {
  find_connected_devices();
  *deviceList = &jtag_devices;
}
