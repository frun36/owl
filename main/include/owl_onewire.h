#pragma once

#include "onewire_types.h"
#include <stddef.h>

onewire_bus_handle_t owl_init_onewire(int bus_gpio_number);
size_t owl_onewire_search(onewire_device_address_t buff[], size_t max_devices);
