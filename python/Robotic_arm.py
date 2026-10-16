import argparse
import json
import socket
import sys
from pathlib import Path

import cv2 as cv
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision


DEFAULTS = {
    "esp32_ip": None,
    "port": 5000,
    "gesture_model": "gesture_recognizer.task",
    "landmarker_model": "hand_landmarker.task",
    "camera": 0,
}


def load_settings():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", default="config.json")
    parser.add_argument("--ip")
    parser.add_argument("--port", type=int)
    parser.add_argument("--gesture-model")
    parser.add_argument("--landmarker-model")
    parser.add_argument("--camera", type=int)
    args = parser.parse_args()

    settings = dict(DEFAULTS)
    config_path = Path(args.config)

    if config_path.exists():
        with open(config_path) as f:
            settings.update(json.load(f))
    elif args.config != "config.json":
        sys.exit(f"ERROR: Config file not found: {config_path}")

    overrides = {
        "esp32_ip": args.ip,
        "port": args.port,
        "gesture_model": args.gesture_model,
        "landmarker_model": args.landmarker_model,
        "camera": args.camera,
    }

    settings.update({
        k: v for k, v in overrides.items() if v is not None
    })

    if not settings["esp32_ip"]:
        sys.exit("ERROR: ESP32 IP not set")

    for key in ("gesture_model", "landmarker_model"):
        if not Path(settings[key]).is_file():
            sys.exit(f"ERROR: Model not found: {settings[key]}")

    return settings


def map_angle(value, min_angle=30, max_angle=90):
    value = max(0.0, min(1.0, value))
    return int(max_angle - value * (max_angle - min_angle))


def main():
    cfg = load_settings()

    esp32 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    esp32.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    try:
        esp32.connect((cfg["esp32_ip"], cfg["port"]))
    except OSError as e:
        esp32.close()
        sys.exit(f"ERROR: Could not connect to ESP32: {e}")

    recognizer = None
    landmarker = None
    cam = None

    last_angles = {}
    last_gesture = None

    display_positions = {
        "BASE": 100,
        "SHOULDER": 130,
        "WRIST": 190,
        "ELBOW": 220,
    }

    try:
        recognizer = vision.GestureRecognizer.create_from_options(
            vision.GestureRecognizerOptions(
                base_options=python.BaseOptions(
                    model_asset_path=cfg["gesture_model"]
                ),
                num_hands=2,
            )
        )

        landmarker = vision.HandLandmarker.create_from_options(
            vision.HandLandmarkerOptions(
                base_options=python.BaseOptions(
                    model_asset_path=cfg["landmarker_model"]
                ),
                running_mode=vision.RunningMode.IMAGE,
                num_hands=2,
            )
        )

        cam = cv.VideoCapture(int(cfg["camera"]))

        if not cam.isOpened():
            sys.exit("ERROR: Cannot open webcam")

        while cam.isOpened():
            success, frame = cam.read()

            if not success:
                break

            frame = cv.resize(frame, (640, 360))
            frame = cv.flip(frame, 1)
            rgb = cv.cvtColor(frame, cv.COLOR_BGR2RGB)

            mp_image = mp.Image(
                image_format=mp.ImageFormat.SRGB,
                data=rgb,
            )

            h, w = frame.shape[:2]

            # Recognize gestures using the physical right hand.
            gesture_result = recognizer.recognize(mp_image)

            for gestures, hands in zip(
                gesture_result.gestures,
                gesture_result.handedness,
            ):
                if not gestures or not hands:
                    continue

                # Image is mirrored, so Left corresponds to physical right.
                if hands[0].category_name != "Left":
                    continue

                gesture = gestures[0]
                name = gesture.category_name

                cv.putText(
                    frame,
                    name,
                    (20, 50),
                    cv.FONT_HERSHEY_SIMPLEX,
                    0.8,
                    (0, 0, 0),
                    2
                )

                if name != last_gesture:
                    if name in ("Open_Palm", "Closed_Fist"):
                        esp32.sendall(f"{name}\n".encode())
                        last_gesture = name

            # Track hand positions.
            result = landmarker.detect(mp_image)

            if result.hand_landmarks:
                for i, hand in enumerate(result.hand_landmarks):
                    hand_side = result.handedness[i][0].category_name

                    x = hand[9].x
                    y = hand[9].y

                    cv.circle(
                        frame, 
                        (int(x * w), int(y * h)),
                        8, 
                        (0, 255, 0), 
                        cv.FILLED
                    )

                    commands = {}

                    if hand_side == "Left":
                        commands["BASE"] = map_angle(x)
                        commands["SHOULDER"] = map_angle(y)

                    elif hand_side == "Right":
                        commands["WRIST"] = map_angle(x)
                        commands["ELBOW"] = map_angle(y)

                    for servo, angle in commands.items():
                        previous = last_angles.get(servo)

                        if (
                            previous is None
                            or abs(angle - previous) >= 5
                        ):
                            last_angles[servo] = angle
                            esp32.sendall(
                                f"{servo}:{angle}\n".encode()
                            )

                        cv.putText(
                            frame,
                            f"{servo}: {last_angles[servo]}",
                            (20, display_positions[servo]),
                            cv.FONT_HERSHEY_SIMPLEX,
                            0.7, (0, 255, 0), 2
                        )

            cv.imshow("Frame", frame)

            if cv.waitKey(1) & 0xFF == ord("q"):
                break

    except OSError as e:
        print(f"Communication error: {e}", file=sys.stderr)

    finally:
        if recognizer is not None:
            recognizer.close()

        if landmarker is not None:
            landmarker.close()

        if cam is not None:
            cam.release()

        esp32.close()
        cv.destroyAllWindows()


if __name__ == "__main__":
    main()
