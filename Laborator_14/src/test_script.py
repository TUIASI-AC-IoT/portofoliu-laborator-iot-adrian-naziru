import requests

FLASK_SERVER = 'http://localhost:5000'

passed = 0
failed = 0

def check(test_name, condition):
    global passed, failed
    if condition:
        print(f"[PASS] {test_name}")
        passed += 1
    else:
        print(f"[FAIL] {test_name}")
        failed += 1

def test_get_sensor(sensor_id):
    print(f"\nGET /sensor/{sensor_id}")
    r = requests.get(f"{FLASK_SERVER}/sensor/{sensor_id}")
    expected_status = 200
    expected_keys = ['sensor_id', 'value']
    check(f"GET sensor {sensor_id} status", r.status_code == expected_status)
    check(f"GET sensor {sensor_id} keys", all(k in r.json() for k in expected_keys))

def test_post_config(sensor_id, expect_success):
    print(f"\nPOST /sensor/{sensor_id}")
    data = {"scale": "metric"}
    r = requests.post(f"{FLASK_SERVER}/sensor/{sensor_id}", json=data)
    if expect_success:
        check(f"POST sensor {sensor_id} first time", r.status_code in [200, 201])
    else:
        check(f"POST sensor {sensor_id} conflict", r.status_code == 409)

def test_put_config(sensor_id, config_file, expect_success):
    print(f"\nPUT /sensor/{sensor_id}/{config_file}")
    data = {"scale": "metric"}
    r = requests.put(f"{FLASK_SERVER}/sensor/{sensor_id}/{config_file}", json=data)
    if expect_success:
        check(f"PUT sensor {sensor_id} update config", r.status_code in [200, 201])
    else:
        check(f"PUT sensor {sensor_id} missing config", r.status_code in [406, 404])

def main():
    # 1️⃣ Test GET sensor values
    test_get_sensor('1')
    test_get_sensor('2')

    # 2️⃣ Test POST create config (should succeed)
    test_post_config('1', expect_success=True)
    test_post_config('2', expect_success=True)

    # 3️⃣ Test POST recreate config (should fail with 409)
    test_post_config('1', expect_success=False)

    # 4️⃣ Test PUT update config (should succeed)
    test_put_config('1', 'sensor_1.json', expect_success=True)
    test_put_config('2', 'sensor_2.json', expect_success=True)

    # 5️⃣ Test PUT on non-existing config (should fail)
    test_put_config('3', 'sensor_3.json', expect_success=False)

    # Final score
    total = passed + failed
    print(f"\n Tests passed: {passed}/{total}")
    print(f" Tests failed: {failed}/{total}")

if __name__ == "__main__":
    main()
