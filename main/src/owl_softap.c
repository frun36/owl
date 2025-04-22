#include "owl_softap.h"

#include "esp_log.h"

#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"

#include "esp_netif.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "owl_softap";

#define SOFTAP_SSID "OWL"
#define SOFTAP_PASS "FR4#1UL4"
#define SOFTAP_CH 6
#define SOFTAP_MAX_CONN 4

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event
            = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac),
                 event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event
            = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG,
                 "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac),
                 event->aid,
                 event->reason);
    }
}

void owl_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

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

    ESP_LOGI(TAG,
             "Initialized Wi-Fi SoftAP. SSID: %s PASS: %s CH: %d",
             SOFTAP_SSID,
             SOFTAP_PASS,
             SOFTAP_CH);
}
