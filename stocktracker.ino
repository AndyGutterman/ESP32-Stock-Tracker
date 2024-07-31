#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <vector>
#include <FS.h>
#include <SPIFFS.h>

// Wi-Fi credentials
const char* ssid = "ssid";
const char* password = "pass";

WiFiServer server(80);

// LED pins
const int greenLED = 27;
const int redLED = 26;

// Set up the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Structure to store stock data
struct Stock {
  String ticker;
  float yesterdayPrice;
  float todayPrice;
};

// List of stocks
std::vector<Stock> stocks;

// Flag to control looping
bool loopEnabled = false;  // Start with the loop disabled
size_t currentStockIndex = 0;  // Index for looping through stocks
bool lcdOn = true;  // LCD is on by default

// Scrolling variables
unsigned long lastScrollTime = 0;
int scrollPosition = 0;
const int scrollSpeed = 300;  // Time between scroll updates in milliseconds

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Initialize the LEDs as outputs
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS.");
    return;
  }
  
  // Load stocks from SPIFFS
  loadStocksFromSPIFFS();

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();

  // Show IP address on LCD
  showIPAddress();
}

void saveStocksToSPIFFS() {
  File file = SPIFFS.open("/stocks.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return;
  }
  for (const Stock& stock : stocks) {
    file.printf("%s,%f,%f\n", stock.ticker.c_str(), stock.yesterdayPrice, stock.todayPrice);
  }
  file.close();
}

void loadStocksFromSPIFFS() {
  File file = SPIFFS.open("/stocks.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return;
  }
  stocks.clear();
  while (file.available()) {
    String line = file.readStringUntil('\n');
    int commaIndex1 = line.indexOf(',');
    int commaIndex2 = line.indexOf(',', commaIndex1 + 1);
    if (commaIndex1 > 0 && commaIndex2 > commaIndex1) {
      String ticker = line.substring(0, commaIndex1);
      float yesterdayPrice = line.substring(commaIndex1 + 1, commaIndex2).toFloat();
      float todayPrice = line.substring(commaIndex2 + 1).toFloat();
      stocks.push_back({ticker, yesterdayPrice, todayPrice});
    }
  }
  file.close();
}

void showIPAddress() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP ADDRESS");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  String currentLine = "";
  String request = "";
  while (client.connected() && client.available()) {
    char c = client.read();
    request += c;
    if (c == '\n') {
      if (currentLine.length() == 0) {
        // Process the request
        if (request.indexOf("GET /add?") >= 0) {
          int start = request.indexOf("ticker=") + 7;
          int end = request.indexOf(" ", start);
          String ticker = request.substring(start, end);
          // Add dummy stock data (replace this with real API call)
          stocks.push_back({ticker, random(10000, 20000) / 100.0, random(10000, 20000) / 100.0});
          saveStocksToSPIFFS();  // Save updated stock list
          loopEnabled = true;    // Start loop when a ticker is added
        } else if (request.indexOf("GET /delete?") >= 0) {
          int start = request.indexOf("ticker=") + 7;
          int end = request.indexOf(" ", start);
          String ticker = request.substring(start, end);
          // Remove the stock ticker
          stocks.erase(std::remove_if(stocks.begin(), stocks.end(), [&ticker](Stock& s) { return s.ticker == ticker; }), stocks.end());
          saveStocksToSPIFFS();  // Save updated stock list
          loopEnabled = true;    // Start loop when a ticker is deleted
        } else if (request.indexOf("GET /start") >= 0) {
          loopEnabled = true;
        } else if (request.indexOf("GET /stop") >= 0) {
          loopEnabled = false;
          // Show IP address on LCD
          showIPAddress();
        } else if (request.indexOf("GET /lcd_on") >= 0) {
          lcd.backlight();  // Turn on LCD backlight
          lcdOn = true;
        } else if (request.indexOf("GET /lcd_off") >= 0) {
          lcd.noBacklight();  // Turn off LCD backlight
          lcdOn = false;
        }

        // Send the HTTP response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();

        client.println("<!DOCTYPE html><html>");
        client.println("<head><title>Stock Controller</title></head>");
        client.println("<body><h1>Stock Controller</h1>");
        client.println("<form action='/add'>Add Ticker: <input type='text' name='ticker'><input type='submit' value='Add'></form>");
        client.println("<form action='/delete'>Delete Ticker: <input type='text' name='ticker'><input type='submit' value='Delete'></form>");
        client.println("<a href='/start'>Start Loop</a><br>");
        client.println("<a href='/stop'>Stop Loop</a><br>");
        client.println("<a href='/lcd_on'>Turn LCD On</a><br>");
        client.println("<a href='/lcd_off'>Turn LCD Off</a><br>");
        client.println("<h2>Current Stocks</h2><ul>");
        for (const Stock& stock : stocks) {
          client.println("<li>" + stock.ticker + " - Yesterday: $" + String(stock.yesterdayPrice, 2) + " - Today: $" + String(stock.todayPrice, 2) + "</li>");
        }
        client.println("</ul></body></html>");

        break;
      } else {
        currentLine = "";
      }
    } else if (c != '\r') {
      currentLine += c;
    }
  }

  // Close the connection
  client.stop();
}

