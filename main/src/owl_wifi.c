#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "owl_wifi.h"

#include "nvs_flash.h"
#include "owl_display.h"

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

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    static size_t s_retry_num = 0;
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);

    if (event_base == WIFI_EVENT
        && event_id == WIFI_EVENT_STA_START) { // STA events
        ESP_LOGI(TAG, "Station started - connecting to WiFi");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT
               && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 3) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG,
                     "Connection to %s: attempt %zu",
                     (const char *) conf.sta.ssid,
                     s_retry_num);
            char msg[17];
            snprintf(msg, 17, "Conn attempt %zu", s_retry_num);
            owl_display((const char *) conf.sta.ssid,
                        msg,
                        owl_rgb(OWL_COLOR_YELLOW),
                        -1);
        } else {
            ESP_LOGI(
                TAG, "Failed to connect to %s: ", (const char *) conf.sta.ssid);
            owl_display((const char *) conf.sta.ssid,
                        "Conn failed",
                        owl_rgb(OWL_COLOR_RED),
                        -1);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) { // AP events
        wifi_event_ap_staconnected_t *event
            = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "Station " MACSTR " joined AP, AID=%d",
                 MAC2STR(event->mac),
                 event->aid);

        owl_display((const char *) conf.sta.ssid,
                    "Connected",
                    owl_rgb(OWL_COLOR_GREEN),
                    -1);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event
            = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "Station " MACSTR " left AP, AID=%d, reason=%d",
                 MAC2STR(event->mac),
                 event->aid,
                 event->reason);
    }
}

void owl_wifi_init(void)
{
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

void owl_apsta(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "APSTA mode");

    owl_display(SOFTAP_SSID, SOFTAP_PASS, owl_rgb(OWL_COLOR_CYAN), -1);
}

void owl_wifi_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA mode");
}
