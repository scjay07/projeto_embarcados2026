import speech_recognition as sr
import serial
import time

recognizer = sr.Recognizer()
ser = serial.Serial('/dev/serial0', 115200, timeout=1)
time.sleep(2) # Aguarda estabilização da conexão

with sr.Microphone() as mic:
    print("Ajustando ruído de fundo... Aguarde.")
    recognizer.adjust_for_ambient_noise(mic, duration=1)
    print("Pronto! Pode falar...")

    while True:
        try:
            audio = recognizer.listen(mic)
            text = recognizer.recognize_google(audio, language="pt-BR")
            text = text.lower()
            print(f"Você disse: {text}")
            text =  f"<{text}>"
            ser.write(text.encode('utf-8'))
            print(f"[RPi TX]: {text.strip()}")
            time.sleep(2)


        except sr.UnknownValueError:
            continue
        except sr.RequestError as e:
            print(f"Erro na conexão com o serviço de voz: {e}")
            continue
        except Exception as e:
            print(f"Erro inesperado de áudio: {e}")
            continue