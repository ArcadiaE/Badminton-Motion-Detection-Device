# Wearable Device for Badminton Motion Detection

A wrist-worn device that recognises badminton strokes in real time and replies with coaching-style feedback. Built as my Part III individual project (undergraduate dissertation) at the University of Southampton, 2024–25. The full dissertation is in [`docs/Final_Report.pdf`](docs/Final_Report.pdf).

The device streams motion and physiological data over Bluetooth Low Energy. A support vector machine classifies four stroke types — forehand, backhand, slice and smash — at **77.37%** test accuracy (macro-averaged F1 = 0.76), up from 53.91% before the feature set was expanded from 10 to 16 dimensions. Given the single-IMU setup, four-class random guessing sits at 25%. Recognised strokes are passed to the ChatGPT API (GPT-4o), which generates personalised coaching feedback shown in a local Flask web app.

## Hardware

The electronics are packaged as a wristwatch with a 3D-printed case.

- **Microcontroller:** Arduino Nano 33 BLE Sense Rev2
- **IMU:** BMI270 + BMM150 (on-board, nine-axis)
- **Biosensor:** MAX30102 heart-rate and SpO2 module
- **Power:** 3.7 V 350 mAh LiPo battery, TP4056-based charging/management
- **Enclosure:** 3D-printed ([`3D_Model/3D.stl`](3D_Model/3D.stl), [`Enclosure/c1.stl`](Enclosure/c1.stl)); PCB layout in [`PCB/yuanlitu.brd`](PCB/yuanlitu.brd)

## Repository structure

- `Hardware/` — final embedded firmware (Arduino/C++): sensor initialisation, continuous sampling, filtering, differential and peak extraction, BLE broadcasting.
- `Data_Process/Matlab/` — MATLAB scripts for BLE data collection and CSV storage, plus the recorded sessions.
- `Data_Process/modelTrain/` — Jupyter-based SVM training, the trained models (`model/*.pkl`), the labelled stroke dataset (`BM_datas/`), and the Python inference script with the Flask feedback app (`templates/`).
- `Data_Process/python/` — earlier Python BLE capture prototype.
- `docs/` — the final dissertation.

## Dependencies

- **Arduino:** [ArduinoBLE](https://github.com/arduino-libraries/ArduinoBLE), [Arduino_BMI270_BMM150](https://github.com/arduino-libraries/Arduino_BMI270_BMM150), [SparkFun MAX3010x Sensor Library](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library), Madgwick.
- **MATLAB:** BLE toolbox (data collection).
- **Python 3.x:**
  ```bash
  pip install pandas scikit-learn joblib bleak flask openai
  ```

## Workflow

1. **Data acquisition** — the IMU streams tri-axial motion data over BLE; MATLAB scripts record sessions to CSV.
2. **Training** — the SVM is trained on the labelled dataset in a Jupyter notebook; the model, scaler and label encoder are exported with joblib.
3. **Inference** — a Python script receives live data and classifies the stroke with the trained model.
4. **Feedback** — the recognised stroke is sent to the ChatGPT API, and the generated coaching feedback is served in a Flask web interface.

## Author

**Yike Zhang**
School of Electronics and Computer Science, University of Southampton
