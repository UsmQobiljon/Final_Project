/*************************************************************************************************************************************************
 *  TITLE: This sketch takes an image when motion is detected using a PIR sensor module that connected to pin GPIO 13 
 *************************************************************************************************************************************************/

#include <esp_camera.h>
#include <Arduino.h>
#include <FS.h>                // SD Card ESP32
#include <SD_MMC.h>            // SD Card ESP32
#include <soc/soc.h>           // Disable brownour problems
#include <soc/rtc_cntl_reg.h>  // Disable brownour problems
#include <driver/rtc_io.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>            // read and write from flash memory
#include <WiFi.h>
#include <WiFiClientSecure.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 1

RTC_DATA_ATTR int bootCount = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Replace with your network credentials
const char* ssid = "Redmi Note 9 Pro";
const char* password = "12345678";

// Replace with your Telegram BOT token
const char* telegramToken = "7424469368:AAGRVxyM2NLXyRQeVy_6-cHBmTiil4d31DU";

// Replace with your Telegram chat ID
const char* chatID = "562606812";

int pictureNumber = 0;

void sendPhoto(String path) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Connection to Telegram failed");
    return;
  }

  String head = "--randomBoundary\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + String(chatID) + "\r\n--randomBoundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"" + path + "\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--randomBoundary--\r\n";

  File file = SD_MMC.open(path.c_str(), FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  String request = "POST /bot" + String(telegramToken) + "/sendPhoto HTTP/1.1\r\n";
  request += "Host: api.telegram.org\r\n";
  request += "Content-Length: " + String(head.length() + file.size() + tail.length()) + "\r\n";
  request += "Content-Type: multipart/form-data; boundary=randomBoundary\r\n";
  request += "\r\n";
  request += head;

  client.print(request);

  uint8_t buffer[512];
  size_t bytesRead;
  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    client.write(buffer, bytesRead);
  }

  client.print(tail);

  file.close();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  Serial.println("Photo sent to Telegram");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);

  Serial.setDebugOutput(true);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  pinMode(4, INPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_dis(GPIO_NUM_4);

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Set the camera parameters
  sensor_t * s = esp_camera_sensor_get();
  s->set_contrast(s, 2);    //min=-2, max=2
  s->set_brightness(s, 2);  //min=-2, max=2
  s->set_saturation(s, 2);  //min=-2, max=2
  delay(100);               //wait a little for settings to take effect

  Serial.println("Starting SD Card");

  delay(500);
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    //return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }

  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Exiting now");
    while (1);   //wait here as something is not right
  }

  // Initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".jpg";

  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());
  // Create new file
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    Serial.println("Exiting now");
    while (1);   //wait here as something is not right
  } else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Send the photo to Telegram
  sendPhoto(path);

  delay(1000);

  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);  //GPIO for LED flash
  digitalWrite(4, LOW);  //turn OFF flash LED
  rtc_gpio_hold_en(GPIO_NUM_4);  //make sure flash is held LOW in sleep

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0);

  Serial.println("Going to sleep now");
  delay(3000);
  esp_deep_sleep_start();
  Serial.println("Now in Deep Sleep Mode");
}

void loop() {

}
