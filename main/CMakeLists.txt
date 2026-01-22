idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio
                       PRIV_REQUIRES esp_wifi 
                                     nvs_flash 
                                     esp_netif 
                                     esp_event
                                     driver
                                     espressif__iot_bridge 
                                     mesh_lite)