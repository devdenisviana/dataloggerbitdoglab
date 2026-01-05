/**
 * @file sd_card.c
 * @author Denis Viana
 * @date 2025
 * @brief BitDogLab Datalogger - Logs button and joystick events to SD card
 * 
 * This firmware controls LEDs, buzzer, and OLED display based on user inputs
 * from buttons and joystick, logging all events with timestamps to an SD card.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "ff.h"
#include "diskio.h"
#include "inc/ssd1306.h"
#include "inc/ssd1306_fonts.h"

// === Pin Definitions ===
#define RED_LED      13
#define GREEN_LED    11
#define BLUE_LED     12
#define BUZZER       21
#define BUTTON_A     5
#define BUTTON_B     6
#define JOY_X        26  // ADC0
#define JOY_Y        27  // ADC1

#define PIN_MISO     16
#define PIN_MOSI     19
#define PIN_SCK      18
#define PIN_CS       17

#define SDA_I2C      8
#define SCL_I2C      9
#define I2C_PORT     i2c0

// === Configuration Constants ===
#define SPI_BAUDRATE        1000000  // 1 MHz
#define I2C_BAUDRATE        100000   // 100 kHz
#define LOG_FILENAME        "bitdoglab.txt"
#define DEBOUNCE_MS         50       // Button debounce time
#define LED_DURATION_MS     300      // LED on duration
#define LOOP_DELAY_MS       50       // Main loop delay
#define JOY_MIN_THRESHOLD   1000     // Joystick minimum threshold
#define JOY_MAX_THRESHOLD   3000     // Joystick maximum threshold
#define ADC_MAX_VALUE       4095     // 12-bit ADC

// === Global Variables ===
FATFS fs;
FIL file;
bool sd_card_ready = false;

// Button state tracking for debounce
static uint32_t last_button_time_a = 0;
static uint32_t last_button_time_b = 0;
static uint32_t last_joystick_time = 0;
static bool last_button_a_state = false;
static bool last_button_b_state = false;

// === Initialize I2C for OLED ===
void init_i2c(void) {
    i2c_init(I2C_PORT, I2C_BAUDRATE);
    gpio_set_function(SDA_I2C, GPIO_FUNC_I2C);
    gpio_set_function(SCL_I2C, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_I2C);
    gpio_pull_up(SCL_I2C);
}

// === Initialize OLED Display ===
void init_oled(void) {
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("BitDogLab v1.0", Font_6x8, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("Initializing...", Font_6x8, White);
    ssd1306_UpdateScreen();
}

// === Initialize GPIO Pins ===
void init_gpio(void) {
    // Output pins
    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_put(RED_LED, 0);
    
    gpio_init(GREEN_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);
    gpio_put(GREEN_LED, 0);
    
    gpio_init(BLUE_LED);
    gpio_set_dir(BLUE_LED, GPIO_OUT);
    gpio_put(BLUE_LED, 0);
    
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);

    // Input pins with pull-up
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
}

// === Initialize ADC for Joystick ===
void init_adc(void) {
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);
}

// === Initialize SD card via SPI ===
bool init_sd_card(void) {
    spi_init(spi0, SPI_BAUDRATE);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("ERROR: Failed to mount SD card (error %d)\n", fr);
        return false;
    }
    
    printf("SD card mounted successfully\n");
    
    fr = f_open(&file, LOG_FILENAME, FA_WRITE | FA_OPEN_APPEND);
    if (fr != FR_OK) {
        printf("ERROR: Failed to open log file (error %d)\n", fr);
        return false;
    }
    
    // Write header only if file is empty
    FSIZE_t size = f_size(&file);
    if (size == 0) {
        f_puts("Event,Timestamp_ms\n", &file);
        f_sync(&file);
        printf("Log file created with header\n");
    } else {
        printf("Appending to existing log file\n");
    }
    
    return true;
}

// === Log event to SD with timestamp ===
void log_event(const char* event) {
    if (!sd_card_ready) {
        printf("WARNING: SD card not ready, event not logged: %s\n", event);
        return;
    }
    
    char line[128];
    uint32_t timestamp = to_ms_since_boot(get_absolute_time());
    snprintf(line, sizeof(line), "%s,%lu\n", event, timestamp);
    
    UINT bytes_written;
    FRESULT fr = f_write(&file, line, strlen(line), &bytes_written);
    
    if (fr != FR_OK || bytes_written != strlen(line)) {
        printf("ERROR: Failed to write to log file (error %d)\n", fr);
        sd_card_ready = false;
    } else {
        f_sync(&file);
        printf("Event logged: %s", line);
    }
}

// === Display event on OLED ===
void display_event(const char* message) {
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("EVENT DETECTED", Font_6x8, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString((char*)message, Font_6x8, White);
    ssd1306_SetCursor(0, 40);
    
    char status[32];
    snprintf(status, sizeof(status), "SD: %s", sd_card_ready ? "OK" : "ERROR");
    ssd1306_WriteString(status, Font_6x8, White);
    ssd1306_UpdateScreen();
}

// === Check button state with debounce ===
bool is_button_pressed(uint8_t pin, bool* last_state, uint32_t* last_time) {
    bool current_state = !gpio_get(pin);  // Active low
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Detect rising edge with debounce
    if (current_state && !(*last_state) && (current_time - *last_time) > DEBOUNCE_MS) {
        *last_state = current_state;
        *last_time = current_time;
        return true;
    }
    
    *last_state = current_state;
    return false;
}

// === Handle LED with automatic turn-off ===
void blink_led(uint8_t led_pin, const char* event_msg) {
    gpio_put(led_pin, 1);
    log_event(event_msg);
    display_event(event_msg);
    sleep_ms(LED_DURATION_MS);
    gpio_put(led_pin, 0);
}

// === Handle buzzer activation ===
void activate_buzzer(void) {
    gpio_put(BUZZER, 1);
    log_event("BUZZER_ACTIVATED");
    display_event("BUZZER ACTIVATED");
    sleep_ms(LED_DURATION_MS);
    gpio_put(BUZZER, 0);
}

// === Check joystick movement ===
bool check_joystick_movement(void) {
    adc_select_input(0);
    uint16_t x = adc_read();
    
    adc_select_input(1);
    uint16_t y = adc_read();
    
    return (x < JOY_MIN_THRESHOLD || x > JOY_MAX_THRESHOLD || 
            y < JOY_MIN_THRESHOLD || y > JOY_MAX_THRESHOLD);
}

// === Main function ===
int main(void) {
    // Initialize standard I/O
    stdio_init_all();
    
    // Wait for USB connection (optional - can be removed for standalone operation)
    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!stdio_usb_connected() && (to_ms_since_boot(get_absolute_time()) - start_time) < 3000) {
        sleep_ms(100);
    }
    
    printf("\n=== BitDogLab Datalogger v1.0 ===\n");
    printf("Author: Denis Viana (2025)\n\n");

    // Initialize all peripherals
    printf("Initializing I2C...\n");
    init_i2c();
    
    printf("Initializing OLED...\n");
    init_oled();
    sleep_ms(1000);
    
    printf("Initializing GPIO...\n");
    init_gpio();
    
    printf("Initializing ADC...\n");
    init_adc();
    
    printf("Initializing SD card...\n");
    sd_card_ready = init_sd_card();
    
    if (sd_card_ready) {
        printf("System ready!\n");
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("System Ready", Font_6x8, White);
        ssd1306_SetCursor(0, 16);
        ssd1306_WriteString("Waiting input", Font_6x8, White);
        ssd1306_UpdateScreen();
    } else {
        printf("WARNING: Running without SD card logging\n");
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("SD CARD ERROR", Font_6x8, White);
        ssd1306_SetCursor(0, 16);
        ssd1306_WriteString("Check card!", Font_6x8, White);
        ssd1306_UpdateScreen();
    }

    // === Main loop ===
    printf("\nEntering main loop...\n");
    
    while (true) {
        // Check Button A with debounce
        if (is_button_pressed(BUTTON_A, &last_button_a_state, &last_button_time_a)) {
            // Check if both buttons pressed simultaneously
            if (!gpio_get(BUTTON_B)) {
                activate_buzzer();
            } else {
                blink_led(RED_LED, "BUTTON_A_PRESSED");
            }
        }
        
        // Check Button B with debounce
        if (is_button_pressed(BUTTON_B, &last_button_b_state, &last_button_time_b)) {
            // Check if both buttons pressed simultaneously
            if (!gpio_get(BUTTON_A)) {
                activate_buzzer();
            } else {
                blink_led(GREEN_LED, "BUTTON_B_PRESSED");
            }
        }

        // Check joystick movement with throttling
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if ((current_time - last_joystick_time) > LED_DURATION_MS) {
            if (check_joystick_movement()) {
                blink_led(BLUE_LED, "JOYSTICK_MOVED");
                last_joystick_time = current_time;
            }
        }

        sleep_ms(LOOP_DELAY_MS);
    }

    // Cleanup (never reached in this implementation)
    f_close(&file);
    f_unmount("");
    
    return 0;
}
