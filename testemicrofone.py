import speech_recognition as sr

recognizer = sr.Recognizer()

# Instancia o microfone fora do loop para não fechar/reabrir a stream a todo momento
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

        except sr.UnknownValueError:
            # Fala não reconhecida ou silêncio - continua escutando normalmente
            print("Não entendi o que foi dito.")
            continue
        except sr.RequestError as e:
            # Erro de conexão com a API do Google
            print(f"Erro na conexão com o serviço de voz: {e}")
            continue
        except Exception as e:
            # Evita que o programa quebre por erros pontuais de áudio (como o -9999)
            print(f"Erro inesperado de áudio: {e}")
            continue
