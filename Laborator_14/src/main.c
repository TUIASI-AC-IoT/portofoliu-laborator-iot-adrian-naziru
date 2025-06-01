#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char *TAG = "REST_SERVER";

static bool config_created_1 = false;
static bool config_created_2 = false;

float simulate_sensor_value(const char* sensor_id) {
    if (strcmp(sensor_id, "1") == 0) {
        return 20.0 + ((float)rand() / RAND_MAX) * (30.0 - 20.0);
    } else if (strcmp(sensor_id, "2") == 0) {
        return 50.0 + ((float)rand() / RAND_MAX) * (60.0 - 50.0);
    } else {
        return -1.0f;
    }
}

static esp_err_t get_handler(httpd_req_t *req) {
    char sensor_id[8];
    sscanf(req->uri, "/sensor/%7s", sensor_id);

    float sensor_value = simulate_sensor_value(sensor_id);
    if (sensor_value < 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Sensor not found");
        return ESP_OK;
    }

    char resp_str[64];
    snprintf(resp_str, sizeof(resp_str), "{\"sensor_id\": \"%s\", \"value\": %.2f}", sensor_id, sensor_value);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t post_handler(httpd_req_t *req) {
    char sensor_id[8];
    sscanf(req->uri, "/sensor/%7s", sensor_id);

    bool *config_flag = (strcmp(sensor_id, "1") == 0) ? &config_created_1 :
                        (strcmp(sensor_id, "2") == 0) ? &config_created_2 : NULL;

    if (config_flag == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown sensor");
        return ESP_OK;
    }

    if (*config_flag) {
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Config file already exists for this sensor.\"}");
        return ESP_OK;
    }

    *config_flag = true;
    httpd_resp_sendstr(req, "Config created (simulated)");
    return ESP_OK;
}

static esp_err_t put_handler(httpd_req_t *req) {
    char sensor_id[8];
    sscanf(req->uri, "/sensor/%7s/config", sensor_id);

    bool *config_flag = (strcmp(sensor_id, "1") == 0) ? &config_created_1 :
                        (strcmp(sensor_id, "2") == 0) ? &config_created_2 : NULL;

    if (config_flag == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Unknown sensor");
        return ESP_OK;
    }

    if (!*config_flag) {
        httpd_resp_set_status(req, "406 Not Acceptable");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"error\": \"Config file does not exist; cannot update.\"}");
        return ESP_OK;
    }

    char buf[100];
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf)));
    if (ret <= 0) {
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Config updated (simulated)");
    return ESP_OK;
}

httpd_uri_t get_uri = {
    .uri      = "/sensor/*",
    .method   = HTTP_GET,
    .handler  = get_handler,
};

httpd_uri_t post_uri = {
    .uri      = "/sensor/*",
    .method   = HTTP_POST,
    .handler  = post_handler,
};

httpd_uri_t put_uri = {
    .uri      = "/sensor/*/config",
    .method   = HTTP_PUT,
    .handler  = put_handler,
};

void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &get_uri);
        httpd_register_uri_handler(server, &post_uri);
        httpd_register_uri_handler(server, &put_uri);
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    srand(time(NULL));

    ESP_LOGI(TAG, "Starting web server");
    start_webserver();
}
