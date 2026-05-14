# FreeRTOS_Realtime_Sensor
Built a multitasking FreeRTOS application on an ESP32-WROOM-32E, pinning an LED blink task and a DHT11 sensor task to separate cores via xTaskCreatePinnedToCore(), streaming live temperature, humidity, and heat index over UART using cooperative vTaskDelay() scheduling.
