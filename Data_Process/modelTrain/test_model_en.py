import random
import numpy as np
import joblib
import openai
from flask import Flask, render_template, jsonify

openai.api_key = "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"

svm_model = joblib.load("svm_model.pkl")
scaler = joblib.load("scaler.pkl")
label_encoder = joblib.load("label_encoder.pkl")

# Flask web server
app = Flask(__name__)

# Simulate BLE sensor data
def generate_fake_sensor_data():
    return {
        "roll_velocity": random.uniform(-500, 500),  
        "pitch_velocity": random.uniform(-500, 500),
        "yaw_velocity": random.uniform(-500, 500),
        "roll_acceleration": random.uniform(-20, 20),  
        "pitch_acceleration": random.uniform(-20, 20),
        "yaw_acceleration": random.uniform(-20, 20),
        "peak_count": random.randint(0, 10)  
    }

def preprocess_data(data):
    raw_features = np.array([
        data["roll_velocity"], 
        data["pitch_velocity"], 
        data["yaw_velocity"], 
        data["roll_acceleration"], 
        data["pitch_acceleration"], 
        data["yaw_acceleration"], 
        data["peak_count"]
    ]).reshape(1, -1)

    return scaler.transform(raw_features)

def predict_action(sensor_data):
    processed_data = preprocess_data(sensor_data)
    predicted_label = svm_model.predict(processed_data)[0]
    predicted_action = label_encoder.inverse_transform([predicted_label])[0]
    return predicted_action

def generate_feedback(action):
    """ Generate feedback using ChatGPT """
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
    """ Web homepage """
    return render_template('Its_MyGo!.html')

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