void loop() {
  handleClient();

  if (loopEnabled) {
    if (!stocks.empty()) {
      size_t stockCount = stocks.size();
      Stock& stock = stocks[currentStockIndex];

      // Print the stock ticker and prices to the Serial Monitor
      Serial.print("Stock: ");
      Serial.print(stock.ticker);
      Serial.print(" | Yesterday: $");
      Serial.print(stock.yesterdayPrice, 2);
      Serial.print(" | Today: $");
      Serial.println(stock.todayPrice, 2);

      // Calculate the percentage change
      float percentChange = ((stock.todayPrice - stock.yesterdayPrice) / stock.yesterdayPrice) * 100;

      // Display the stock ticker and prices on the LCD if enabled
      if (lcdOn) {
        lcd.clear();
        lcd.setCursor(0, 0);
        // First line: Stock ticker and today's price
        String firstLine = stock.ticker + " T:$" + String(stock.todayPrice, 2);
        if (firstLine.length() > lcdColumns) {
          firstLine = firstLine.substring(0, lcdColumns);
        }
        lcd.print(firstLine);

        // Second line: Percentage change and yesterday's price
        String secondLine = String(percentChange, 1) + "% Y:$" + String(stock.yesterdayPrice, 2);
        
        // Handle scrolling for the second line
        if (secondLine.length() > lcdColumns) {
          unsigned long currentTime = millis();
          if (currentTime - lastScrollTime >= scrollSpeed) {
            lastScrollTime = currentTime;
            scrollPosition = (scrollPosition + 1) % (secondLine.length() - lcdColumns + 1);
          }
          String displayText = secondLine.substring(scrollPosition, scrollPosition + lcdColumns);
          lcd.setCursor(0, 1);
          lcd.print(displayText);
        } else {
          lcd.setCursor(0, 1);
          lcd.print(secondLine);
        }
      }

      // Check if the stock is up or down
      if (stock.todayPrice > stock.yesterdayPrice) {
        // Stock is up
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        Serial.println("Stock is up!");
      } else {
        // Stock is down
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        Serial.println("Stock is down!");
      }

      // Wait for a few seconds before checking the next stock
      delay(5000);

      // Turn off LEDs before displaying the next stock
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, LOW);

      // Move to the next stock ticker
      currentStockIndex = (currentStockIndex + 1) % stockCount;
    } else {
      // No stocks, show a message on the LCD if enabled
      if (lcdOn) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No stocks to show");
        lcd.setCursor(0, 1);
        lcd.print("Waiting...");
      }
      delay(5000);  // Wait before the next loop iteration

      // Reset the index and show the IP address
      currentStockIndex = 0;
      if (lcdOn) {
        lcd.clear();
        showIPAddress();
      }
      loopEnabled = false;  // Stop the loop to prevent continuous error state
    }
  }
}
