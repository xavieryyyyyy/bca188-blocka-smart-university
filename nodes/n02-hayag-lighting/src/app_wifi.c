#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "app_wifi.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_random.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0

#define NVS_NAMESPACE "hayag_wifi"
#define NVS_SSID_KEY "ssid"
#define NVS_PASSWORD_KEY "password"

#define SETUP_AP_SSID "HAYAG-N02-Setup"

static const char *TAG = "HAYAG_WIFI";

static EventGroupHandle_t wifi_event_group;
static httpd_handle_t portal_server = NULL;

static bool credentials_available = false;

static const char setup_page[] =
    "<!DOCTYPE html>"
    "<html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>HAYAG Node 2 Setup</title></head>"
    "<body style='font-family:Arial;max-width:420px;margin:40px auto;padding:20px'>"
    "<h2>HAYAG Node 2</h2>"
    "<p>Configure your Wi-Fi connection.</p>"
    "<form method='POST' action='/save'>"
    "<label>Wi-Fi name (SSID)</label><br>"
    "<input name='ssid' maxlength='32' required style='width:100%;padding:10px'><br><br>"
    "<label>Wi-Fi password</label><br>"
    "<input name='password' type='password' maxlength='63' style='width:100%;padding:10px'><br><br>"
    "<button type='submit' style='padding:10px 20px'>Save Wi-Fi settings</button>"
    "</form></body></html>";

static bool app_wifi_load_credentials(char *ssid, size_t ssid_size,
                                      char *password, size_t password_size)
{
    nvs_handle_t handle;

    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    size_t required_ssid = ssid_size;
    size_t required_password = password_size;

    esp_err_t ssid_result =
        nvs_get_str(handle, NVS_SSID_KEY, ssid, &required_ssid);

    esp_err_t password_result =
        nvs_get_str(handle, NVS_PASSWORD_KEY,
                    password, &required_password);

    nvs_close(handle);

    return ssid_result == ESP_OK && password_result == ESP_OK;
}

static esp_err_t app_wifi_save_credentials(const char *ssid,
                                           const char *password)
{
    nvs_handle_t handle;

    esp_err_t result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

    if (result != ESP_OK) {
        return result;
    }

    result = nvs_set_str(handle, NVS_SSID_KEY, ssid);

    if (result == ESP_OK) {
        result = nvs_set_str(handle, NVS_PASSWORD_KEY, password);
    }

    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }

    nvs_close(handle);

    return result;
}

static void url_decode(char *destination, const char *source, size_t max_length)
{
    size_t output = 0;

    while (*source && output < max_length - 1) {
        if (*source == '+') {
            destination[output++] = ' ';
            source++;
        }
        else if (*source == '%' && source[1] && source[2]) {
            char hex[3] = { source[1], source[2], '\0' };
            destination[output++] = (char)strtol(hex, NULL, 16);
            source += 3;
        }
        else {
            destination[output++] = *source++;
        }
    }

    destination[output] = '\0';
}

static bool get_form_value(const char *body, const char *key,
                           char *output, size_t output_size)
{
    char search[40];

    snprintf(search, sizeof(search), "%s=", key);

    const char *start = strstr(body, search);

    if (start == NULL) {
        return false;
    }

    start += strlen(search);

    const char *end = strchr(start, '&');

    size_t length = end ? (size_t)(end - start) : strlen(start);

    if (length >= output_size * 3) {
        return false;
    }

    char encoded[192];

    if (length >= sizeof(encoded)) {
        return false;
    }

    memcpy(encoded, start, length);
    encoded[length] = '\0';

    url_decode(output, encoded, output_size);

    return true;
}

static esp_err_t portal_get_handler(httpd_req_t *request)
{
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, setup_page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t portal_save_handler(httpd_req_t *request)
{
    if (request->content_len <= 0 || request->content_len >= 512) {
        return httpd_resp_send_err(
            request, HTTPD_400_BAD_REQUEST, "Invalid form data"
        );
    }

    char body[512] = {0};

    int received = httpd_req_recv(
        request, body, request->content_len
    );

    if (received <= 0) {
        return httpd_resp_send_err(
            request, HTTPD_400_BAD_REQUEST, "Could not read form"
        );
    }

    body[received] = '\0';

    char ssid[33] = {0};
    char password[64] = {0};

    if (!get_form_value(body, "ssid", ssid, sizeof(ssid))) {
        return httpd_resp_send_err(
            request, HTTPD_400_BAD_REQUEST, "Wi-Fi name is required"
        );
    }

    get_form_value(body, "password", password, sizeof(password));

    if (strlen(ssid) == 0) {
        return httpd_resp_send_err(
            request, HTTPD_400_BAD_REQUEST, "Wi-Fi name is required"
        );
    }

    esp_err_t result = app_wifi_save_credentials(ssid, password);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Could not save Wi-Fi settings: %s",
                 esp_err_to_name(result));

        return httpd_resp_send_err(
            request, HTTPD_500_INTERNAL_SERVER_ERROR,
            "Could not save settings"
        );
    }

    const char response[] =
        "<html><body style='font-family:Arial;padding:30px'>"
        "<h2>Wi-Fi settings saved</h2>"
        "<p>HAYAG Node 2 is now connecting to your Wi-Fi.</p>"
        "<p>You can close this page.</p>"
        "</body></html>";

    httpd_resp_set_type(request, "text/html");
    httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);

    wifi_config_t station_config = {0};

    strncpy((char *)station_config.sta.ssid, ssid,
            sizeof(station_config.sta.ssid) - 1);

    strncpy((char *)station_config.sta.password, password,
            sizeof(station_config.sta.password) - 1);

    station_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    result = esp_wifi_set_config(WIFI_IF_STA, &station_config);

    if (result == ESP_OK) {
        credentials_available = true;
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

        esp_wifi_disconnect();
        esp_wifi_connect();

        ESP_LOGI(TAG, "New Wi-Fi settings applied");
    }
    else {
        ESP_LOGE(TAG, "Could not apply Wi-Fi settings: %s",
                 esp_err_to_name(result));
    }

    return ESP_OK;
}

