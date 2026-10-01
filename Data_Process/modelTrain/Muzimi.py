import asyncio
import struct
import joblib
import openai
import nest_asyncio
import numpy as np
from flask import Flask, render_template, jsonify
from bleak import BleakClient, BleakScanner
import os
import random
import pandas as pd


openai.api_key = "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"

model_dir = r"F:\\modelTrain\\model"
svm_model = joblib.load(os.path.join(model_dir, "fix2_svm_model.pkl"))
scaler = joblib.load(os.path.join(model_dir, "fix2_scaler.pkl"))
label_encoder = joblib.load(os.path.join(model_dir, "fix2_label_encoder.pkl"))

app = Flask(__name__)

def generate_fake_sensor_data():
    roll = random.uniform(0.25, 0.90)
    pitch = random.uniform(0.25, 0.65)
    yaw = random.uniform(0.15, 0.90)

    roll_velocity = random.uniform(-20, 15)
    pitch_velocity = random.uniform(-10, 10)
    yaw_velocity = random.uniform(-25, 25)

    roll_acceleration = random.uniform(-2200, 2000)
    pitch_acceleration = random.uniform(-1200, 1400)
    yaw_acceleration = random.uniform(-2800, 3200)

    peak_count = random.randint(0, 90)

    return {
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

def preprocess_data(data):
    feature_order = [
        "roll_velocity", "pitch_velocity", "yaw_velocity",
        "roll_acceleration", "pitch_acceleration", "yaw_acceleration",
        "peak_count",
        "roll", "pitch", "yaw",
        "roll_velocity_abs", "pitch_velocity_abs", "yaw_velocity_abs",
        "roll_acceleration_abs", "pitch_acceleration_abs", "yaw_acceleration_abs"
    ]
    # raw_features = np.array([data[feat] for feat in feature_order]).reshape(1, -1)
    # return scaler.transform(raw_features)
    # raw_features = pd.DataFrame([data[feat] for feat in feature_order], index=[0], columns=feature_order)
    raw_features = pd.DataFrame([[data[feat] for feat in feature_order]], columns=feature_order)
    return scaler.transform(raw_features)

def predict_action(sensor_data):
    processed_data = preprocess_data(sensor_data)
    predicted_label = svm_model.predict(processed_data)[0]
    predicted_action = label_encoder.inverse_transform([predicted_label])[0]
    return predicted_action

def generate_feedback(action):
    prompt = f"""
    The user's swing action is {action}.
    Please analyze the characteristics of this movement, provide training suggestions, and explain how to improve the technique.
    """
    response = openai.ChatCompletion.create(
        model="gpt-4o",
        messages=[{"role": "system", "content": "You are a professional badminton coach."},
                  {"role": "user", "content": prompt}]
    )
    return response["choices"][0]["message"]["content"]

@app.route('/')
def index():
    return render_template("Mortis.html")

@app.route('/sensor_data')
def get_sensor_data():
    fake_data = generate_fake_sensor_data()
    predicted_action = predict_action(fake_data)
    feedback = generate_feedback(predicted_action)

    return jsonify({
        "data": fake_data,
        "predicted_action": predicted_action,
        "feedback": feedback
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
