#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <HX711_ADC.h>
#include <Adafruit_SH110X.h>
#include <OneButton.h>
#include <driver/rtc_io.h>
#include <WiFiManager.h>
#include <RunningMedian.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <unordered_map>

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
auto constexpr button_zero_pin = 5;
auto constexpr button_on_off_pin = 6;
auto constexpr sda_pin = 7;
auto constexpr slc_pin = 44;
auto constexpr voltage_reading_pin = 1;
HX711_ADC loadCell{hx711_dout_pin, hx711_clk_pin};
OneButton button_zero{};
OneButton button_on_off{};
float last_weight_ = 0;
float last_speed_ = 0;
auto constexpr y_coordinate = 0;
auto constexpr window_ = 10U;
auto constexpr window_average_size_ = 5U;
auto voltage_ = RunningMedian{window_};
auto weight_ = RunningMedian{window_};
auto speed_ = RunningMedian{window_};
unsigned long last_print_time_ = millis();
unsigned long last_speed_print_time_ = millis();
float setting_weight_ = 0;
float setting_delay_ = 0;
auto constexpr delay_key_ = "delay";
auto constexpr weight_key_ = "weight";
auto constexpr settings_key_ = "scale";
auto already_measured_ = false;

enum class Page
{
  Cell,
  SetupWeight,
  SetupDelay
};

Page active_page_ = Page::Cell;

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
  voltage_.add(analogReadMilliVolts(voltage_reading_pin));
  auto const value = voltage_.getMedianAverage(window_average_size_);
  float voltage = 2 * value / 1000.0; // attenuation ratio 1/2, mV --> V
  return voltage;
}

enum class Command
{
  Toggle,
  On,
  Off
};

auto const commands_map_ = std::unordered_map<Command, String>{{Command::Off, "Off"}, {Command::On, "On"}, {Command::Toggle, "Toggle"}};

bool send_command(Command cmd)
{
  auto constexpr ip = "192.168.88.147";
  // http://192.168.88.165/cm?cmnd=Power%20TOGGLE
  // http://192.168.88.165/cm?cmnd=Power%20On
  // http://192.168.88.165/cm?cmnd=Power%20Off

  HTTPClient client{};
  String url = String("http://") + ip + "/cm?user=admin&password=password&cmnd=Power%20" + commands_map_.at(cmd);
  client.begin(url);
  auto const code = client.GET();
  String response = client.getString();
  Serial.println("Response: " + response);
  // handle the code
  Serial.println("code " + String{code});
  client.end();
  return 200 == code;
}

void zero_click()
{
  switch (active_page_)
  {
  case Page::Cell:
  {
    Serial.println("tare");
    display.clearDisplay();
    display.setCursor(0, y_coordinate);
    display.println("");
    display.println("taring...");
    display.display();
    loadCell.tare();
    break;
  }
  case Page::SetupWeight:
  {
    setting_weight_ -= 0.1;
    break;
  }
  case Page::SetupDelay:
  {
    setting_delay_ -= 0.1;
    break;
  }
  }
}

void zero_long_press()
{
  switch (active_page_)
  {
  case Page::Cell:
    active_page_ = Page::SetupWeight;
    break;
  case Page::SetupWeight:
    active_page_ = Page::SetupDelay;
    break;
  case Page::SetupDelay:
    active_page_ = Page::Cell;
    break;
  }
}

