import asyncio
import struct
import csv
import datetime
from bleak import BleakClient, BleakScanner
import nest_asyncio
from flask import Flask, render_template_string

nest_asyncio.apply()

app = Flask(__name__)

DEVICE_NAME = "TOGENASHI_TOGEARI"

SENSOR_SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214"
ROLL_CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"
PITCH_CHARACTERISTIC_UUID = "19B10002-E8F2-537E-4F6C-D104768A1214"
YAW_CHARACTERISTIC_UUID = "19B10003-E8F2-537E-4F6C-D104768A1214"
HEART_RATE_CHARACTERISTIC_UUID = "19B10004-E8F2-537E-4F6C-D104768A1214"
SPO2_CHARACTERISTIC_UUID = "19B10005-E8F2-537E-4F6C-D104768A1214"

roll_value = 0.0
pitch_value = 0.0
yaw_value = 0.0
heart_rate_value = 0.0
spo2_value = 0.0

running_event = asyncio.Event()

# CSV
csv_file_name = f"sensor_data_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
with open(csv_file_name, mode='w', newline='') as csv_file:
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(["Timestamp", "Roll", "Pitch", "Yaw"])

def update_csv():
    with open(csv_file_name, mode='a', newline='') as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow([datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'), roll_value, pitch_value, yaw_value])

@app.route('/')
def index():
    return render_template_string('''
    <!doctype html>
    <html lang="en">
      <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
        <title>Gailiuzi Band Beijing</title>
      </head>
      <body>
        <h2>Sensor Data Monitoring</h2>
        <p id="roll">Roll: {{ roll_value }}</p>
        <p id="pitch">Pitch: {{ pitch_value }}</p>
        <p id="yaw">Yaw: {{ yaw_value }}</p>
        <hr>
        <p id="heart_rate">Heart Rate: {{ heart_rate_value }}</p>
        <p id="spo2">SpO2: {{ spo2_value }}</p>
        <hr>
        <button onclick="window.location.href='/exit'">Exit</button>
      </body>
    </html>
    ''', roll_value=roll_value, pitch_value=pitch_value, yaw_value=yaw_value, heart_rate_value=heart_rate_value, spo2_value=spo2_value)

@app.route('/exit')
def exit_app():
    running_event.set()
    return "<p>Stopping...</p>"

async def run_ble_client():
    print("Scanning for devices...")

    devices = await BleakScanner.discover()
    imu_device = None
    for device in devices:
        print(f"Found device: {device.name}, Address: {device.address}")
        if device.name and DEVICE_NAME in device.name:
            imu_device = device
            break

    if not imu_device:
        print(f"Device {DEVICE_NAME} not found.")
        return

    print(f"Found device: {DEVICE_NAME}")

    async def notification_handler(characteristic_uuid, data):
        global roll_value, pitch_value, yaw_value, heart_rate_value, spo2_value
        try:
            if characteristic_uuid == ROLL_CHARACTERISTIC_UUID and len(data) == 4:
                roll_value = struct.unpack('<f', data)[0]
            elif characteristic_uuid == PITCH_CHARACTERISTIC_UUID and len(data) == 4:
                pitch_value = struct.unpack('<f', data)[0]
            elif characteristic_uuid == YAW_CHARACTERISTIC_UUID and len(data) == 4:
                yaw_value = struct.unpack('<f', data)[0]
            elif characteristic_uuid == HEART_RATE_CHARACTERISTIC_UUID and len(data) == 4:
                heart_rate_value = struct.unpack('<f', data)[0]
            elif characteristic_uuid == SPO2_CHARACTERISTIC_UUID and len(data) == 4:
                spo2_value = struct.unpack('<f', data)[0]
            update_csv()
        except Exception as e:
            print(f"Error processing notification: {e}")

    try:
        async with BleakClient(imu_device) as client:
            print(f"Connecting to {DEVICE_NAME}...")
            await client.connect()
            if not client.is_connected:
                print(f"Failed to connect to {DEVICE_NAME}")
                return
            print(f"Connected to {DEVICE_NAME}")

            await client.start_notify(ROLL_CHARACTERISTIC_UUID, lambda sender, data: notification_handler(ROLL_CHARACTERISTIC_UUID, data))
            await client.start_notify(PITCH_CHARACTERISTIC_UUID, lambda sender, data: notification_handler(PITCH_CHARACTERISTIC_UUID, data))
            await client.start_notify(YAW_CHARACTERISTIC_UUID, lambda sender, data: notification_handler(YAW_CHARACTERISTIC_UUID, data))
            await client.start_notify(HEART_RATE_CHARACTERISTIC_UUID, lambda sender, data: notification_handler(HEART_RATE_CHARACTERISTIC_UUID, data))
            await client.start_notify(SPO2_CHARACTERISTIC_UUID, lambda sender, data: notification_handler(SPO2_CHARACTERISTIC_UUID, data))

            await running_event.wait()

            await client.stop_notify(ROLL_CHARACTERISTIC_UUID)
            await client.stop_notify(PITCH_CHARACTERISTIC_UUID)
            await client.stop_notify(YAW_CHARACTERISTIC_UUID)
            await client.stop_notify(HEART_RATE_CHARACTERISTIC_UUID)
            await client.stop_notify(SPO2_CHARACTERISTIC_UUID)

    except Exception as e:
        print(f"Failed to connect or communicate with the device: {e}")

    print("Disconnected.")

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    loop.create_task(run_ble_client())
    app.run(host='0.0.0.0', port=5000, debug=False)
