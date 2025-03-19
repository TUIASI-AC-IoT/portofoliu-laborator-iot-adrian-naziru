#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/event_groups.h"

#include "esp_http_server.h"

static const char *TAG = "HTTP_SERVER";

char wifi_list_html[2048]; // Adjust size as needed

// Placeholder for the selected SSID and password
char selected_ssid[32];
char password[64];

/* Our URI handler function to be called during GET /index.html request */
esp_err_t get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /index.html request received");
    /* Send the HTML content containing the WiFi list */
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, wifi_list_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /results.html request */
esp_err_t post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /results.html request received");
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1); // Leave space for null terminator

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[recv_size] = '\0'; // Null-terminate the received data
    ESP_LOGI(TAG, "Received content: %s", content);

    // Parse the received data (assuming it's in a simple format like "ssid=your_ssid&password=your_password")
    char *ssid_start = strstr(content, "ssid=");
    if (ssid_start != NULL) {
        ssid_start += 5; // Move past "ssid="
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end != NULL) {
            strncpy(selected_ssid, ssid_start, MIN(sizeof(selected_ssid) - 1, ssid_end - ssid_start));
            selected_ssid[MIN(sizeof(selected_ssid) - 1, ssid_end - ssid_start)] = '\0';
        } else {
            strncpy(selected_ssid, ssid_start, sizeof(selected_ssid) - 1);
            selected_ssid[sizeof(selected_ssid) - 1] = '\0';
        }
        ESP_LOGI(TAG, "Selected SSID: %s", selected_ssid);
    }

    char *password_start = strstr(content, "password=");
    if (password_start != NULL) {
        password_start += 9; // Move past "password="
        strncpy(password, password_start, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
        ESP_LOGI(TAG, "Password: %s", password);
    }

    // Prepare the response
    char resp[256];
    snprintf(resp, sizeof(resp), "SSID: %s<br>Password: %s", selected_ssid, password);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /index.html */
httpd_uri_t uri_get = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = get_handler,
    .user_ctx  = NULL
};

/* URI handler structure for POST /results.html */
httpd_uri_t uri_post = {
    .uri       = "/results.html",
    .method    = HTTP_POST,
    .handler   = post_handler,
    .user_ctx  = NULL
};

/* Function to update the WiFi list HTML */
void update_wifi_list(wifi_ap_record_t *ap_list, uint16_t ap_count)
{
    memset(wifi_list_html, 0, sizeof(wifi_list_html));
    strcat(wifi_list_html, "<!DOCTYPE html><html><head><title>WiFi Scanner</title></head><body><h1>Available WiFi Networks</h1><form action=\"/results.html\" method=\"post\">");
    strcat(wifi_list_html, "<select name=\"ssid\">");
    for (int i = 0; i < ap_count; i++) {
        char option[128];
        snprintf(option, sizeof(option), "<option value=\"%s\">%s (RSSI: %d)</option>", ap_list[i].ssid, ap_list[i].ssid, ap_list[i].rssi);
        strcat(wifi_list_html, option);
    }
    strcat(wifi_list_html, "</select><br><br>");
    strcat(wifi_list_html, "Password: <input type=\"password\" name=\"password\"><br><br>");
    strcat(wifi_list_html, "<input type=\"submit\" value=\"Connect\">");
    strcat(wifi_list_html, "</form></body></html>");
}

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "Starting web server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    } else {
        ESP_LOGE(TAG, "Error starting web server!");
    }
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        ESP_LOGI(TAG, "Stopping web server");
        httpd_stop(server);
    }
}