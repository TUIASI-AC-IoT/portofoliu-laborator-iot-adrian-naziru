from flask import Flask, request, jsonify
import random
import os
import json

app = Flask(__name__)

SENSOR_FOLDER = './sensors'
os.makedirs(SENSOR_FOLDER, exist_ok=True)

@app.route('/sensor/<sensor_id>', methods=['GET'])
def get_sensor(sensor_id):
    if sensor_id not in ['1', '2']:
        return jsonify({'error': 'Sensor not found'}), 404
    if sensor_id == '1':
        value = round(random.uniform(20.0, 30.0), 2)
    elif sensor_id == '2':
        value = round(random.uniform(50.0, 60.0), 2)
    return jsonify({'sensor_id': sensor_id, 'value': value})

@app.route('/sensor/<sensor_id>', methods=['POST'])
def post_sensor(sensor_id):
    config_path = os.path.join(SENSOR_FOLDER, f'sensor_{sensor_id}.json')
    if os.path.exists(config_path):
        return jsonify({'error': 'Config file already exists for this sensor.'}), 409
    
    body = request.get_json(force=True, silent=True) or {"default": "config"}
    with open(config_path, 'w') as f:
        json.dump({"sensor_id": sensor_id, "config": body}, f, indent=2)
    
    return jsonify({'message': 'Config created (simulated)'}), 201

@app.route('/sensor/<sensor_id>/<config_file>', methods=['PUT'])
def put_sensor(sensor_id, config_file):
    config_path = os.path.join(SENSOR_FOLDER, config_file)
    if not os.path.exists(config_path):
        return jsonify({'error': 'Config file does not exist; cannot update.'}), 406
    
    body = request.get_json(force=True, silent=True) or {"updated": "config"}
    with open(config_path, 'w') as f:
        json.dump({"sensor_id": sensor_id, "updated_config": body}, f, indent=2)
    
    return jsonify({'message': 'Config updated (simulated)', 'received': body}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
