#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_blufi_api.h> // needed for BLE_ADDR types, do not remove
#include <esp_coexist.h>


// Logging tag
static const char* TAG = "MAIN";

#define STACK_SIZE  2048
#define BT_MODE ESP_BT_MODE_BLE
#define BLESCANTIME 10

#define BT_BD_ADDR_HEX(addr) addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

const char *bt_addr_t_to_string(esp_ble_addr_type_t type) {
  switch (type) {
  case BLE_ADDR_TYPE_PUBLIC:
    return "BLE_ADDR_TYPE_PUBLIC";
  case BLE_ADDR_TYPE_RANDOM:
    return "BLE_ADDR_TYPE_RANDOM";
  case BLE_ADDR_TYPE_RPA_PUBLIC:
    return "BLE_ADDR_TYPE_RPA_PUBLIC";
  case BLE_ADDR_TYPE_RPA_RANDOM:
    return "BLE_ADDR_TYPE_RPA_RANDOM";
  default:
    return "Unknown addr_t";
  }
}

const char *btsig_gap_type(uint32_t gap_type) {
    switch (gap_type) {
    case 0x01:
        return "Flags";
    case 0x02:
        return "Incomplete List of 16-bit Service Class UUIDs";
    case 0x03:
        return "Complete List of 16-bit Service Class UUIDs";
    case 0x04:
        return "Incomplete List of 32-bit Service Class UUIDs";
    case 0x05:
        return "Complete List of 32-bit Service Class UUIDs";
    case 0x06:
        return "Incomplete List of 128-bit Service Class UUIDs";
    case 0x07:
        return "Complete List of 128-bit Service Class UUIDs";
    case 0x08:
        return "Shortened Local Name";
    case 0x09:
        return "Complete Local Name";
    case 0x0A:
        return "Tx Power Level";
    case 0x0D:
        return "Class of Device";
    case 0x0E:
        return "Simple Pairing Hash C/C-192";
    case 0x0F:
        return "Simple Pairing Randomizer R/R-192";
    case 0x10:
        return "Device ID/Security Manager TK Value";
    case 0x11:
        return "Security Manager Out of Band Flags";
    case 0x12:
        return "Slave Connection Interval Range";
    case 0x14:
        return "List of 16-bit Service Solicitation UUIDs";
    case 0x1F:
        return "List of 32-bit Service Solicitation UUIDs";
    case 0x15:
        return "List of 128-bit Service Solicitation UUIDs";
    case 0x16:
        return "Service Data - 16-bit UUID";
    case 0x20:
        return "Service Data - 32-bit UUID";
    case 0x21:
        return "Service Data - 128-bit UUID";
    case 0x22:
        return "LE Secure Connections Confirmation Value";
    case 0x23:
        return "LE Secure Connections Random Value";
    case 0x24:
        return "URI";
    case 0x25:
        return "Indoor Positioning";
    case 0x26:
        return "Transport Discovery Data";
    case 0x17:
        return "Public Target Address";
    case 0x18:
        return "Random Target Address";
    case 0x19:
        return "Appearance";
    case 0x1A:
        return "Advertising Interval";
    case 0x1B:
        return "LE Bluetooth Device Address";
    case 0x1C:
        return "LE Role";
    case 0x1D:
        return "Simple Pairing Hash C-256";
    case 0x1E:
        return "Simple Pairing Randomizer R-256";
    case 0x3D:
        return "3D Information Data";
    case 0xFF:
        return "Manufacturer Specific Data";
        
    default:
        return "Unknown type";
    }
}

IRAM_ATTR void gap_callback_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_ble_gap_cb_param_t *p = (esp_ble_gap_cb_param_t *)param;
    
    ESP_LOGI(TAG, "BT payload rcvd -> type: 0x%.2x -> %s", *p->scan_rst.ble_adv,btsig_gap_type(*p->scan_rst.ble_adv));
    
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        ESP_ERROR_CHECK(esp_ble_gap_start_scanning(BLESCANTIME));
        break;

    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        // evaluate scan results

        // Inquiry complete, scan is done
        if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT) {
            ESP_ERROR_CHECK(esp_ble_gap_start_scanning(BLESCANTIME));
            return;
        }
        
        // Inquiry result for a peer device
        if (p->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
        // evaluate sniffed packet
            ESP_LOGI(TAG, "Device address (bda): %02x:%02x:%02x:%02x:%02x:%02x",
                     BT_BD_ADDR_HEX(p->scan_rst.bda));
            ESP_LOGI(TAG, "Addr_type           : %s",
                     bt_addr_t_to_string(p->scan_rst.ble_addr_type));
            ESP_LOGI(TAG, "RSSI                : %d", p->scan_rst.rssi);

            
            // add this device and show new count total if it was not previously added
            // mac_add((uint8_t *)p->scan_rst.bda, p->scan_rst.rssi, MAC_SNIFF_BLE);
            break;
        }

    default:
        break;
    }
}

bool btSetup() {
    ESP_ERROR_CHECK(esp_coex_preference_set(ESP_COEX_PREFER_BALANCE));
    
    // Check if the BT controller is already running
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        ESP_LOGI(TAG, "Controller already enabled");
        return true;
    }

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        ESP_LOGI(TAG, "Initializing BT controller");
        esp_bt_controller_init(&cfg);

        // Block intil no longer idle
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
            // NOP
        }
        ESP_LOGI(TAG, "BT controller initialized");
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if (esp_bt_controller_enable(BT_MODE)) {
            ESP_LOGE(TAG, "BT controller enable failed");
            return false;
        }
    }

    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_LOGE(TAG, "BT controller not enabled");
        return false;
    }
    
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    
    static esp_ble_scan_params_t ble_scan_params = {
           .scan_type = BLE_SCAN_TYPE_ACTIVE,
           .own_addr_type = BLE_ADDR_TYPE_RANDOM,
           .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,

    };
    
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(&gap_callback_handler));

    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(BLESCANTIME));
    
    ESP_LOGI(TAG, "Bluetooth scanning started");
    return true;
}

void init_flash() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "Erasing flash");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    init_flash();
    
    ESP_LOGI(TAG, "Starting scan application");
    if (!btSetup()) {
        ESP_LOGE(TAG, "FAILED TO START SCANNING");
    }

    while (true) {
        ESP_LOGI(TAG, "TICK");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

