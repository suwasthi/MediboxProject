#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <vector>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 5
#define LED_1 15
#define LED_2 17
#define PB_CANCEL 25
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12
#define SNOOZE_TIME_MS 60000
#define PB_SNOOZE 18
#define LDRPIN 34
#define SERVOPIN 19
char tempAr[6];

WiFiClient espClient;
PubSubClient mqttClient(espClient);
Servo servoMotor;

#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 19800
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// Adjustable parameters
float theta_offset = 30.0;
float gamma_val = 0.75;
float T_med = 30.0;

unsigned long lastSampleTime = 0;
unsigned long lastSendTime = 0;

std::vector<float> ldrSamples;

int ts = 5;  //in seconds
int tu = 2;  //in minutes

int days;
int hours;
int minutes;
int seconds;
int utc_offset_seconds = 19800;

bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = {1, 2};
int alarm_minutes[] = {0, 1};
bool alarm_triggered[] = {false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1: Set Time Zone", "2: Set Alarm 1", "3: Set Alarm 2", "4: Disable Alarms", "5: Delete Alarm"};

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_SNOOZE, INPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LDRPIN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  servoMotor.attach(SERVOPIN);

  Serial.begin(9600);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 1);
  }

  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 1);
  delay(2000);

  setupMqtt();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();
  print_line("Welcome to Medibox!", 5, 10, 2);
  delay(500);
  display.clearDisplay();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToBroker();
  }

  mqttClient.loop();

  update_time_with_check_alarm();
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }
  checkAndUpdateTemp();
  measureAndControlLight();
  mqttClient.publish("ADMIN-TEMP", tempAr);
  delay(1000);
}

void setupMqtt() {
  mqttClient.setServer("broker.hivemq.com", 1883);
  mqttClient.setCallback(callback);
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(1000, 9999));
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT Broker");
      mqttClient.subscribe("ADMIN-TEMP");
      mqttClient.subscribe("ADMIN-ts");
      mqttClient.subscribe("ADMIN-tu");
      mqttClient.subscribe("MEDIBOX/theta_offset");
      mqttClient.subscribe("MEDIBOX/gamma");
      mqttClient.subscribe("MEDIBOX/Tmed");
    } else {
      Serial.print("Failed, error code: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println("Received topic: " + String(topic));
  Serial.println("Message payload: " + messageTemp);


  if (String(topic) == "ADMIN-ts") {
    ts = messageTemp.toInt();
  } else if (String(topic) == "ADMIN-tu") {
    tu = messageTemp.toInt();
  } else if (String(topic) == "MEDIBOX/theta_offset") {
    theta_offset = messageTemp.toFloat();
  } else if (String(topic) == "MEDIBOX/gamma") {
    gamma_val = messageTemp.toFloat();
  } else if (String(topic) == "MEDIBOX/Tmed") {
    T_med = messageTemp.toFloat();
  }
}

void measureAndControlLight() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSampleTime >= ts * 1000) {
    lastSampleTime = currentMillis;
    float ldrValue = analogRead(LDRPIN) / 4095.0;
    ldrSamples.push_back(ldrValue);

    if (ldrSamples.size() > (tu * 60) / ts) {
      ldrSamples.erase(ldrSamples.begin());
    }

    Serial.println("Sampled LDR: " + String(ldrValue));
  }

  if (currentMillis - lastSendTime >= tu * 60000) {
    lastSendTime = currentMillis;
    if (!ldrSamples.empty()) {
      float sum = 0;
      for (float val : ldrSamples) sum += val;
      float average = sum / ldrSamples.size();

      TempAndHumidity data = dhtSensor.getTempAndHumidity();
      float T = data.temperature;

      float ratio = max(0.01f, (float)ts / (float)(tu * 60)); // Avoid log(0) or negative
      float angle = theta_offset + (180 - theta_offset) * average * gamma_val * log(ratio) * (T / T_med);

      // If angle out of 0-180 range, reset to theta_offset
      if (angle < 0 || angle > 180) {
        angle = theta_offset;
      }

      servoMotor.write(angle);

      mqttClient.publish("ADMIN-LIGHT-AVG", String(average).c_str());
      mqttClient.publish("MEDIBOX/motor_angle", String(angle).c_str());

      Serial.println("Published avg light intensity: " + String(average));
      Serial.println("Motor angle set to: " + String(angle));
    }
  }
}


void checkAndUpdateTemp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  data.temperature = round(data.temperature * 100) / 100.0; // Keep two decimal points

  // Update global temperature array
  String(data.temperature, 2).toCharArray(tempAr, 6);

  bool all_good = true;
  String tempText, humText;

  // Check Temperature
  if (data.temperature > 32) {
    tempText = "Temp HIGH: " + String(data.temperature) + "C";
    all_good = false;
  } else if (data.temperature < 24) {
    tempText = "Temp LOW: " + String(data.temperature) + "C";
    all_good = false;
  } else {
    tempText = "Temperature: " + String(data.temperature) + "C";
  }

  // Check Humidity
  if (data.humidity > 80) {
    humText = "Humidity HIGH: " + String(data.humidity) + "%";
    all_good = false;
  } else if (data.humidity < 65) {
    humText = "Humidity LOW: " + String(data.humidity) + "%";
    all_good = false;
  } else {
    humText = "Humidity: " + String(data.humidity) + "%";
  }

  // Display on screen
  print_line(tempText, 0, 40, 1);
  print_line(humText, 0, 50, 1);

  // LED indicator
  digitalWrite(LED_2, all_good ? LOW : HIGH);
}

