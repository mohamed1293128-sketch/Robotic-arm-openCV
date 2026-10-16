# Gesture-Controlled Robotic Arm

A robotic arm controlled by hand gestures and hand tracking using Python, OpenCV, MediaPipe, and an ESP32. The Python application tracks hand landmarks, maps hand positions to servo angles, and sends commands to the ESP32 over Wi-Fi using TCP.

## Features

- Hand tracking using MediaPipe Hand Landmarker.
- Hand gesture recognition using MediaPipe Gesture Recognizer.
- Control of the base, shoulder, elbow, and wrist servos using hand positions.
- Gripper control using open-palm and closed-fist gestures.
- Wireless communication between the computer and ESP32 over TCP.
- Servo movement handled by the ESP32 using a PCA9685 PWM driver.
- Configurable camera, model paths, ESP32 IP address, and TCP port.

## Hardware

- ESP32 development board
- Webcam connected to the computer running the Python application
- PCA9685 16-channel PWM servo driver
- Five servos for the robotic arm
- External power supply suitable for the servos

**Important:** Do not power multiple servos directly from the ESP32's 3.3 V pin. Use an appropriate external servo power supply and connect the power supply ground to the ESP32 and PCA9685 ground.

## Software Requirements

- Python 3
- OpenCV
- MediaPipe
- Arduino IDE or another compatible development environment
- Adafruit PWM Servo Driver library

## Project Structure

```text
Robotic-arm-openCV/
├── README.md
├── LICENSE
├── requirements.txt
├── .gitignore
├── config.example.json
├── models/
│   ├── gesture_recognizer.task
│   └── hand_landmarker.task
├── python/
│   └── Robotic_arm.py
└── esp32 frimware/
    ├── robotic_arm_openCV_control.ino
    └── secrets.h.example
```

The firmware directory is currently named `esp32 frimware`. The spelling is retained here to match the existing repository path.

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/mohamed1293128-sketch/Robotic-arm-openCV.git
cd Robotic-arm-openCV
```

### 2. Create a Python virtual environment

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 3. Install Python dependencies

```bash
python -m pip install --upgrade pip
pip install -r requirements.txt
```

### 4. Configure the application

Copy the example configuration:

```bash
cp config.example.json config.json
```

Edit `config.json` to set the ESP32 IP address and check the camera index. The example configuration expects both MediaPipe model files to be in `models/`.

The local `config.json` is ignored by Git so machine-specific settings do not need to be committed.

### 5. Configure and upload the ESP32 firmware

Open `esp32 frimware/robotic_arm_openCV_control.ino` in your Arduino-compatible development environment.

Copy the example Wi-Fi header:

```bash
cp "esp32 frimware/secrets.h.example" "esp32 frimware/secrets.h"
```

Edit `secrets.h` and replace the placeholder Wi-Fi network name and password. Do not commit the real `secrets.h` file.

Install the Adafruit PWM Servo Driver library, select the correct ESP32 board and port, and upload the sketch. Ensure the ESP32 and the computer running Python are connected to the same local network.

## Running the Application

Run the following command from the repository root:

```bash
python python/Robotic_arm.py
```

The application reads `config.json` by default. You can override settings with command-line arguments:

```bash
python python/Robotic_arm.py --ip 192.168.1.9 --port 5000
python python/Robotic_arm.py --help
```

The camera window displays the video feed and hand tracking information. Press `q` to exit.

## Communication Protocol

The Python application sends newline-terminated TCP commands to the ESP32.

| Command | Purpose |
|---|---|
| `BASE:60` | Set the base target angle to 60 degrees |
| `SHOULDER:45` | Set the shoulder target angle to 45 degrees |
| `ELBOW:90` | Set the elbow target angle to 90 degrees |
| `WRIST:70` | Set the wrist target angle to 70 degrees |
| `Open_Palm` | Open the gripper |
| `Closed_Fist` | Close the gripper |

The ESP32 parses incoming commands and updates the target positions of the corresponding servos. The firmware uses PCA9685 channels 0, 1, 2, 4, and 5 for the base, shoulder, elbow, wrist, and gripper respectively.

Servo angles and physical movement ranges depend on the firmware configuration and mechanical assembly. Test the arm carefully before using the full movement range.

## Configuration

The `config.example.json` file contains these settings:

- `esp32_ip`: IP address of the ESP32.
- `port`: TCP server port (default: `5000`).
- `gesture_model`: path to the gesture recognition model.
- `landmarker_model`: path to the hand landmark model.
- `camera`: camera device index (usually `0` for the default webcam).

## Troubleshooting

### Cannot connect to the ESP32

- Confirm the ESP32 is powered on and connected to Wi-Fi.
- Check its current IP address.
- Verify that the firmware is listening on the configured TCP port.
- Confirm that the computer and ESP32 can communicate over the local network.

### Camera cannot be opened

- Check the camera connection.
- Close applications that may already be using the camera.
- Try another camera index in `config.json`.

### MediaPipe model not found

- Confirm both `.task` files exist in `models/`.
- Check that the model paths in `config.json` are correct.

### Servos do not move correctly

- Verify PCA9685 wiring and I2C connections.
- Confirm the servo power supply can provide sufficient current.
- Check the servo channel assignments and pulse-width limits in the firmware.
- Test individual servos at conservative angles before operating the complete arm.

## Safety

Keep hands clear of moving joints during testing. Incorrect angle mappings, wiring errors, or mechanical interference can damage servos or the arm.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
