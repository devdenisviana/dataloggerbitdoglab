# BitDogLab Datalogger System

[![Language](https://img.shields.io/badge/Language-C-00599C?logo=c)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico-C51A4A?logo=raspberry-pi)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![Storage](https://img.shields.io/badge/Storage-SD%20Card-orange)](https://en.wikipedia.org/wiki/SD_card)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE.txt)

## 📋 Overview

BitDogLab Datalogger is an embedded firmware solution designed for the Raspberry Pi Pico W platform that provides real-time event logging capabilities. The system captures user interactions through physical buttons and an analog joystick, providing immediate visual feedback via RGB LEDs and an OLED display while simultaneously recording all events with precise timestamps to an SD card storage medium.

This project serves as a comprehensive example of embedded systems integration, combining multiple peripherals and communication protocols (SPI, I2C) in a cohesive, production-ready application.

**Author:** Denis Viana  
**Year:** 2025  
**Version:** 1.0

---

## ✨ Key Features

- **Real-time Event Logging**: All user interactions are timestamped and stored in CSV format on SD card
- **Multi-peripheral Integration**: Seamless coordination between buttons, joystick, LEDs, buzzer, OLED display, and SD storage
- **Robust Error Handling**: Graceful degradation when SD card is unavailable
- **Visual Feedback System**: Instant confirmation of user actions through LED indicators and OLED messages
- **Debounced Input**: Hardware debouncing for reliable button press detection
- **Non-blocking Architecture**: Responsive system with optimized polling intervals
- **Professional Code Quality**: Modular design, comprehensive documentation, and maintainable structure

---

## 🔧 Hardware Requirements

### Essential Components

| Component | Specification | Purpose |
|-----------|---------------|---------|
| **Microcontroller** | Raspberry Pi Pico W | Main processing unit |
| **Storage** | MicroSD Card (FAT32) | Event data persistence |
| **Display** | SSD1306 OLED (128x64) | Visual status feedback |
| **Input Devices** | 2x Tactile Buttons + Analog Joystick | User interaction |
| **Output Indicators** | RGB LED (3 channels) + Buzzer | Event confirmation |

### Pin Configuration

#### SD Card Module (SPI0)
- **MISO** → GPIO 16
- **MOSI** → GPIO 19
- **SCK** → GPIO 18
- **CS** → GPIO 17

#### OLED Display (I2C0)
- **SDA** → GPIO 8
- **SCL** → GPIO 9

#### User Interface
- **Button A** → GPIO 5 (pull-up)
- **Button B** → GPIO 6 (pull-up)
- **Joystick X-axis** → GPIO 26 (ADC0)
- **Joystick Y-axis** → GPIO 27 (ADC1)

#### Visual/Audio Output
- **Red LED** → GPIO 13
- **Green LED** → GPIO 11
- **Blue LED** → GPIO 12
- **Buzzer** → GPIO 21

---

## 📁 Project Structure

```
SD_CARD-BITDOGLAB-master/
├── sd_card.c              # Main application firmware
├── hw_config.c            # Hardware configuration for SD driver
├── CMakeLists.txt         # Build configuration
├── pico_sdk_import.cmake  # Pico SDK integration
├── LICENSE.txt            # MIT License
├── README.md              # This file
├── inc/                   # OLED display drivers
│   ├── ssd1306.c/.h       # SSD1306 OLED controller
│   ├── ssd1306_fonts.c/.h # Font rendering
│   └── ssd1306_conf.h     # Display configuration
└── lib/                   # External libraries
    └── FatFs_SPI/         # FAT filesystem implementation
        ├── ff15/          # FatFs core library
        ├── sd_driver/     # SD card SPI driver
        └── include/       # Library headers
```

---

## 🚀 Getting Started

### Prerequisites

- **Pico SDK** (v1.5.1 or later) properly configured
- **CMake** (v3.13+)
- **ARM GCC Toolchain** (arm-none-eabi-gcc)
- **VS Code** with Raspberry Pi Pico extension (recommended)

### Installation Steps

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd SD_CARD-BITDOGLAB-master
   ```

2. **Prepare SD Card**
   - Format SD card as FAT32
   - Insert into SD card module

3. **Build the Project**
   
   Using VS Code:
   - Open project folder in VS Code
   - Run task: **Compile Project** (Ctrl+Shift+B)
   
   Using Command Line:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j4
   ```

4. **Flash to Pico W**
   
   **Method 1 - BOOTSEL Mode:**
   - Hold BOOTSEL button on Pico W
   - Connect USB cable to computer
   - Release BOOTSEL button
   - Copy `dist_card.uf2` from `build/` to the mounted drive
   
   **Method 2 - VS Code:**
   - Connect Pico W via USB
   - Run task: **Run Project**

5. **Verify Operation**
   - Open serial monitor (115200 baud)
   - Observe initialization messages
   - Check OLED display shows "System Ready"

---

## 💡 Operation Guide

### System Behavior

#### Initialization Sequence
1. System displays boot message on OLED
2. Initializes I2C bus for OLED communication
3. Configures GPIO pins for buttons, LEDs, and buzzer
4. Initializes ADC for joystick reading
5. Mounts SD card and opens/creates log file
6. Displays ready status

#### User Interactions

| Action | Visual Response | Log Entry | Notes |
|--------|-----------------|-----------|-------|
| Press **Button A** | Red LED blinks 300ms | `BUTTON_A_PRESSED` | Single button only |
| Press **Button B** | Green LED blinks 300ms | `BUTTON_B_PRESSED` | Single button only |
| Press **Both Buttons** | Buzzer sounds 300ms | `BUZZER_ACTIVATED` | Simultaneous press |
| Move **Joystick** | Blue LED blinks 300ms | `JOYSTICK_MOVED` | Beyond threshold zone |

#### OLED Display Information
- **Line 1**: Event type description
- **Line 2**: Event details
- **Line 3**: SD card status (OK/ERROR)

### Data Logging Format

Events are recorded in CSV format:

```csv
Event,Timestamp_ms
BUTTON_A_PRESSED,1234
JOYSTICK_MOVED,5678
BUZZER_ACTIVATED,9012
BUTTON_B_PRESSED,12345
```

- **Event**: Descriptive event identifier
- **Timestamp_ms**: Milliseconds since system boot

---

## 🛠️ Configuration

Key parameters can be adjusted in `sd_card.c`:

```c
#define DEBOUNCE_MS         50      // Button debounce time (ms)
#define LED_DURATION_MS     300     // LED/Buzzer activation duration (ms)
#define LOOP_DELAY_MS       50      // Main loop polling interval (ms)
#define JOY_MIN_THRESHOLD   1000    // Joystick lower threshold
#define JOY_MAX_THRESHOLD   3000    // Joystick upper threshold
#define LOG_FILENAME        "bitdoglab.txt"  // SD card log file
```

---

## 🔍 Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| SD card not detected | Improper connections | Verify SPI wiring, check power supply |
| OLED blank | I2C address mismatch | Verify I2C address in `ssd1306_conf.h` |
| Button not responding | Debounce too aggressive | Reduce `DEBOUNCE_MS` value |
| Joystick too sensitive | Threshold too wide | Adjust `JOY_MIN/MAX_THRESHOLD` |
| Log file corrupted | Power loss during write | Ensure stable power supply |

### Debug Output

Enable serial monitoring at **115200 baud** to view:
- Initialization status
- Event logging confirmation
- Error messages with diagnostic codes

---

## 📚 Dependencies

- **Pico SDK**: Core framework for RP2040
- **FatFs**: FAT filesystem implementation (ChaN, 2021)
- **SSD1306 Driver**: OLED display controller library
- **Hardware Libraries**: SPI, I2C, ADC, GPIO

---

## 📄 License

This project is licensed under the **MIT License** - see [LICENSE.txt](LICENSE.txt) for details.

Copyright (c) 2025 Denis Viana

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for:
- Bug fixes
- Feature enhancements
- Documentation improvements
- Hardware configuration variants

---

## 📞 Support

For questions, issues, or suggestions, please open an issue in the repository.

---

**Happy Logging! 🎯**

