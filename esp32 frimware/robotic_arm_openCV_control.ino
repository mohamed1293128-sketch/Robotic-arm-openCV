#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "secrets.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WiFiServer server(5000);

#define SDA_PIN 21
#define SCL_PIN 22

#define SERVO_MIN 110
#define SERVO_MAX 500

#define BASE_CH     0
#define SHOULDER_CH 1
#define ELBOW_CH    2
#define WRIST_CH    4
#define GRIPPER_CH  5

#define MOVE_INTERVAL 20
#define COMMAND_BUFFER_SIZE 64

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

const uint8_t servo_channels[] = {
  BASE_CH,
  SHOULDER_CH,
  ELBOW_CH,
  WRIST_CH,
  GRIPPER_CH
};

const size_t SERVO_COUNT =
    sizeof(servo_channels) / sizeof(servo_channels[0]);

int servo_angle[16];
int servo_target[16];

unsigned long last_servo_move = 0;

char command_buffer[COMMAND_BUFFER_SIZE];
size_t command_length = 0;
bool dropping_long_command = false;


int angleToPulse(int angle) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}


void moveServo(int channel, int angle){
  angle = constrain(angle, 0, 180);
  pwm.setPWM(channel, 0, angleToPulse(angle));
}


void setTarget(int channel, int angle){
  servo_target[channel] = constrain(angle, 0, 180);
}


// Accept only known servo channels and valid integer angles.
void processCommand(char* command){
  // Remove leading whitespace.
  while(isspace((unsigned char)*command)){
    command++;
  }

  // Remove trailing whitespace.
  size_t length = strlen(command);

  while(length > 0 && isspace((unsigned char)command[length - 1])){
    command[--length] = '\0';
  }

  if(length == 0){
    return;
  }

  // Gripper gestures.
  if(strcmp(command, "Open_Palm") == 0){
    setTarget(GRIPPER_CH, 90);
    return;
  }

  if(strcmp(command, "Closed_Fist") == 0){
    setTarget(GRIPPER_CH, 0);
    return;
  }

  // Position commands use the format SERVO:ANGLE.
  char* separator = strchr(command, ':');

  if(separator == nullptr){
    return;
  }

  // Reject commands containing multiple separators.
  if(strchr(separator + 1, ':') != nullptr){
    return;
  }

  *separator = '\0';

  const char* servo = command;
  const char* angle_text = separator + 1;

  if(*servo == '\0' || *angle_text == '\0'){
    return;
  }

  // Parse the entire numeric field, not just its initial digits.
  char* end_pointer = nullptr;
  long angle = strtol(angle_text, &end_pointer, 10);

  if(end_pointer == angle_text || *end_pointer != '\0'){
    return;
  }

  if(angle < 0 || angle > 180){
    return;
  }

  int channel = -1;

  if(strcmp(servo, "BASE") == 0){
    channel = BASE_CH;
  }
  else if(strcmp(servo, "SHOULDER") == 0){
    channel = SHOULDER_CH;
  }
  else if(strcmp(servo, "ELBOW") == 0){
    channel = ELBOW_CH;
  }
  else if(strcmp(servo, "WRIST") == 0){
    channel = WRIST_CH;
  }
  else{
    return;
  }

  setTarget(channel, (int)angle);
}


// Read complete newline-terminated commands without blocking.
void readClientCommands(WiFiClient& client){
  while(client.available() > 0){
    int incoming = client.read();

    if(incoming < 0){
      break;
    }

    char c = (char)incoming;

    if(c == '\n'){
      if(!dropping_long_command){
        command_buffer[command_length] = '\0';
        processCommand(command_buffer);
      }

      command_length = 0;
      dropping_long_command = false;
      continue;
    }

    if(dropping_long_command){
      continue;
    }

    if(command_length < COMMAND_BUFFER_SIZE - 1){
      command_buffer[command_length++] = c;
    }
    else{
      // Discard an oversized command until its newline.
      command_length = 0;
      dropping_long_command = true;
    }
  }
}


// Move only the five configured servos.
void updateServos() {
  unsigned long now = millis();

  if(now - last_servo_move < MOVE_INTERVAL){
    return;
  }

  last_servo_move = now;

  for(size_t i = 0; i < SERVO_COUNT; i++){
    int channel = servo_channels[i];

    if(servo_angle[channel] < servo_target[channel]){
      servo_angle[channel]++;
      moveServo(channel, servo_angle[channel]);
    }
    else if(servo_angle[channel] > servo_target[channel]){
      servo_angle[channel]--;
      moveServo(channel, servo_angle[channel]);
    }
  }
}


void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  // Initialize software state for all array entries.
  for(int i = 0; i < 16; i++){
    servo_angle[i] = 90;
    servo_target[i] = 90;
  }

  // Initial positions for configured servos.
  servo_angle[BASE_CH] = servo_target[BASE_CH] = 0;
  servo_angle[SHOULDER_CH] = servo_target[SHOULDER_CH] = 90;
  servo_angle[ELBOW_CH] = servo_target[ELBOW_CH] = 90;
  servo_angle[WRIST_CH] = servo_target[WRIST_CH] = 90;
  servo_angle[GRIPPER_CH] = servo_target[GRIPPER_CH] = 50;

  // Apply initial positions.
  for(size_t i = 0; i < SERVO_COUNT; i++){
    int channel = servo_channels[i];
    moveServo(channel, servo_angle[channel]);
  }

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  digitalWrite(2, HIGH);

  server.begin();
}


void loop(){
  WiFiClient client = server.available();

  if(client){
    Serial.println("Python connected!");

    command_length = 0;
    dropping_long_command = false;

    while(client.connected()){
      readClientCommands(client);
      updateServos();
    }

    client.stop();

    command_length = 0;
    dropping_long_command = false;

    Serial.println("Python disconnected");
  }
}
