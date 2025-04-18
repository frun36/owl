#include "owl_softap.h"

#include "esp_log.h"

#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "esp_http_server.h"
#include "esp_netif.h"
#include <stdint.h>

static const char *TAG = "owl_softap";

#define SOFTAP_SSID "OWL"
#define SOFTAP_PASS "FR4#1UL4"
#define SOFTAP_CH 6
#define SOFTAP_MAX_CONN 4

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    // clang-format off
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SOFTAP_SSID,
            .ssid_len = strlen(SOFTAP_SSID),
            .channel = SOFTAP_CH,
            .password = SOFTAP_PASS,
            .max_connection = SOFTAP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
               .required = true,
            },
        },
    };
    // clang-format on

    if (strlen(SOFTAP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Initialized Wi-Fi SoftAP. SSID: %s PASS: %s CH: %d", SOFTAP_SSID, SOFTAP_PASS, SOFTAP_CH);
}

static httpd_handle_t server_handle = NULL;
static int ws_fd = -1;

static esp_err_t root_get_handler(httpd_req_t *req)
{
    static const char *resp = "<!DOCTYPE html><html><body>"
                              "<h1>OWL</h1>"
                              "<h3>Device history:</h3>"
                              "<script>"
                              "const ws = new WebSocket('ws://' + location.host + '/ws');"
                              "ws.onmessage = e => document.body.innerHTML += '<br>' + e.data;"
                              "</script>"
                              "</body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL};

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ws_fd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "WS handshake done (fd = %d)", ws_fd);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {.type = HTTPD_WS_TYPE_TEXT, .payload = NULL, .len = 0};

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame length: %s", esp_err_to_name(ret));
        return ret;
    }

    if (frame.len > 0) {
        frame.payload = malloc(frame.len + 1);
        if (!frame.payload) return ESP_ERR_NO_MEM;

        ret = httpd_ws_recv_frame(req, &frame, frame.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read WS frame: %s", esp_err_to_name(ret));
            free(frame.payload);
            return ret;
        }
        frame.payload[frame.len] = '\0';
        ESP_LOGI(TAG, "Received: %s", (char *) frame.payload);
        free(frame.payload);
    }

    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "WS closed");
        ws_fd = -1;
    }
    return ret;
}

static const httpd_uri_t ws = {
    .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .user_ctx = NULL, .is_websocket = true};

static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &ws);
    }
    return server;
}

void ws_send(const char *message)
{
    if (ws_fd < 0 || server_handle == NULL) {
        ESP_LOGW(TAG, "No active WS connection");
        return;
    }

    httpd_ws_frame_t ws_pkt = {.final = true,
                               .fragmented = false,
                               .type = HTTPD_WS_TYPE_TEXT,
                               .payload = (uint8_t *) message,
                               .len = strlen(message)};

    esp_err_t ret = httpd_ws_send_frame_async(server_handle, ws_fd, &ws_pkt);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send WS message: %s", esp_err_to_name(ret));
        ws_fd = -1;
    }
}

void init_softap_server()
{
    wifi_init_softap();
    server_handle = start_webserver();
}
