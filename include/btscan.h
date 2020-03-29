#ifndef BTSCAN_H
#define BTSCAN_H

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
#include <esp_coexist.h>

bool bt_scan_init();
bool bt_scan_stop();

#endif
