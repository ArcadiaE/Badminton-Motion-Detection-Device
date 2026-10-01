import asyncio
import struct
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "TOGENASHI_TOGEARI"
SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214"

CHARACTERISTIC_UUIDS = {
    "roll": "19B10001-E8F2-537E-4F6C-D104768A1214",
    "pitch": "19B10002-E8F2-537E-4F6C-D104768A1214",
    "yaw": "19B10003-E8F2-537E-4F6C-D104768A1214",
    "heartRate": "19B10004-E8F2-537E-4F6C-D104768A1214",
    "spo2": "19B10005-E8F2-537E-4F6C-D104768A1214"
}

def unpack_float(data):
    if len(data) == 4:
        return struct.unpack('<f', bytearray(data))[0]
    return None

async def debug_ble_device():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    target = next((d for d in devices if d.name and DEVICE_NAME in d.name), None)

    if not target:
        print(f"Device '{DEVICE_NAME}' not found.")
        return

    print(f"Found device: {target.name} ({target.address})")
    client = BleakClient(target)

    try:
        await client.connect()
        print(f"Connected to {DEVICE_NAME}")
        characteristics = {}
        for key, uuid in CHARACTERISTIC_UUIDS.items():
            characteristics[key] = client.services.get_characteristic(uuid)

        while True:
            print("\n--- Reading BLE Data ---")
            for name, char in characteristics.items():
                try:
                    raw = await client.read_gatt_char(char)
                    val = unpack_float(raw)
                    print(f"{name}: bytes={list(raw)}  float={val}")
                except Exception as e:
                    print(f"{name}: read failed - {e}")
            await asyncio.sleep(0.5)

    except Exception as e:
        print(f"Connection failed: {e}")
    finally:
        if client.is_connected:
            await client.disconnect()
            print("Disconnected from device.")

if __name__ == "__main__":
    asyncio.run(debug_ble_device())
