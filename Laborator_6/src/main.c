/* WiFi Station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "http-server.h"
#include "mdns.h"
#include "mdns_console.h"

/* DEFINES */
#define CONFIG_ESP_WIFI_SSID      "lab-iot"
#define CONFIG_ESP_WIFI_PASS      "IoT-IoT-IoT"
#define CONFIG_ESP_MAXIMUM_RETRY  5
#define CONFIG_LOCAL_PORT         10001

#define DEFAULT_ESP_WIFI_SSID      "Esp-32_Naziru"
#define DEFAULT_ESP_WIFI_PASS      "Parola12"
#define DEFAULT_ESP_WIFI_CHANNEL   11
#define DEFAULT_MAX_STA_CONN       5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* ENUMS */
typedef enum {
    INIT = 0,
    INIT_NVS,
    START_AP,
    STOP_AP,
    START_HTTP,
    WAIT_FOR_DATA,
    START_STA,
    STOP_STA,
    SCAN_SSID,
    IDLE,
    ERROR = 0xFF
} e_iot_states;

/* VARIABLES */
static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi_station";
static int livetime_counter = 0;
static int s_retry_num = 0;
static esp_timer_handle_t periodic_timer;
static e_iot_states iot_mode = INIT;

/* FUNCTION DECLARATIONS */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

/* WiFi SoftAP Initialization */
void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL, NULL));
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_ESP_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_ESP_WIFI_SSID),
            .channel = DEFAULT_ESP_WIFI_CHANNEL,
            .password = DEFAULT_ESP_WIFI_PASS,
            .max_connection = DEFAULT_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.required = true},
        },
    };
    
    if (strlen(DEFAULT_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP initialized. SSID:%s password:%s channel:%d",
             DEFAULT_ESP_WIFI_SSID, DEFAULT_ESP_WIFI_PASS, DEFAULT_ESP_WIFI_CHANNEL);
}

/* WiFi Event Handler */
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "Device connected to AP");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "Device disconnected from AP");
    }
}

/* WiFi Scaning Initialization */
void wifi_init_sta(void) {
    //ESP_ERROR_CHECK(esp_netif_init());  this cause an error
    // ESP_ERROR_CHECK(esp_event_loop_create_default()); this cause an error
    esp_netif_create_default_wifi_sta();

   // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();   this cause an error
   // ESP_ERROR_CHECK(esp_wifi_init(&cfg)); this cause an error
    
    wifi_config_t wifi_config = {};
    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wifi_config.sta.threshold.rssi = -127;
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

}

/* Timer Callback */
static void periodic_timer_callback(void *arg) {
    livetime_counter++;
    if (livetime_counter % 100 == 0) {
        ESP_LOGI(TAG, "Uptime: %d seconds", livetime_counter / 100);
        ESP_LOGI(TAG, "IoT Mode: %d", iot_mode);
    }
    
    switch (iot_mode) {
        case INIT:
            iot_mode = INIT_NVS;
            break;

        case INIT_NVS:
            if (nvs_flash_init() != ESP_OK) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ESP_ERROR_CHECK(nvs_flash_init());
            }
            iot_mode = START_AP;
            break;

        case START_AP:
            wifi_init_softap();
            iot_mode = START_HTTP;
            break;

        case START_HTTP:

            httpd_handle_t server = start_webserver();
            if (server) {
            ESP_LOGI(TAG, "Webserver started successfully!");
            iot_mode = START_STA;
            } else {
            iot_mode = ERROR;
            ESP_LOGE(TAG, "Failed to start webserver!");
            }
           
            break;

        case WAIT_FOR_DATA:
        case STOP_AP:
        case START_STA:
            wifi_init_sta();
            iot_mode = WAIT_FOR_DATA;
            break;
        case STOP_STA:
        case SCAN_SSID:
        case IDLE:
        case ERROR:
            break;
    }
}

/* Main Application Entry */
void app_main(void) {
    esp_timer_create_args_t periodic_timer_args = {
        .callback = periodic_timer_callback,
        .name = "periodic_100ms"
    };

    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10000)); // 10ms
}