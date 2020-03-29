#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"

#include "btscan.h"


// Logging tag
static const char* TAG = "MAIN";

#define STACK_SIZE  2048

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
    if (!bt_scan_init()) {
        ESP_LOGE(TAG, "FAILED TO START SCANNING");
    }

    while (true) {
        ESP_LOGI(TAG, "TICK");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

