#include "futural.h"
#include "scripts.h"
#include "timesi.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <string>
#include <ESP32Servo.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//4 = 4-wire full-step (faster, ~2048 steps/rev)
//8 = 4-wire half-step (smoother, ~4096 steps/rev)
#define MOTOR_INTERFACE_TYPE 4

//Motor X Pins: IN1, IN2, IN3, IN4
#define X_IN1 10
#define X_IN2 11
#define X_IN3 12
#define X_IN4 13

//Motor Y Pins: IN1, IN2, IN3, IN4
#define Y_IN1 43
#define Y_IN2 44
#define Y_IN3 21
#define Y_IN4 16

#define BOT_FC_X 1
#define BOT_FC_Y 2
#define BOT_PAPEL 17

#define SERVO 3

#define DISPLAY_POWER_ON 15

#define RXD2 18
#define BAUDRATE 115200

#define SerialRPi Serial2

TaskHandle_t listenToNewText = NULL;
TaskHandle_t stringToDraw = NULL;
QueueHandle_t filaTexto = NULL;

Servo servZ;

const int MAX_X_OFFSET = 100000; //alterar para algo compatível com a estrutura final
const int MAX_Y_OFFSET = 100000; //alterar para algo compatível com a estrutura final

const int MAX_SPEED = 400;

const int FONT_SCALE = 10;

String mensagem = "";


//IMPORTANT: Wire order passed to AccelStepper MUST be: IN1, IN3, IN2, IN4
AccelStepper stepperX(MOTOR_INTERFACE_TYPE, X_IN1, X_IN3, X_IN2, X_IN4);
AccelStepper stepperY(MOTOR_INTERFACE_TYPE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);

MultiStepper steppers;

TFT_eSPI tft = TFT_eSPI();

class Bot_FC_X {
  public:
    void setup() {
      pinMode(BOT_FC_X, INPUT_PULLUP);
    }
    bool isPressed() {
      if (digitalRead(BOT_FC_X) == LOW) {
        return true;
      } else {
        return false;
      }
    }
};

class Bot_FC_Y {
  public:
    void setup() {
      pinMode(BOT_FC_Y, INPUT_PULLUP);
    }
    bool isPressed() {
      if (digitalRead(BOT_FC_Y) == LOW) {
        return true;
      } else {
        return false;
      }
    }
};

class Bot_Papel {
  public:
    void setup() {
      pinMode(BOT_PAPEL, INPUT_PULLUP);
    }
    bool isPressed() {
      if (digitalRead(BOT_PAPEL) == LOW) {
        return true;
      } else {
        return false;
      }
    }
};

Bot_FC_X botFCX;
Bot_FC_Y botFCY;
Bot_Papel botPapel;

class Display {
  public:

    char* lastDisplay;

    void setup() {
      //1. Turn on power to the peripheral and display rail
      pinMode(DISPLAY_POWER_ON, OUTPUT);
      digitalWrite(DISPLAY_POWER_ON, HIGH);
      delay(100);

      //2. Initialize display
      tft.init();
      tft.setRotation(1); //1 or 3 for landscape (320x170), 0 or 2 for portrait (170x320)
      tft.fillScreen(TFT_BLACK);
    }
    void showText(char* text) {
      if (lastDisplay == text) {
        return;
      } else {
        lastDisplay = text;
      }
      tft.fillScreen(TFT_BLACK);
      tft.drawRect(10, 10, 300, 150, TFT_BLUE);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString(text, 20, 30);
    }
};

class Ctrl {
  public:

    int xOffset = 0; //para levar em conta próximas letras e palavras
    int yOffset = 0; //para levar em conta mudanças de linha

    void zeroPosition() {
      //enquanto botão fim de curso x não grita: vai pra origem x
      //enquanto botão fim de curso y não grita: vai pra origem y
      stepperX.move(-1);
      while (botFCX.isPressed() == false) {
        //claude e gemini falaram que sem o runSpeed ele não se move, o move(-1) apenas seta a posição e não move o motor podem estat chapando
        stepperX.move(-1);
        stepperX.run();

      }
      while (botFCY.isPressed() == false) {
        stepperY.move(-1);
        stepperY.run();
      }
      stepperX.setCurrentPosition(0);
      stepperY.setCurrentPosition(0);
    }

    void liftPen(Servo* servZ) {
      //controla o servo para levantar a caneta
      servZ->write(0); vTaskDelay(pdMS_TO_TICKS(250));
    };

    void lowerPen(Servo* servZ) {
      //controla o servo para abaixar a caneta
      servZ->write(100); vTaskDelay(pdMS_TO_TICKS(250));
    };

    void lineMove(int x, int y) {
      long target[2];

      //as coordenadas são absolutas. NÃO SÃO RELATIVAS
      target[0] = x;
      target[1] = y;

      steppers.moveTo(target);
      steppers.runSpeedToPosition();
    }

    int checkLimits(int addX, int addY) {
      //0 = out of bounds x; 1 = out of bounds y; 2 = in bounds
      if (xOffset + addX > MAX_X_OFFSET) {
        return 0;
      } else if (yOffset + addY > MAX_Y_OFFSET) {
        return 1;
      }
      return 2;
    }

    void requestNewPage(Ctrl* control, Servo* servZ, Display* display, Bot_Papel* botPapel) {
      control->liftPen(servZ);
      display->showText("Troque a folha de papel!");
      while (botPapel->isPressed() == false) {
        //wait
        //perguntar para julinha como continuar ouvindo RX enquanto nesse estado
      }
      display->showText("Novo papel recebido! Zerando os eixos...");
      control->zeroPosition();
      control->xOffset = 0;
      control->yOffset = 0;
    }
};


Ctrl control;
Display display; //se torna um objeto global