void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);

  display.display();
}

void print_time_now() {
  display.clearDisplay();

  // Display Time in HH:MM:SS format
  display.setTextSize(2);  // Larger text for time display
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.printf("%02d:%02d:%02d", hours, minutes, seconds);

  int y_offset = 20;
  for (int i = 0; i < n_alarms; i++) {
    if (alarm_enabled) {
      String alarmText = "Alarm" + String(i + 1) + ": " + String(alarm_hours[i]) + ":" +
                         (alarm_minutes[i] < 10 ? "0" : "") + String(alarm_minutes[i]);
      print_line(alarmText, 10, y_offset, 1);
      y_offset += 10;
    }
  }
}

void update_time() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to get time.");
    return;
  }
  // Extract time components
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
}

void ring_alarm() {
  display.clearDisplay();
  print_line("Medicine Time!", 0, 0, 1);
  digitalWrite(LED_1, HIGH);

  bool break_happened = false;
  bool snooze_happened = false;
  unsigned long snooze_start_time = 0;

  while (!break_happened) {
    update_time();

    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {  // Cancel button pressed
        delay(200);
        break_happened = true;
        break;
      }
      if (digitalRead(PB_SNOOZE) == LOW) {  // Snooze button pressed
        delay(200);
        snooze_happened = true;
        snooze_start_time = millis();  // Record snooze start time
        digitalWrite(BUZZER, LOW);     // Stop buzzer immediately
        digitalWrite(LED_1, LOW);      // Turn off LED during snooze
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }

    if (snooze_happened) {
      display.clearDisplay();
      unsigned long snooze_end_time = snooze_start_time + SNOOZE_TIME_MS;

      while (millis() < snooze_end_time) {
        update_time();
        print_time_now();
        display.display();

        // Check if PB_OK is pressed to go to the menu
        if (digitalRead(PB_OK) == LOW) {
          delay(200);
          go_to_menu();  // Enter menu without breaking snooze
        }

        delay(100);  // Small delay for UI updates
      }

      // Restart alarm after snooze
      snooze_happened = false;
      digitalWrite(LED_1, HIGH); // Turn LED back on
    }
  }
  digitalWrite(LED_1, LOW);
}


void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  if (alarm_enabled) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        if (!alarm_triggered[i]) {
          ring_alarm();
          alarm_triggered[i] = true;
        }
      } else {
        // Reset alarm when the time has passed so it can ring again later
        alarm_triggered[i] = false;
      }
    }
  }
}

int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }
    else if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    update_time();
  }
}

void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 1);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

void run_mode(int mode) {
  if (mode == 0) {
    set_time_zone();
  }

  else if (mode == 1 || mode == 2) {
    set_alarm(mode - 1);
  }
  else if (mode == 3) {
    alarm_enabled = false;
  }
  else if (mode == 4) {
    if (n_alarms > 0) {
      int alarm_to_delete = 0;
      while (true) {
        display.clearDisplay();
        print_line("Delete Alarm: " + String(alarm_to_delete + 1), 0, 0, 2);

        int pressed = wait_for_button_press();

        // Handle the button presses:
        if (pressed == PB_UP) {
          alarm_to_delete = (alarm_to_delete + 1) % n_alarms;
        } else if (pressed == PB_DOWN) {
          alarm_to_delete = (alarm_to_delete - 1 + n_alarms) % n_alarms;
        } else if (pressed == PB_OK) {
          delete_alarm(alarm_to_delete);
          break;
        } else if (pressed == PB_CANCEL) {
          break;
        }
      }
    } else {
      display.clearDisplay();
      print_line("No alarms set", 0, 0, 1);
      delay(1000);
    }
  }
}

void set_time_zone() {
  float temp_offset = utc_offset_seconds / 3600.0; // Convert seconds to hours (including fractions)

  while (true) {
    display.clearDisplay();
    print_line("UTC Offset: " + String(temp_offset, 2), 0, 0, 1);  // Show up to 2 decimal places

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_offset += 0.5;  // Increase offset by 0.5 hours (30 minutes)

    } else if (pressed == PB_DOWN) {
      delay(200);
      temp_offset -= 0.5;  // Decrease offset by 0.5 hours

    } else if (pressed == PB_OK) {
      delay(200);
      utc_offset_seconds = temp_offset * 3600;  // Convert hours to seconds

      // Reconfigure NTP with the new offset
      configTime(utc_offset_seconds, 0, NTP_SERVER);

      //Fetch and update the time immediately
      update_time();
      break;

    } else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time Zone Set", 0, 0, 1);
  delay(1000);
}

void set_alarm(int alarm) {
  alarm_enabled = true;
  int temp_hour = alarm_hours[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 1);
  delay(1000);
}

void delete_alarm(int alarm_index) {
  if (alarm_index >= 0 && alarm_index < n_alarms) {
    for (int i = alarm_index; i < n_alarms - 1; i++) {
      alarm_hours[i] = alarm_hours[i + 1];
      alarm_minutes[i] = alarm_minutes[i + 1];
      alarm_triggered[i] = alarm_triggered[i + 1];
    }
    n_alarms--;  // Reduce the number of alarms
    display.clearDisplay();
    print_line("Alarm Deleted", 0, 0, 1);
    delay(1000);
  }
}






