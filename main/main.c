#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"

// Pemetaan PIN berdasarkan diagram Wokwi
#define I2C_MASTER_SDA_IO           21    // SDA -> Pin 21
#define I2C_MASTER_SCL_IO           19    // SCL -> Pin 19
#define BUTTON_PIN                  8   // Tombol -> Pin 8

// Konfigurasi UART untuk menerima instruksi serial
#define UART_PORT_NUM               UART_NUM_0
#define BUF_SIZE                    1024

// Register MPU6050
#define MPU6050_I2C_ADDR            0x68  
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_ACCEL_XOUT_H        0x3B

i2c_master_dev_handle_t mpu6050_dev;

void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
}

void init_hardware_dan_i2c(void) {
    // 1. Konfigurasi GPIO
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(BUTTON_PIN);
    gpio_pullup_dis(BUTTON_PIN); 

    // 2. Inisialisasi Bus I2C Master
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    // 3. Daftarkan MPU6050
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &mpu6050_dev));

    // 4. Bangunkan MPU6050
    uint8_t wake_cmd[2] = {MPU6050_PWR_MGMT_1, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(mpu6050_dev, wake_cmd, sizeof(wake_cmd), -1));
}

void app_main(void) {
    printf("Inisialisasi Sistem...\n");
    init_uart();
    init_hardware_dan_i2c();
    printf("Sistem Siap. Menunggu masukan...\n");

    uint8_t raw_data[14];
    uint8_t start_reg = MPU6050_ACCEL_XOUT_H;

    while (1) {
        int current_button_level = gpio_get_level(BUTTON_PIN);

        // HANYA mengeksekusi jika tombol BARU saja ditekan (0 ke 1) ATAU dipicu oleh serial
        if ((current_button_level == 1)) {
            if (current_button_level == 1) {
                printf("\n--- Tombol ditekan, Pin 2 HIGH ---\n");
            } else {
                printf("\n--- Trigger Serial Diterima, Pin 2 HIGH ---\n");
            }

            // Membaca register data asli dari simulator MPU6050
            esp_err_t err = i2c_master_transmit_receive(mpu6050_dev, &start_reg, 1, raw_data, 14, -1);
            
            if (err == ESP_OK) {
                int16_t raw_ax = (raw_data[0] << 8) | raw_data[1];
                int16_t raw_ay = (raw_data[2] << 8) | raw_data[3];
                int16_t raw_az = (raw_data[4] << 8) | raw_data[5];
                int16_t raw_temp = (raw_data[6] << 8) | raw_data[7];
                int16_t raw_gx = (raw_data[8] << 8) | raw_data[9];
                int16_t raw_gy = (raw_data[10] << 8) | raw_data[11];
                int16_t raw_gz = (raw_data[12] << 8) | raw_data[13];

                // Mengonversi data mentah ke satuan G-Force (float) sesuai dengan Automation Controls Wokwi
                float ax = (float)raw_ax / 16384.0;
                float ay = (float)raw_ay / 16384.0;
                float az = (float)raw_az / 16384.0;
                float temperature = ((float)raw_temp / 340.0) + 36.53;
                int gx = raw_gx / 131;
                int gy = raw_gy / 131;
                int gz = raw_gz / 131;

                // Format cetak ringkas yang ditunggu oleh wait-serial di YAML
                printf("DATA_MPU_OK\n");
                printf("Acceleration : X=%.2f g, Y=%.2f g, Z=%.2f g\n", ax, ay, az);
                printf("Rotation     : X=%d °/sec, Y=%d °/sec, Z=%d °/sec\n", gx, gy, gz);
                printf("Temperature  : %.2f °C\n", temperature);
                printf("----------------------------------\n"); 
            } else {
                printf("Gagal mengambil data dari MPU6050!\n");
            }
        }
        // Jeda waktu pemicu loop demi stabilitas pembacaan simulator
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}