class Text_Conversion {
  public:
    static int charToIndex(char c) { //coloquei elas static para elas poderem ser usadas sem objeto pelas outras funções da classe

      //já que começamos no char=32 e terminamos no char=127, em ordem,
      //para converter de char para index basta subtrair 32:

      return (unsigned char)c - 32;

    }

    static void indexToDrawGlyph(int index, Ctrl* control, Servo* servZ) {

      const char *data = futural[index]; //obtém o array de coordenadas para as linhas que definem o símbolo
      int size = futural_size[index]; //quantidade de coordenadas das linhas que definem o símbolo
      char width = futural_width[index]; //largura total do símbolo
      //o tipo char foi utilizado ao invés de int porque char ocupa apenas 1 byte e int ocupa 4 bytes. Assim, já que apenas um byte é suficiente, para os valores possíveis, isso é preferível.

      int lastX2 = -1;
      int lastY2 = -1;

      if (data != NULL) {
        for (int i = 0; i < size; i += 4) {

          int x1 = data[i + 0] * FONT_SCALE + control->xOffset;
          int y1 = data[i + 1] * FONT_SCALE + control->yOffset;
          int x2 = data[i + 2] * FONT_SCALE + control->xOffset;
          int y2 = data[i + 3] * FONT_SCALE + control->yOffset;

          if (lastX2 != x1 || lastY2 != y1) {
            control->liftPen(servZ);
            control->lineMove(x1, y1);
            control->lowerPen(servZ);
          }

          control->lineMove(x2, y2);
          lastX2 = x2;
          lastY2 = y2;
        }
        control->xOffset += width * FONT_SCALE;
      }
    }

    static void listenToNewText(void *pvParameters) {


      String mensagem = "";
      //fica constantemente ouvindo a entrada serial para detectar se a rasp enviou algum texto
      bool lendoMensagem = false;
      mensagem.reserve(64);

      for (;;) {
        while (SerialRPi.available() > 0) {
          char c = SerialRPi.read();


          if (c == '<') {
            lendoMensagem = true;
            mensagem = "";
          }

          else if (c == '>') {
            if (lendoMensagem) {
              Serial.print("[RECEBIDO COM SUCESSO]: ");
              Serial.println(mensagem);
              char msgLida[64]; //cria um array de char para armazenar a msg lida
              memset(msgLida, 0, sizeof(msgLida));
              mensagem.toCharArray(msgLida, 64);
              xQueueSend(filaTexto, &msgLida, portMAX_DELAY); //portMAX_DELAY: bloco de tempo para se esperar um evento acontecer
              lendoMensagem = false;
            }
          }
          //pega apenas os caracteres válidos dentro do pacote
          else if (lendoMensagem) {
            if (c >= 32 && c <= 126) {//ascii
              mensagem += c;
            }
          }

          vTaskDelay(pdMS_TO_TICKS(1));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }

    static void stringToDraw(void* pvParameters) {

      char msgRecebida[64];

      for (;;) {
        if (xQueueReceive(filaTexto, &msgRecebida, portMAX_DELAY) == pdTRUE) {
          String str = String(msgRecebida);
          if (str.isEmpty()) continue;
          if (str[(str.length()) - 1] != ' ') str += ' ';


          std::string currentWord = "";
          currentWord.reserve(64);

          for (int i = 0; i < str.length(); i++) {
            if (str[i] == ' ') {
              int wordWidth = 0;
              for (int a = 0; a < currentWord.length(); a++) {
                wordWidth += futural_width[charToIndex(currentWord[a])];
              }
              if (control.checkLimits(wordWidth, 0) == 0) {
                //x fora do limite
                if (control.checkLimits(0, futural_height * FONT_SCALE) == 1) {
                  //y também fora do limite
                  control.requestNewPage(&control, &servZ, &display, &botPapel);
                  for (int a = 0; a < currentWord.length(); a++) {
                    indexToDrawGlyph(charToIndex(str[i]), &control, &servZ);
                  }
                  currentWord.clear();
                } else {
                  control.xOffset = 0;
                  control.yOffset += futural_height * FONT_SCALE; //altura de char aleatorio, já que todos tem mesma altura
                  for (int a = 0; a < currentWord.length(); a++) {
                    indexToDrawGlyph(charToIndex(str[i]), &control, &servZ);
                  }
                  currentWord.clear();
                }
              } else {
                for (int a = 0; a < currentWord.length(); a++) {
                  indexToDrawGlyph(charToIndex(str[i]), &control, &servZ);
                }
                currentWord.clear();
              }
            } else {
              currentWord += str[i];
            }

          }
        }
      }
    }

};

Text_Conversion textConversion;

void setup() {

  Serial.begin(115200);

  stepperX.setMaxSpeed(MAX_SPEED);
  stepperY.setMaxSpeed(MAX_SPEED);

  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);

  servZ.attach(SERVO);

  botFCX.setup();
  botFCY.setup();
  botPapel.setup();

  control.zeroPosition();

  filaTexto = xQueueCreate(10, sizeof(char) * 64);

  display.setup();
  display.showText("VAMO TELLER JULIA BIAAA");

  pinMode(RXD2, INPUT_PULLUP);
  SerialRPi.begin(BAUDRATE, SERIAL_8N1, RXD2, -1);


  xTaskCreatePinnedToCore(
    Text_Conversion::listenToNewText, "listenToNewText", 4096, NULL, 1, NULL, 1
  );

  xTaskCreatePinnedToCore(
    Text_Conversion::stringToDraw, "drawNewText", 8192, NULL, 1, NULL, 1
  );


}

void loop() {

  vTaskDelay(pdMS_TO_TICKS(1000));
  //caso escute algo no serial, passar para as funções de desenho e ir acumulando em um array de coisas para escrever, como uma fila

}
