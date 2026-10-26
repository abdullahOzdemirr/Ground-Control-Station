import socket
import json
import time
import threading

GCS_IP = "127.0.0.1"
GCS_PORT = 9000
SIM_PORT = 8081

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", SIM_PORT))

flight_state = "GROUND"
altitude = 0.0
x_position = 0.0
y_position = 0.0
speed = 0.0
battery = 100.0
roll_angle = 0.0
pitch_angle = 0.0

def listen_commands():
    global flight_state, speed, altitude, x_position, y_position, roll_angle, pitch_angle
    print(f"IHA Command Listener Active -> Port {SIM_PORT}")
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            cmd = json.loads(data.decode('utf-8'))
            command_type = cmd.get("command")
            print(f"\n[COMMAND RECEIVED]: {command_type}")
            
            if command_type == "TAKEOFF":
                flight_state = "TAKEOFF"
            elif command_type == "RTH":
                flight_state = "RTH"
            elif command_type == "LAND":
                flight_state = "LANDING"
            elif command_type == "DISARM":
                flight_state = "GROUND"
                speed = 0.0
                roll_angle = 0.0
                pitch_angle = 0.0
            # Manuel Eksen Komutları (Y ekseni düzeltildi: Sol -, Sağ +)
            elif command_type == "MOVE_FORWARD":
                x_position += 10.0
                pitch_angle = 10.0
            elif command_type == "MOVE_BACKWARD":
                x_position -= 10.0
                pitch_angle = -10.0
            elif command_type == "MOVE_LEFT":
                y_position -= 10.0  # Sola gittikçe azalır (-)
                roll_angle = -15.0
            elif command_type == "MOVE_RIGHT":
                y_position += 10.0  # Sağa gittikçe artar (+)
                roll_angle = 15.0
            elif command_type == "ALT_UP":
                altitude += 5.0
            elif command_type == "ALT_DOWN":
                if altitude >= 5.0:
                    altitude -= 5.0
        except Exception as e:
            break

threading.Thread(target=listen_commands, daemon=True).start()

print(f"IHA Simulator Started -> Target GCS: {GCS_IP}:{GCS_PORT}")
counter = 1

try:
    while True:
        altitude, x_position, y_position, speed, battery, roll_angle, pitch_angle
        
        if flight_state == "TAKEOFF":
            altitude += 5.0
            x_position += 2.0
            speed = 15.0
            pitch_angle = 12.0
            roll_angle = 0.0
            if altitude >= 100.0:
                flight_state = "CRUISE"
        elif flight_state == "CRUISE":
            altitude = 100.0 + (counter % 5) - 2.0
            x_position += 1.0
            speed = 20.0
            pitch_angle = 2.0
            roll_angle = 0.0
        elif flight_state == "RTH":
            if x_position > 0 or y_position != 0:
                if x_position > 0: x_position -= 4.0
                if y_position > 0: y_position -= 2.0
                elif y_position < 0: y_position += 2.0
                roll_angle = -20.0
                pitch_angle = 5.0
            if x_position <= 5.0 and y_position == 0 and altitude > 0:
                altitude -= 5.0
                roll_angle = 0.0
                pitch_angle = -10.0
            if x_position <= 0 and y_position == 0 and altitude <= 0:
                flight_state = "GROUND"
                roll_angle = 0.0
                pitch_angle = 0.0
        elif flight_state == "LANDING":
            altitude -= 5.0
            speed = 8.0
            roll_angle = 0.0
            pitch_angle = -10.0
            if altitude <= 0.0:
                altitude = 0.0
                speed = 0.0
                flight_state = "GROUND"
                pitch_angle = 0.0
        elif flight_state == "GROUND":
            altitude = 0.0
            speed = 0.0

        if flight_state != "GROUND" and battery > 2.0:
            battery -= 0.04

        telemetry_data = {
            "id": "MERGEN_IHA_01",
            "seq": counter,
            "altitude_m": round(altitude, 2),
            "x_pos_m": round(x_position, 2),
            "y_pos_m": round(y_position, 2),
            "speed_ms": round(speed, 2),
            "battery_pct": round(battery, 1),
            "roll_deg": round(roll_angle, 2),
            "pitch_deg": round(pitch_angle, 2),
            "status": flight_state
        }
        
        message = json.dumps(telemetry_data)
        sock.sendto(message.encode('utf-8'), (GCS_IP, GCS_PORT))
        counter += 1
        time.sleep(0.5)
        
except KeyboardInterrupt:
    print("\nSimulator stopped.")
    sock.close()