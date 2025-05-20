#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <HX711_ADC.h>
#include <Adafruit_SH110X.h>
#include <OneButton.h>
#include <driver/rtc_io.h>
#include <WiFiManager.h>

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
auto constexpr i2c_Address = 0x3c; // initialize with the I2C addr 0x3C Typically eBay OLED's
                                   //  e.g. the one with GM12864-77 written on it
// #define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

auto constexpr screen_width = 128; // OLED display width, in pixels
auto constexpr screen_height = 64; // OLED display height, in pixels
auto constexpr screen_reset = -1;  //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(screen_width, screen_height, &Wire, screen_reset);

auto constexpr hx711_dout_pin = 9;
auto constexpr hx711_clk_pin = 8;
auto constexpr vcc_peripherial_pin = 4;
auto constexpr button_tare_pin = 5;
auto constexpr button_on_off_pin = 6;
auto constexpr sda_pin = 7;
auto constexpr slc_pin = 44;
auto constexpr voltage_reading_pin = 1;
HX711_ADC loadCell{hx711_dout_pin, hx711_clk_pin};
OneButton button_tare{};
OneButton button_on_off{};

void tare_click()
{
  Serial.println("tare");
  display.clearDisplay();
  display.setCursor(0, 16);
  display.println("");
  display.println("taring...");
  display.display();
  loadCell.tare();
}

bool woked_up()
{
  auto const wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
  case ESP_SLEEP_WAKEUP_EXT1:
  case ESP_SLEEP_WAKEUP_TIMER:
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
  case ESP_SLEEP_WAKEUP_ULP:
    return true;
  default:
    return false;
  }
}

float read_battery_voltage()
{
  uint32_t value = 0;
  for (int i = 0; i < 16; i++)
  {
    value = value + analogReadMilliVolts(voltage_reading_pin); // ADC with correction
  }
  float voltage = 2 * value / 16 / 1000.0; // attenuation ratio 1/2, mV --> V
  return voltage;
}

void on_off_click()
{
  // no need, as the peripherial is connected via the vcc pin and it's high only
  // active time, not when deep sleep
  // loadCell.powerDown();
  // display.oled_command(SH110X_DISPLAYOFF);
  // auto const hx711_gpio_numner = static_cast<gpio_num_t>(hx711_clk_pin);
  // rtc_gpio_pulldown_dis(hx711_gpio_numner);
  // rtc_gpio_pullup_en(hx711_gpio_numner);
  // auto const gpio_wake_button_number = static_cast<gpio_num_t>(button_on_off_pin);
  // esp_sleep_enable_ext0_wakeup(gpio_wake_button_number, 1); // High
  // rtc_gpio_pullup_dis(gpio_wake_button_number);
  // rtc_gpio_pulldown_en(gpio_wake_button_number);
  digitalWrite(vcc_peripherial_pin, LOW);
  WiFi.disconnect(true);
  WiFiManager wifi_manager;
  wifi_manager.disconnect();
  delay(1000);
  Serial.println("Sleep");
  esp_deep_sleep_start();
}

void setup_wifi()
{
  WiFiManager wifi_manager;
  if (woked_up())
  {
    WiFi.mode(WIFI_OFF);
    delay(1);
    wifi_manager.reboot();
  }
  {
    esp_sleep_disable_wifi_wakeup();
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);

    display.clearDisplay();
    display.setCursor(0, 16);
    display.println("");
    display.println("setup wifi");
    display.display();
    
    wifi_manager.setTimeout(60);
    wifi_manager.autoConnect("fellow-scale", "password");
  }
}

void setup()
{
  pinMode(voltage_reading_pin, INPUT); // ADC
  pinMode(vcc_peripherial_pin, OUTPUT); // ADC
  digitalWrite(vcc_peripherial_pin, HIGH);
  Serial.begin(115200);
  Wire.begin(sda_pin, slc_pin);
  delay(250);                       // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.setRotation(2);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.setCursor(0, 16);
  display.println("");
  display.println("loading...");
  display.display();

  button_tare.setup(button_tare_pin, INPUT_PULLDOWN, false);
  button_on_off.setup(button_on_off_pin, INPUT_PULLDOWN, false);

  // link the doubleclick function to be called on a doubleclick event.
  button_tare.attachClick(tare_click);
  button_on_off.attachClick(on_off_click);

  setup_wifi();

  long stabilizingtime = 100;
  loadCell.begin();
  loadCell.start(stabilizingtime, true);
  // TODO: calibration factor could be determined, by flashing the code from the example
  loadCell.setCalFactor(2227.69);
}

void drawBatteryStatus(float voltage) {
  auto const battery_level = (voltage - 3.3) / 0.7 * 100;

  // Calculate battery level width in pixels (0-10)
  int battery_width = map(battery_level, 0, 100, 0, 10);

  // Define battery dimensions and position
  auto constexpr y_margin = 1;
  auto constexpr battery_height = 5;
  auto constexpr battery_x = screen_width - 10 - 10;  // 2 pixels margin from the right edge
  auto constexpr battery_y = screen_height - battery_height - y_margin;  // 2 pixels margin from the bottom edge

  // Draw battery outline
  display.drawRect(battery_x, battery_y, 10, battery_height, SH110X_WHITE);

  // Draw battery level
  if (battery_width > 0) {
    display.fillRect(battery_x + 1, battery_y + 1, battery_width, battery_height - 2, SH110X_WHITE);
  }

  // Optionally, clear the area before drawing to avoid overlapping previous drawings
  display.fillRect(battery_x, battery_y, 10, battery_height, SH110X_BLACK);
  display.drawRect(battery_x, battery_y, 10, battery_height, SH110X_WHITE);
  display.fillRect(battery_x + 1, battery_y + 1, battery_width, battery_height - 2, SH110X_WHITE);
}

void loop()
{
  button_tare.tick();
  button_on_off.tick();
  if (loadCell.update())
  {
    display.clearDisplay();
    auto value = loadCell.getData();
    // text display tests
    Serial.println(value);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 16);
    display.println("");
    display.print(value > 0 ? "  " : " ");
    display.println(value);
    display.setTextSize(1);
    auto const voltage = read_battery_voltage();
    display.println("");
    display.print("           "); display.print(voltage); display.print(" V");
    drawBatteryStatus(voltage);
    display.setTextSize(2);
    display.display();
  }
}
