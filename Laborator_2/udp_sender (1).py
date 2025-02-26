import socket
import time


PEER_IP = "192.168.89.39"  
PEER_PORT = 10001  # 

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_command(command):

    sock.sendto(command.encode(), (PEER_IP, PEER_PORT))
    print(f"Am trimis comanda: {command}")

while True:
    try:
        print("Alege o comandă:")
        print("1. LED_ON")
        print("2. LED_OFF")
        print("3. SET_VALUE <val>")
        print("4. Ieșire")

        choice = input("Introduceți opțiunea: ")

        if choice == "1":
            send_command("GPIO4=0")
        elif choice == "2":
            send_command("GPIO4=1")
        elif choice == "3":
            value = input("Introduceți valoarea: ")
            send_command(f"SET_VALUE {value}")
        elif choice == "4":
            break
        else:
            send_command(f"SET_VALUE {value}")

        time.sleep(1)

    except KeyboardInterrupt:
        break

sock.close()