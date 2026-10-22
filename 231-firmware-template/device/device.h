#pragma once

#define DEVICE_NAME "es-firmware"
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/L3V666/es-student"

#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

void device_info(void);
