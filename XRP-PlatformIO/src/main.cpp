#include <Arduino.h>
#include <RPAsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <OneButton.h>

#if __has_include(<secrets.h>)
#   include <secrets.h>
#else
#   error "Create a secrets.h file following secrets.h.template."
#endif

OneButton userbutton;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

const std::vector<std::string> whitelist = {};

void arcadeDrive(double power, double turn) {
  double leftPower = power + turn;
  double rightPower = - power + turn;

  if (abs(power + turn) > 1.0) {
    leftPower = leftPower / (abs(power + turn));
    rightPower = rightPower / (abs(power + turn));
  }

  analogWrite(MOTOR_L_IN_1, leftPower > 0 ? leftPower * 255 : 0);
  analogWrite(MOTOR_L_IN_2, leftPower < 0 ? -leftPower * 255 : 0);
  analogWrite(MOTOR_R_IN_1, rightPower > 0 ? rightPower * 255 : 0);
  analogWrite(MOTOR_R_IN_2, rightPower < 0 ? -rightPower * 255 : 0);
}

void setServo(int servo) {
  if (servo < 0 || servo > 255) {return;}
  analogWrite(SERVO_1, servo);
}

void executeJSONCommand(const char *jsonString) {
  JsonDocument cmd;
  DeserializationError error = deserializeJson(cmd, jsonString);
  if (error) {
    Serial.print("Error deserializing JSON: ");
    Serial.println(error.c_str());
    return;
  }

  double power = cmd["power"] | 0.0;
  double turn = cmd["turn"] | 0.0;
  int servo = cmd["servo"] | 0;

  Serial.print(">P:");
  Serial.println(power);
  Serial.print(">T:");
  Serial.println(turn);
  Serial.print(">S:");
  Serial.println(servo);

  arcadeDrive(power, turn);
  setServo(servo);
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if(type == WS_EVT_CONNECT) {

    // if (std::find(whitelist.begin(), whitelist.end(), client->remoteIP().toString().c_str()) == whitelist.end()) {
    //   Serial.printf("WebSocket client #%u connection from %s rejected (not on whitelist)\n", client->id(), client->remoteIP().toString().c_str());
    //   client->close();
    //   return;
    // }

    if (server->getClients().size() > 1) {
      Serial.printf("WebSocket client #%u connection from %s rejected (too many clients)\n", client->id(), client->remoteIP().toString().c_str());
      client->close();
      return;
    }

    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    client->text("Hello from XRP!");

  } else if(type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    arcadeDrive(0, 0);
    setServo(0);
  } else if(type == WS_EVT_DATA){
    executeJSONCommand((const char *)data);
    digitalWrite(BOARD_LED, HIGH);
    delay(20);
    digitalWrite(BOARD_LED, LOW);
  }
}

void onUserButton() {
  Serial.println("MAC Address: " + WiFi.macAddress());
  Serial.println("IP Address: " + WiFi.localIP().toString());
  Serial.printf("SSID: %s\n", ssid);
}

void setup(){                                                                                                                                                                                                                                                   

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("CONNECTED!!!");

  analogWriteFreq(100);
  analogWriteRange(255);

  pinMode(MOTOR_L_IN_1, OUTPUT);
  pinMode(MOTOR_L_IN_2, OUTPUT);
  pinMode(MOTOR_R_IN_1, OUTPUT);
  pinMode(MOTOR_R_IN_2, OUTPUT);
  pinMode(SERVO_1, OUTPUT);

  userbutton.setup(BOARD_USER_BUTTON, INPUT, true);
  userbutton.attachClick(onUserButton);

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.begin();
}

void loop(){

  userbutton.tick();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Reconnected to WiFi!");
  }
  delay(20);

}