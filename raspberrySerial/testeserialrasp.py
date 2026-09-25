import serial
import time

# Na Raspberry Pi 3, a serial primaria costuma ser /dev/ttyS0 ou /dev/serial0
ser = serial.Serial('/dev/serial0', 115200, timeout=1)
time.sleep(2) # Aguarda estabilização da conexão

print("Iniciando comunicação Serial com ESP32...")

try:
    count = 0
    while True:
        # Envia dados para o ESP32
        send_msg = f"RPI_PING_COUNT:{count}\n"
        ser.write(send_msg.encode('utf-8'))
        print(f"[RPi TX]: {send_msg.strip()}")
        count += 1
        
        # Lê dados recebidos do ESP32
        if ser.in_waiting > 0:
            incoming = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"[RPi RX]: {incoming}")
            
        time.sleep(2)

except KeyboardInterrupt:
    ser.close()
    print("Conexão encerrada.")