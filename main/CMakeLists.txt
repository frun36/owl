idf_component_register(
    SRCS 
    "owl_main.c" 
    "src/owl_led.c" 
    "src/owl_onewire.c" 
    "src/owl_wifi.c" 
    "src/owl_button.c" 
    "src/owl_http_server.c"
    "src/owl_lcd.c"

    INCLUDE_DIRS "include/" "."
)