void on_off_long_press()
{
  switch (active_page_)
  {
  case Page::Cell:
  {
    {
      // no need, as the peripherial is connected via the vcc pin and it's high only
      // active time, not when deep sleep
      loadCell.powerDown();
      display.oled_command(SH110X_DISPLAYOFF);
      // auto const hx711_gpio_numner = static_cast<gpio_num_t>(hx711_clk_pin);
      // rtc_gpio_pulldown_dis(hx711_gpio_numner);
      // rtc_gpio_pullup_en(hx711_gpio_numner);
      auto const gpio_wake_button_number = static_cast<gpio_num_t>(button_on_off_pin);
      esp_sleep_enable_ext0_wakeup(gpio_wake_button_number, HIGH);
      rtc_gpio_pullup_dis(gpio_wake_button_number);
      rtc_gpio_pulldown_en(gpio_wake_button_number);
      digitalWrite(vcc_peripherial_pin, LOW);

      WiFiManager wifi_manager;
      wifi_manager.disconnect();
      delay(1000);
      Serial.println("Sleep");
      esp_deep_sleep_start();
      break;
    }
  }
  case Page::SetupWeight:
  {
    Preferences preferences{};
    preferences.begin(settings_key_, /*readonly=*/false);
    preferences.putFloat(weight_key_, setting_weight_);
    preferences.end();

    display.clearDisplay();
    display.setCursor(0, y_coordinate);
    display.println("");
    display.println("weight");
    display.println("updated");
    display.display();
    delay(1000);

    break;
  }
  case Page::SetupDelay:
  {
    Preferences preferences{};
    preferences.begin(settings_key_, /*readonly=*/false);
    preferences.putFloat(delay_key_, setting_delay_);
    preferences.end();

    display.clearDisplay();
    display.setCursor(0, y_coordinate);
    display.println("");
    display.println("delay");
    display.println("updated");
    display.display();
    delay(1000);

    break;
  }
  }
}

void on_off_click()
{
  switch (active_page_)
  {
  case Page::Cell:
  {
    send_command(Command::Toggle);
    already_measured_ = false;
    break;
  }
  case Page::SetupWeight:
  {
    setting_weight_ += 0.1;
    break;
  }
  case Page::SetupDelay:
  {
    setting_delay_ += 0.1;
    break;
  }
  }
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
    // WiFi.mode(WIFI_STA);
    esp_sleep_disable_wifi_wakeup();
    // WiFi.setSleep(wifi_ps_type_t::WIFI_PS_NONE);

    display.clearDisplay();
    display.setCursor(0, y_coordinate);
    display.println("");
    display.println("setting up");
    display.print("  wifi...");
    display.display();

    wifi_manager.setRestorePersistent(true);
    wifi_manager.setTimeout(60);
    wifi_manager.setWiFiAutoReconnect(true);
    wifi_manager.autoConnect("fellow-scale", "password");
  }
}

void setup()
{
  pinMode(voltage_reading_pin, INPUT);  // ADC
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
  display.setCursor(0, y_coordinate);
  display.println("");
  display.println("loading...");
  display.display();

  button_zero.setup(button_zero_pin, INPUT_PULLDOWN, false);
  button_on_off.setup(button_on_off_pin, INPUT_PULLDOWN, false);

  // link the doubleclick function to be called on a doubleclick event.
  button_zero.attachClick(zero_click);
  button_zero.attachLongPressStart(zero_long_press);
  button_on_off.attachClick(on_off_click);
  button_on_off.attachLongPressStart(on_off_long_press);

  setup_wifi();

  auto constexpr stabilizingtime = 1000;
  loadCell.begin();
  loadCell.start(stabilizingtime, /*doTare=*/true);
  // TODO: calibration factor could be determined, by flashing the code from the example
  loadCell.setCalFactor(2227.69);
  Preferences preferences{};
  preferences.begin(settings_key_, /*readonly=*/true);
  setting_delay_ = preferences.getFloat(delay_key_, 0);
  setting_weight_ = preferences.getFloat(weight_key_, 0);
  preferences.end();
}

