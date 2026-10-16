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
- ESP32-CAM or a compatible webcam setup
- PCA9685 16-channel PWM servo driver
- Five servos for the robotic arm
- External power supply suitable for the servos

**Important:** Do not power multiple servos directly from the ESP32's 3.3 V pin. Use an appropriate external servo power supply and connect the power supply ground to the ESP32 and PCA9685 ground.

## Software Requirements

- Python 3
- OpenCV
- MediaPipe
- An Arduino-compatible development environment
- Adafruit PWM Servo Driver library for the ESP32 firmware

## Project Structure

```text
Robotic-arm-openCV/
├── README.md
├── requirements.txt
├── .gitignore
├── config.example.json
├── config.json
├── models/
│   └── .gitkeep
├── python/
│   └── main.py
└── esp32/
    ├── robotic_arm.ino
    └── secrets.h
```

`config.json` and `esp32/secrets.h` contain local configuration and must not be committed if they contain private settings or credentials.

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

### 4. Add the MediaPipe models

Place the following model files in the `models/` directory:

- `gesture_recognizer.task`
- `hand_landmarker.task`

The model files are not included in this repository. Obtain them from the official MediaPipe documentation and follow their applicable licenses.

### 5. Configure the application

Copy the example configuration:

```bash
cp config.example.json config.json
```

Edit `config.json` to match your ESP32 IP address, TCP port, model paths, and camera index.

### 6. Configure the ESP32

Open `esp32/robotic_arm.ino` in your Arduino-compatible development environment.

Create `esp32/secrets.h` locally and define your Wi-Fi credentials there. Ensure this file is ignored by Git.

Install the Adafruit PWM Servo Driver library and select the correct ESP32 board and port before uploading the firmware.

Ensure the ESP32 and the computer running Python are connected to the same network.

## Running the Application

From the repository root, run:

```bash
python python/main.py
```

The application reads `config.json` by default.

You can override settings using command-line arguments:

```bash
python python/main.py --ip 192.168.1.9 --port 5000
```

For other available options:

```bash
python python/main.py --help
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

The ESP32 parses incoming commands and updates the target positions of the corresponding servos.

Servo angles and physical movement ranges depend on the firmware configuration and mechanical assembly. Test the arm carefully before using the full movement range.

## Configuration

See `config.example.json` for the available configuration fields.

- `esp32_ip`: IP address of the ESP32.
- `port`: TCP server port.
- `gesture_model`: path to the gesture recognition model.
- `landmarker_model`: path to the hand landmark model.
- `camera`: camera device index.

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

- Confirm both `.task` files exist.
- Check that the model paths in `config.json` are correct.

### Servos do not move correctly

- Verify PCA9685 wiring and I2C connections.
- Confirm the servo power supply can provide sufficient current.
- Check the servo channel assignments and pulse-width limits in the firmware.
- Test individual servos at conservative angles before operating the complete arm.

## Safety

Keep hands clear of moving joints during testing. Incorrect angle mappings, wiring errors, or mechanical interference can damage servos or the arm.

## License

Add a license file if you intend to distribute this project publicly.
