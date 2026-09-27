#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define RXD2 13
#define TXD2 12
#define BAUDRATE 115200

TaskHandle_t xTaskRxHandle = NULL;
TaskHandle_t xTaskTxHandle = NULL;

void TaskRx(void *pvParameters) {
  String buffer = "";
  buffer.reserve(128);

  for (;;) { // = while true
    while (Serial2.available() > 0) {
      char c = Serial2.read();
      if (c == '\n') {
        Serial.print("[RX Core ");
        Serial.print(xPortGetCoreID()); 
        Serial.print("] RPi disse: ");
        Serial.println(buffer);
        buffer = ""; 
      } else if (c != '\r') {
        buffer += c;
      }
      vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void TaskTx(void *pvParameters) {
  uint32_t counter = 0;

  for (;;) {
    String payload = "LILYGO_DATA_COUNT:" + String(counter++) + "\n";

    Serial2.print(payload);
    Serial.print("[TX Core ");
    Serial.print(xPortGetCoreID()); 
    Serial.print("] Enviado: ");
    Serial.print(payload);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); 

  //inicializa UART2 nos pinos mapeados
  Serial2.begin(BAUDRATE, SERIAL_8N1, RXD2, TXD2);

  Serial.println("ESP INICIALIZADA");

  xTaskCreatePinnedToCore(
    TaskRx, "Task_RX", 4096, NULL, 1, &xTaskRxHandle, 1
  );

  xTaskCreatePinnedToCore(
    TaskTx, "Task_TX", 4096, NULL, 1, &xTaskTxHandle, 1
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
