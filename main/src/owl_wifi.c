#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"

#include "owl_wifi.h"

#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "nvs_flash.h"
#include "owl_display.h"
#include "owl_http_server.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WIFI_SSID CONFIG_OWL_WIFI_SSID
#define WIFI_PASS CONFIG_OWL_WIFI_PASS

static const char *TAG = "owl_wifi";

#define SOFTAP_SSID CONFIG_OWL_SOFTAP_SSID
#define SOFTAP_PASS CONFIG_OWL_SOFTAP_PASS
#define SOFTAP_CH CONFIG_OWL_SOFTAP_CH
#define SOFTAP_MAX_CONN CONFIG_OWL_SOFTAP_MAX_CONN

static owl_wifi_status_t s_wifi_status = OWL_WIFI_UNKNOWN;
static char s_ip_addr[17] = {};
static SemaphoreHandle_t s_wifi_status_mutex = NULL;

static void set_wifi_status(owl_wifi_status_t status)
{
    if (s_wifi_status_mutex == NULL) {
        ESP_LOGE(TAG, "WiFi status mutex uninitialized");
        return;
    }

    if (xSemaphoreTake(s_wifi_status_mutex, portMAX_DELAY)) {
        s_wifi_status = status;
        xSemaphoreGive(s_wifi_status_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take WiFi status mutex");
    }

    owl_display_update_status();
}

static void set_ip_addr(esp_ip4_addr_t *ip)
{
    if (s_wifi_status_mutex == NULL) {
        ESP_LOGE(TAG, "WiFi status mutex uninitialized");
        return;
    }

    if (xSemaphoreTake(s_wifi_status_mutex, portMAX_DELAY)) {
        if (ip == NULL) {
            snprintf(s_ip_addr, 17, "%s", "");
        } else {
            snprintf(s_ip_addr, 17, IPSTR, IP2STR(ip));
        }
        xSemaphoreGive(s_wifi_status_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take WiFi status mutex");
    }

    owl_display_update_status();
}

owl_wifi_status_t owl_wifi_get_status()
{
    if (s_wifi_status_mutex == NULL) {
        ESP_LOGE(TAG, "WiFi status mutex uninitialized");
        return OWL_WIFI_UNKNOWN;
    }

    owl_wifi_status_t status = OWL_WIFI_UNKNOWN;
    if (xSemaphoreTake(s_wifi_status_mutex, portMAX_DELAY)) {
        status = s_wifi_status;
        xSemaphoreGive(s_wifi_status_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take WiFi status mutex");
    }
    return status;
}

const char *owl_wifi_get_ip_str()
{
    if (s_wifi_status_mutex == NULL) {
        ESP_LOGE(TAG, "WiFi status mutex uninitialized");
        return "-";
    }

    const char *ip = "-";
    if (xSemaphoreTake(s_wifi_status_mutex, portMAX_DELAY)) {
        ip = s_ip_addr;
        xSemaphoreGive(s_wifi_status_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take WiFi status mutex");
    }
    return ip;
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    static size_t s_try_num = 0;
    wifi_config_t sta_conf;
    esp_wifi_get_config(WIFI_IF_STA, &sta_conf);
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);

    switch (event_id) {
    case WIFI_EVENT_STA_START:
        if (wifi_mode != WIFI_MODE_STA)
            break;
        ESP_LOGI(TAG, "Station started - connecting to WiFi");
        set_wifi_status(OWL_WIFI_STA);
        // fall through
    case WIFI_EVENT_STA_DISCONNECTED:
        if (s_try_num < 3) {
            esp_wifi_connect();
            s_try_num++;
            ESP_LOGI(TAG,
                     "Connection to %s: attempt %zu",
                     (const char *) sta_conf.sta.ssid,
                     s_try_num);
            char msg[17];
            snprintf(msg, 17, "Conn attempt %zu", s_try_num);
            owl_display((const char *) sta_conf.sta.ssid,
                        msg,
                        owl_rgb(OWL_COLOR_YELLOW),
                        3000);
        } else {
            ESP_LOGI(TAG,
                     "Failed to connect to %s",
                     (const char *) sta_conf.sta.ssid);
            owl_display("Conn failed",
                        (const char *) sta_conf.sta.ssid,
                        owl_rgb(OWL_COLOR_RED),
                        3000);
            s_try_num = 0;
            set_wifi_status(OWL_WIFI_DISCONNECTED);
            owl_reset_ws_fd();
        }
        break;
    case WIFI_EVENT_STA_CONNECTED:
        s_try_num = 0;
        break;

    // AP events
    case WIFI_EVENT_AP_STACONNECTED: {
        wifi_event_ap_staconnected_t *event
            = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "Station " MACSTR " joined AP, AID=%d",
                 MAC2STR(event->mac),
                 event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED: {
        wifi_event_ap_stadisconnected_t *event
            = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "Station " MACSTR " left AP, AID=%d, reason=%d",
                 MAC2STR(event->mac),
                 event->aid,
                 event->reason);
        break;
    }
    }
}

void ip_event_handler(void *arg,
                      esp_event_base_t event_base,
                      int32_t event_id,
                      void *event_data)
{
    wifi_config_t sta_conf;
    esp_wifi_get_config(WIFI_IF_STA, &sta_conf);

    switch (event_id) {
    case IP_EVENT_STA_GOT_IP: {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        set_ip_addr(&event->ip_info.ip);
        ESP_LOGI(TAG, "Got IP: %s", owl_wifi_get_ip_str());
        owl_display("Connected",
                    (const char *) sta_conf.sta.ssid,
                    owl_rgb(OWL_COLOR_GREEN),
                    3000);
        break;
    }
    }
}

void owl_wifi_init(void)
{
    s_wifi_status_mutex = xSemaphoreCreateMutex();
    if (s_wifi_status_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi status mutex");
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES
        || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL));

    uint8_t mac_sta[6];
    uint8_t mac_ap[6];

    esp_read_mac(mac_sta, ESP_MAC_WIFI_STA);
    esp_read_mac(mac_ap, ESP_MAC_WIFI_SOFTAP);

    ESP_LOGI(TAG,
             "Initialized WiFi. STA MAC: " MACSTR "; AP MAC: " MACSTR,
             MAC2STR(mac_sta),
             MAC2STR(mac_ap));
}

void owl_wifi_configure(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure STA
    static const char *ssid = WIFI_SSID, *pass = WIFI_PASS;

#ifdef CONFIG_OWL_DEV
#include "secrets.inc"
#endif

    wifi_config_t sta_config = {
        .sta = {
            .ssid = "",
            .password = "",
        },
    };
    strncpy((char *) sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid));
    strncpy((char *) sta_config.sta.password,
            pass,
            sizeof(sta_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    // Configure SoftAP
    // clang-format off
    wifi_config_t softap_config = {
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
        softap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &softap_config));
}

void owl_wifi_ap(void)
{
    set_ip_addr(NULL);
    owl_reset_ws_fd();

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP mode");

    set_wifi_status(OWL_WIFI_AP);
}

void owl_wifi_sta(void)
{
    owl_reset_ws_fd();

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA mode");
}
