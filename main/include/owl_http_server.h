#pragma once

#include <stdbool.h>

void owl_http_server_init();

bool owl_ws_is_connected();
void owl_ws_send(const char *message);

