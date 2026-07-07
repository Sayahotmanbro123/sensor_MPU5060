#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Berdasarkan diagram.json, tombol terhubung ke GPIO 12
#define BUTTON_PIN GPIO_NUM_12 

void app_main(void)
{
    // 1. Reset konfigurasi pin GPIO 12
    gpio_reset_pin(BUTTON_PIN);
    
    // 2. Atur GPIO 12 sebagai Input
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    
    // 3. Aktifkan Pull-Down internal karena jalur tombol menuju ke 3V3
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLDOWN_ONLY);

    ESP_LOGI("MAIN", "Sistem Kontrol Tombol Dimulai. Menunggu input...");

    while (1) {
        // Karena menggunakan rangkaian Pull-Down, tombol ditekan = HIGH (1)
        if (gpio_get_level(BUTTON_PIN) == 1) { 
            
            // Mencetak pesan ke Serial Monitor (Dicari oleh wokwi_test.yaml)
            printf("Button 1 pressed\n"); 
            
            // Delay debounce & menahan pengiriman teks bertubi-tubi saat tombol ditahan
            vTaskDelay(pdMS_TO_TICKS(500)); 
        }
        
        // Jeda pembacaan berkala agar CPU tidak bekerja 100% terus-menerus
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}