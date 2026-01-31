idf_component_register(SRCS "hello_world_main.cpp" "EndpointTask.cpp" "MotorTask.cpp" "HTTPServer.cpp" "SpeakerTask.cpp" "RX433Task.cpp"
                       PRIV_REQUIRES spi_flash
                                     esp_wifi 
                                     esp_http_server
                                     esp_https_ota
                                     app_update
                                     spiffs
                                     nvs_flash
                                     driver
                                     esp_driver_i2c
                                     esp_driver_ledc
                                     Task
                                     WiFiTask
                                     ENS160Task
				                     RMTTask
                                     ButtonTask
                                 
                       INCLUDE_DIRS "."                                      
                       REQUIRES 
                                
                       )
