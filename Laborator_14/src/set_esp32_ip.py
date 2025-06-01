import socket

# Detect local LAN IP
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Doesn’t have to be reachable; just used for interface detection
        s.connect(('10.255.255.255', 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

ip = get_local_ip()
target_line = f"ESP32_IP = 'http://{ip}:8000'\n"

rest_file = 'REST.py'
new_lines = []
found = False

with open(rest_file, 'r') as f:
    for line in f:
        if line.strip().startswith('ESP32_IP'):
            new_lines.append(target_line)
            found = True
        else:
            new_lines.append(line)

if not found:
    new_lines.insert(0, target_line)

with open(rest_file, 'w') as f:
    f.writelines(new_lines)

print(f"[INFO] Updated ESP32_IP to http://{ip}:8000 in {rest_file}")
