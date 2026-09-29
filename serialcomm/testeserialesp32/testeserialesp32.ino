#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define RXD2 13
#define BAUDRATE 9600


void TaskRx(void *pvParameters) {
  bool lendoMensagem = false;
  String mensagem = "";
  mensagem.reserve(64);

  for (;;) {
    while (SerialRPi.available() > 0) {
      char c = SerialRPi.read();

      // Marcador de início de pacote válido
      if (c == '<') {
        lendoMensagem = true;
        mensagem = "";
      } 
      // Marcador de fim de pacote válido
      else if (c == '>') {
        if (lendoMensagem) {
          Serial.print("[RECEBIDO COM SUCESSO]: ");
          Serial.println(mensagem);
          lendoMensagem = false;
        }
      } 
      // Acumula apenas os caracteres válidos dentro do pacote
      else if (lendoMensagem) {
        if (c >= 32 && c <= 126) { // Aceita apenas caracteres ASCII imprimíveis
          mensagem += c;
        }
      }

      vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n--- ESP32 Receptor com Filtro Anti-Ruído ---");

  // Habilita o Pull-Up interno no pino RXD2
  pinMode(RXD2, INPUT_PULLUP);

  // Inicializa a Serial com a Raspberry Pi
  SerialRPi.begin(BAUDRATE, SERIAL_8N1, RXD2, -1);

  xTaskCreatePinnedToCore(
    TaskRx, "TaskRx", 4096, NULL, 1, NULL, 1
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}4