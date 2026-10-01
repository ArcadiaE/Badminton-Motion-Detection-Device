import asyncio
import struct
import joblib
import openai
import nest_asyncio
import numpy as np
from flask import Flask, render_template, jsonify
from bleak import BleakClient, BleakScanner
import os
import pandas as pd

nest_asyncio.apply()

openai.api_key = "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"

app = Flask(__name__)

DEVICE_NAME = "TOGENASHI_TOGEARI"
SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214"

CHARACTERISTIC_UUIDS = {
    "roll": "19B10001-E8F2-537E-4F6C-D104768A1214",
    "pitch": "19B10002-E8F2-537E-4F6C-D104768A1214",
    "yaw": "19B10003-E8F2-537E-4F6C-D104768A1214",
    "heartRate": "19B10004-E8F2-537E-4F6C-D104768A1214",
    "spo2": "19B10005-E8F2-537E-4F6C-D104768A1214"
}

model_dir = r"F:\\modelTrain\\model"
svm_model = joblib.load(os.path.join(model_dir, "fix2_svm_model.pkl"))
scaler = joblib.load(os.path.join(model_dir, "fix2_scaler.pkl"))
label_encoder = joblib.load(os.path.join(model_dir, "fix2_label_encoder.pkl"))

ble_client = None
ble_characteristics = {}
sensor_values = {}

def unpack_float(data):
    return struct.unpack('<f', bytearray(data))[0] if len(data) == 4 else np.nan

async def connect_ble_device():
    global ble_client, ble_characteristics
    print("BLE scanning for devices")

    devices = await BleakScanner.discover()
    target = next((d for d in devices if d.name and DEVICE_NAME in d.name), None)
    if not target:
        print(f"BLE device named {DEVICE_NAME} not found")
        raise Exception(f"Device {DEVICE_NAME} not found")

    print(f"BLE found device: {target.name} at {target.address}")
    ble_client = BleakClient(target)

    try:
        await ble_client.connect()
    except Exception as e:
        print(f"BLE connection failed: {e}")
        raise e

    if not ble_client.is_connected:
        print("BLE device connected but not responsive")
        raise Exception("BLE device connection failed")

    print(f"BLE connected to {DEVICE_NAME}")

    for name, uuid in CHARACTERISTIC_UUIDS.items():
        ble_characteristics[name] = ble_client.services.get_characteristic(uuid)
        print(f"BLE characteristic {name} is ready")

async def read_sensor_data():
    if ble_client is None or not ble_client.is_connected:
        print("BLE reconnecting to device")
        await connect_ble_device()

    print("BLE reading sensor data")
    for key, char in ble_characteristics.items():
        data = await ble_client.read_gatt_char(char)
        sensor_values[key] = unpack_float(data)

def preprocess_data(data):
    feature_names = [
        "roll_velocity", "pitch_velocity", "yaw_velocity",
        "roll_acceleration", "pitch_acceleration", "yaw_acceleration",
        "peak_count",
        "roll", "pitch", "yaw",
        "roll_velocity_abs", "pitch_velocity_abs", "yaw_velocity_abs",
        "roll_acceleration_abs", "pitch_acceleration_abs", "yaw_acceleration_abs"
    ]
    raw_features = pd.DataFrame([data], columns=feature_names)
    return scaler.transform(raw_features)

def predict_action(sensor_data):
    processed = preprocess_data(sensor_data)
    label = svm_model.predict(processed)[0]
    return label_encoder.inverse_transform([label])[0]

def generate_feedback(action):
    prompt = f"""
    The user's swing action is {action}
    Please analyze the characteristics of this movement, provide training suggestions, and explain how to improve the technique
    """
    response = openai.ChatCompletion.create(
        model="gpt-4o",
        messages=[
            {"role": "system", "content": "You are a professional badminton coach"},
            {"role": "user", "content": prompt}
        ]
    )
    return response["choices"][0]["message"]["content"]

@app.route('/')
def index():
    return render_template("Mortis.html")

@app.route('/sensor_data')
def get_sensor_data():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(read_sensor_data())

    roll = sensor_values.get("roll", 0)
    pitch = sensor_values.get("pitch", 0)
    yaw = sensor_values.get("yaw", 0)

    dt = 0.01

    roll_velocity = (roll - get_sensor_data.prev_roll) / dt
    pitch_velocity = (pitch - get_sensor_data.prev_pitch) / dt
    yaw_velocity = (yaw - get_sensor_data.prev_yaw) / dt

    roll_acceleration = (roll_velocity - get_sensor_data.prev_roll_v) / dt
    pitch_acceleration = (pitch_velocity - get_sensor_data.prev_pitch_v) / dt
    yaw_acceleration = (yaw_velocity - get_sensor_data.prev_yaw_v) / dt

    get_sensor_data.prev_roll = roll
    get_sensor_data.prev_pitch = pitch
    get_sensor_data.prev_yaw = yaw
    get_sensor_data.prev_roll_v = roll_velocity
    get_sensor_data.prev_pitch_v = pitch_velocity
    get_sensor_data.prev_yaw_v = yaw_velocity

    get_sensor_data.roll_velocity_buffer.append(roll_velocity)
    if len(get_sensor_data.roll_velocity_buffer) > 10:
        get_sensor_data.roll_velocity_buffer.pop(0)

    peak_count = 0
    if len(get_sensor_data.roll_velocity_buffer) >= 3:
        diff_sign = np.sign(np.diff(get_sensor_data.roll_velocity_buffer))
        peak_count = int(np.sum(np.diff(diff_sign) < 0))

    features = {
        "roll_velocity": roll_velocity,
        "pitch_velocity": pitch_velocity,
        "yaw_velocity": yaw_velocity,
        "roll_acceleration": roll_acceleration,
        "pitch_acceleration": pitch_acceleration,
        "yaw_acceleration": yaw_acceleration,
        "peak_count": peak_count,
        "roll": roll,
        "pitch": pitch,
        "yaw": yaw,
        "roll_velocity_abs": abs(roll_velocity),
        "pitch_velocity_abs": abs(pitch_velocity),
        "yaw_velocity_abs": abs(yaw_velocity),
        "roll_acceleration_abs": abs(roll_acceleration),
        "pitch_acceleration_abs": abs(pitch_acceleration),
        "yaw_acceleration_abs": abs(yaw_acceleration)
    }

    action = predict_action(features)
    feedback = generate_feedback(action)

    return jsonify({
        "data": features,
        "predicted_action": action,
        "feedback": feedback
    })

get_sensor_data.prev_roll = 0
get_sensor_data.prev_pitch = 0
get_sensor_data.prev_yaw = 0
get_sensor_data.prev_roll_v = 0
get_sensor_data.prev_pitch_v = 0
get_sensor_data.prev_yaw_v = 0
get_sensor_data.roll_velocity_buffer = []

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)
