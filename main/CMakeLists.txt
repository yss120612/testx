idf_component_register(SRCS "TX433Task.cpp" "hello_world_main.cpp" "EndpointTask.cpp" "MotorTask.cpp" "HTTPServer.cpp" "SpeakerTask.cpp"
                       PRIV_REQUIRES spi_flash
                                     esp_wifi 
                                     esp_http_server
                                     esp_https_ota
                                     app_update
                                     spiffs
                                     nvs_flash
                                     esp_timer
                                     driver
                                     esp_driver_i2c
                                     esp_driver_ledc
                                     Task
                                     WiFiTask
                                     ENS160Task
                                     RX433Task
                                     DISPTask
                                     RMTTask
                                     ButtonTask
                                 
                       INCLUDE_DIRS "."                                      
                       REQUIRES 
                                
                       )