void drawBatteryStatus(float voltage)
{
  auto const battery_level = (voltage - 3.3) / 0.7 * 100;

  // Calculate battery level width in pixels (0-10)
  int battery_width = map(battery_level, 0, 100, 0, 10);

  // Define battery dimensions and position
  auto constexpr y_margin = 2;
  auto constexpr battery_height = 5;
  auto constexpr battery_x = screen_width - 10 - 10;
  auto constexpr battery_y = screen_height - battery_height - y_margin; // y_margin pixels margin from the bottom edge

  // Draw battery outline
  display.drawRect(battery_x, battery_y, 10, battery_height, SH110X_WHITE);

  // Draw battery level
  if (battery_width > 0)
  {
    display.fillRect(battery_x + 1, battery_y + 1, battery_width, battery_height - 2, SH110X_WHITE);
  }

  // Optionally, clear the area before drawing to avoid overlapping previous drawings
  display.fillRect(battery_x, battery_y, 10, battery_height, SH110X_BLACK);
  display.drawRect(battery_x, battery_y, 10, battery_height, SH110X_WHITE);
  display.fillRect(battery_x + 1, battery_y + 1, battery_width, battery_height - 2, SH110X_WHITE);
}

void draw_wifi_status()
{
  if (WiFi.isConnected())
  {
    const unsigned char wifiicon[] = {// wifi icon
                                      0x00, 0xff, 0x00, 0x7e, 0x00, 0x18, 0x00, 0x00};
    auto constexpr x_position = screen_width - 9 - 10;
    auto constexpr y_position = screen_height - 10 - 10;
    display.drawBitmap(x_position, y_position, wifiicon, 8, 8, SH110X_WHITE);
  }
  else
  {
    auto const status = WiFi.status();
    Serial.println("Not connected to wifi: " + String{status});
  }
}

String float_to_string(float value)
{
  auto string = String(value); // 2 decimals
  auto const position = string.lastIndexOf('.');
  auto const number_of_additional_symbols = 4 - position;
  auto result = String{};
  for (int i = 0; i < number_of_additional_symbols; ++i)
  {
    result += " ";
  }
  return result + string;
}

void draw_scale()
{
  auto const now = millis();
  if (loadCell.update())
  {
    weight_.add(loadCell.getData());
    if (now - last_print_time_ > 100 /*ms*/)
    {
      display.clearDisplay();
      auto const value = weight_.getMedianAverage(window_average_size_);
      // text display tests
      Serial.println(value);
      display.setTextSize(2);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(0, y_coordinate);
      display.println("");
      display.println(float_to_string(value));
      // draw the speed
      {
        display.setTextSize(1);
        speed_.add((value - last_weight_) * 1000 /*ms*/ / (now - last_print_time_));
        auto const speed = speed_.getMedianAverage(window_average_size_);
        last_weight_ = value;
        if (now - last_speed_print_time_ > 500)
        {
          last_speed_ = speed;
          last_speed_print_time_ = now;
        }
        display.print(float_to_string(last_speed_));
        display.println(" g/s");
      }
      display.setTextSize(1);
      auto const voltage = read_battery_voltage();
      display.println("");
      display.print("           ");
      display.print(voltage);
      display.print(" V");
      drawBatteryStatus(voltage);
      draw_wifi_status();
      display.setTextSize(2);
      display.display();
      last_print_time_ = now;
      weight_.clear();
    }
  }
}

void draw_setup_weight()
{
  display.clearDisplay();
  display.setCursor(0, y_coordinate);
  display.println("");
  display.println("weight");
  display.println(float_to_string(setting_weight_));
  display.display();
}

void draw_setup_delay()
{
  display.clearDisplay();
  display.setCursor(0, y_coordinate);
  display.println("");
  display.println("delay");
  display.println(float_to_string(setting_delay_));
  display.display();
}

void check_weight()
{
  if (!already_measured_)
  {
    auto const weight = weight_.getMedianAverage(window_average_size_);
    if (weight > setting_weight_)
    {
      auto const success = send_command(Command::Off);
      already_measured_ = success;
    }
  }
}

void loop()
{
  button_zero.tick();
  button_on_off.tick();
  switch (active_page_)
  {
  case Page::Cell:
    draw_scale();
    check_weight();
    return;
  case Page::SetupDelay:
    draw_setup_delay();
    return;
  case Page::SetupWeight:
    draw_setup_weight();
    return;
  }
}
