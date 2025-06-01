from flask import Flask, request, jsonify
import os
import requests
import json

app = Flask(__name__)

SENSOR_FOLDER = './sensors'
os.makedirs(SENSOR_FOLDER, exist_ok=True)

ESP32_IP = 'http://192.168.1.196:8000'

@app.route('/sensor/<sensor_id>', methods=['GET'])
def get_sensor_value(sensor_id):
    try:
        response = requests.get(f'{ESP32_IP}/sensor/{sensor_id}')
        return jsonify(response.json()), response.status_code
    except requests.exceptions.RequestException as e:
        return jsonify({"error": f"ESP32 request failed: {str(e)}"}), 500

@app.route('/sensor/<sensor_id>', methods=['POST'])
def create_config(sensor_id):
    config_path = os.path.join(SENSOR_FOLDER, f'sensor_{sensor_id}.json')
    if os.path.exists(config_path):
        return jsonify({"error": "Config file already exists."}), 409

    body_content = request.get_json(force=True, silent=True) or {"default": "config"}
    with open(config_path, 'w') as f:
        json.dump({"sensor_id": sensor_id, "config": body_content}, f, indent=2)

    try:
        response = requests.post(f'{ESP32_IP}/sensor/{sensor_id}', json=body_content)
        return jsonify(response.json()), response.status_code
    except requests.exceptions.RequestException as e:
        return jsonify({"error": f"ESP32 POST failed: {str(e)}"}), 500

@app.route('/sensor/<sensor_id>/<config_file>', methods=['PUT'])
def update_config(sensor_id, config_file):
    config_path = os.path.join(SENSOR_FOLDER, config_file)
    if not os.path.exists(config_path):
        return jsonify({"error": "Config file does not exist; cannot update."}), 406

    body_content = request.get_json(force=True, silent=True) or {"updated": "config"}
    with open(config_path, 'w') as f:
        json.dump({"sensor_id": sensor_id, "updated_config": body_content}, f, indent=2)

    try:
        response = requests.put(f'{ESP32_IP}/sensor/{sensor_id}/{config_file}', json=body_content)
        return jsonify(response.json()), response.status_code
    except requests.exceptions.RequestException as e:
        return jsonify({"error": f"ESP32 PUT failed: {str(e)}"}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
