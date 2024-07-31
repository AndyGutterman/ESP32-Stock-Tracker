## Stock Ticker ESP32 LCD1602

### Overview

This project uses an ESP32 microcontroller to create a stock ticker with a 16x2 LCD display. It connects to Wi-Fi to fetch stock data and displays it on the LCD. The system includes a web interface for managing stocks and controlling the ticker, as well as LED indicators to show if a stock is up (green) or down (red). 


### Installation and Usage

1. **Hardware Setup**:
   - Connect the LCD1602 to the ESP32 via I2C. (D21 and D22)
   - Connect LEDs to GPIO 27 (green) and GPIO 26 (red). (D27 and D26)
  
   <img src="https://raw.githubusercontent.com/AndyGutterman/StockTicker-ESP32-LCD1602/main/demo_images/wiring.jpg" alt="Wiring" style="width: 60%; max-width: 400px;">
2. **Software Setup**:
   - Install the Arduino IDE and add ESP32 board support.
   - Install required libraries: `WiFi.h`, `LiquidCrystal_I2C.h`, `SPIFFS.h`.
   - Upload the code to the ESP32.

3. **Web Interface**:
   - Access via the IP address displayed on the LCD.
   - Use the web interface to add/delete stocks, start/stop the ticker loop, and toggle the LCD backlight.

![Stock Controller Demo](https://raw.githubusercontent.com/AndyGutterman/StockTicker-ESP32-LCD1602/main/demo_images/stock_controller_demo.PNG)

<img src="https://raw.githubusercontent.com/AndyGutterman/ESP32-Stock-Tracker/main/demo_images/loop_example.jpg" alt="Wiring" style="width: 60%; max-width: 400px;">


### Todo:
   - Improve handling of large prices and/or changes, improve scrolled formatting.
   - Track date and time that price was generated, only generate new data after a specified interval.
   - Add calls to API to track live data.
