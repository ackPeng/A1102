
set(priv_requires "esp_mm" "esp_timer")
set(requires "driver" "spiffs")

idf_component_register(
    SRCS "sensecap-a1102.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES ${priv_requires}
    REQUIRES ${requires}
)
