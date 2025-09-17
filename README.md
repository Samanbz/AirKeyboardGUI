## AirKeyboard GUI

A minimal GUI for data collection for the [AirKeyboard](https://www.github.com/Samanbz/AirKeyboard) project.
Goal of the data collection is to collect hand landmarks (joint positions) while typing on a keyboard and save them in a CSV file with the corresponding key events.

This is done in the following manner:

1. The user starts the GUI and selects a folder to save the data.
2. A text prompt (configurable) is displayed on the screen.
3. The user types the text prompt on a physical keyboard while the webcam records the hand landmarks.
4. The webcam feed is processed using [Google MediaPipe](https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker) to extract hand landmarks in real time.
5. The hand landmarks and the corresponding key events are saved in a CSV file in the selected folder.

### Demo
https://github.com/user-attachments/assets/c668ce5d-ddf4-4f02-b161-5a52670c4fa0

### Example output
https://github.com/user-attachments/assets/8b9e7415-3c79-4b4b-ac67-2d526cb7767a

### The components of the application
<img width="2494" height="1335" alt="diagram" src="https://github.com/user-attachments/assets/1282c1f6-1fe4-4db3-bd51-62b3459ef13c" />

### Requirements

- A Windows machine
- A webcam (for now, only NV12 format is supported))
- WIN32 API
- Python 3.x
  - Google mediapipe
  - Watchdog
  - Numpy
  - Pandas
- CUDA
