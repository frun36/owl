#pragma once

typedef enum {
    OWL_WIFI_UNKNOWN,
    OWL_WIFI_DISCONNECTED,
    OWL_WIFI_STA,
    OWL_WIFI_AP,
} owl_wifi_status_t;

owl_wifi_status_t owl_wifi_get_status();
const char *owl_wifi_get_ip_str();

void owl_wifi_init(void);

void owl_wifi_configure(void);

void owl_wifi_sta(void);
void owl_wifi_ap(void);