static esp_err_t start_portal_server(void)
{
    if (portal_server != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = 80;

    esp_err_t result = httpd_start(&portal_server, &config);

    if (result != ESP_OK) {
        return result;
    }

    httpd_uri_t page_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = portal_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t save_uri = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = portal_save_handler,
        .user_ctx = NULL
    };

    result = httpd_register_uri_handler(portal_server, &page_uri);

    if (result != ESP_OK) {
        return result;
    }

    return httpd_register_uri_handler(portal_server, &save_uri);
}

static esp_err_t start_setup_ap(void)
{
    wifi_config_t ap_config = {0};

    uint8_t random_bytes[6];
    char ap_password[13];

    esp_fill_random(random_bytes, sizeof(random_bytes));

    snprintf(
        ap_password,
        sizeof(ap_password),
        "%02X%02X%02X%02X%02X%02X",
        random_bytes[0],
        random_bytes[1],
        random_bytes[2],
        random_bytes[3],
        random_bytes[4],
        random_bytes[5]
    );

    strncpy((char *)ap_config.ap.ssid,
            SETUP_AP_SSID,
            sizeof(ap_config.ap.ssid) - 1);

    strncpy((char *)ap_config.ap.password,
            ap_password,
            sizeof(ap_config.ap.password) - 1);

    ap_config.ap.ssid_len = strlen(SETUP_AP_SSID);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 2;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    esp_err_t result = esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    if (result != ESP_OK) {
        return result;
    }

    ESP_LOGI(TAG, "Setup hotspot: %s", SETUP_AP_SSID);
    ESP_LOGI(TAG, "Setup password: %s", ap_password);
    ESP_LOGI(TAG, "Open http://192.168.4.1 to configure Wi-Fi");

    return start_portal_server();
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        if (credentials_available) {
            ESP_LOGI(TAG, "Connecting to saved Wi-Fi...");
            esp_wifi_connect();
        }
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

        ESP_LOGW(TAG, "Wi-Fi disconnected");

        if (credentials_available) {
            ESP_LOGI(TAG, "Retrying Wi-Fi connection...");
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);

        ESP_LOGI(TAG, "Connected. IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));
    }

    (void)arg;
}

bool app_wifi_is_connected(void)
{
    if (wifi_event_group == NULL) {
        return false;
    }

    return (xEventGroupGetBits(wifi_event_group) &
            WIFI_CONNECTED_BIT) != 0;
}

esp_err_t app_wifi_start(void)
{
    esp_err_t result = nvs_flash_init();

    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    if (result != ESP_OK) {
        return result;
    }

    result = esp_netif_init();

    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return result;
    }

    result = esp_event_loop_create_default();

    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        return result;
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_event_group = xEventGroupCreate();

    if (wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();

    result = esp_wifi_init(&wifi_config);

    if (result != ESP_OK) {
        return result;
    }

    result = esp_event_handler_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler,
        NULL
    );

    if (result != ESP_OK) {
        return result;
    }

    result = esp_event_handler_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event_handler,
        NULL
    );

    if (result != ESP_OK) {
        return result;
    }

    char ssid[33] = {0};
    char password[64] = {0};

    credentials_available = app_wifi_load_credentials(
        ssid, sizeof(ssid), password, sizeof(password)
    );

    result = esp_wifi_set_mode(WIFI_MODE_APSTA);

    if (result != ESP_OK) {
        return result;
    }

    result = start_setup_ap();

    if (result != ESP_OK) {
        return result;
    }

    if (credentials_available) {
        wifi_config_t station_config = {0};

        strncpy((char *)station_config.sta.ssid, ssid,
                sizeof(station_config.sta.ssid) - 1);

        strncpy((char *)station_config.sta.password, password,
                sizeof(station_config.sta.password) - 1);

        station_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

        result = esp_wifi_set_config(WIFI_IF_STA, &station_config);

        if (result != ESP_OK) {
            return result;
        }

        ESP_LOGI(TAG, "Saved Wi-Fi credentials found");
    }
    else {
        ESP_LOGW(TAG, "No saved Wi-Fi credentials. Use the setup hotspot.");
    }

    return esp_wifi_start();
}
