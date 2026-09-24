#include "futural.h"
#include "scripts.h"
#include "timesi.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <string>
#include <ESP32Servo.h>
#include <TFT_eSPI.h>

// 4 = 4-wire full-step (faster, ~2048 steps/rev)
// 8 = 4-wire half-step (smoother, ~4096 steps/rev)
#define MOTOR_INTERFACE_TYPE 4

// Motor X Pins: IN1, IN2, IN3, IN4
#define X_IN1 10
#define X_IN2 11
#define X_IN3 12
#define X_IN4 13

// Motor Y Pins: IN1, IN2, IN3, IN4
#define Y_IN1 43
#define Y_IN2 44
#define Y_IN3 21
#define Y_IN4 16

#define BOT_FC_X 1
#define BOT_FC_Y 2

#define SERVO 3

#define DISPLAY_POWER_ON 15

Servo servZ;

const int MAX_X_OFFSET = 100000; // alterar para algo compatível com a estrutura final
const int MAX_Y_OFFSET = 100000; // alterar para algo compatível com a estrutura final

const int MAX_SPEED = 400;

const int FONT_SCALE = 10;

// IMPORTANT: Wire order passed to AccelStepper MUST be: IN1, IN3, IN2, IN4
AccelStepper stepperX(MOTOR_INTERFACE_TYPE, X_IN1, X_IN3, X_IN2, X_IN4);
AccelStepper stepperY(MOTOR_INTERFACE_TYPE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);

MultiStepper steppers;

TFT_eSPI tft = TFT_eSPI();


class Bot_FC_X{
  public:
    void setup(){
      pinMode(BOT_FC_X, INPUT_PULLUP);
    }
    bool isPressed(){
      if(digitalRead(BOT_FC_X) == LOW){
        return true;
      }else{
        return false;
      }
    }
};

class Bot_FC_Y{
  public:
    void setup(){
      pinMode(BOT_FC_Y, INPUT_PULLUP);
    }
    bool isPressed(){
      if(digitalRead(BOT_FC_Y) == LOW){
        return true;
      }else{
        return false;
      }
    }
};

Bot_FC_X botFCX;
Bot_FC_Y botFCY;

class Ctrl{
  public:

    int xOffset = 0; // para levar em conta próximas letras e palavras
    int yOffset = 0; // para levar em conta mudanças de linha

    void zeroPosition(){
      // enquanto botão fim de curso x não grita: vai pra origem x
      // enquanto botão fim de curso y não grita: vai pra origem y
      while(botFCX.isPressed() == false){
        stepperX.move(-1);
      }
      while(botFCY.isPressed() == false){
        stepperY.move(-1);
      }
      stepperX.setCurrentPosition(0);
      stepperY.setCurrentPosition(0);
    }

    void liftPen(Servo* servZ){
      // controla o servo para levantar a caneta
      servZ->write(0);
    };

    void lowerPen(Servo* servZ){
      // controla o servo para abaixar a caneta
      servZ->write(100);
    };

    void lineMove(int x, int y){
      long target[2];

      // as coordenadas são absolutas. NÃO SÃO RELATIVAS
      target[0] = x;
      target[1] = y;

      steppers.moveTo(target);
      steppers.runSpeedToPosition();
    }

    void checkLimits(int addX, int addY){
      //false = out of bounds; true = in bounds
      if(xOffset + addX > MAX_X_OFFSET){
        return false;
      }else if(yOffset + addY > MAX_Y_OFFSET){
        return false;
      }
      return true;
    }

    void requestNewPage(){
      // pausa a execução de tudo e demanda troca de folha, até que o usuário aperte o botão de falar afirmando que a folha foi trocada
    }
};

class Text_Conversion{
  public:
    int charToIndex(char c){

      // já que começamos no char=32 e terminamos no char=127, em ordem,
      // para converter de char para index basta subtrair 32:
      
      return (unsigned char)c - 32;
      
    }

    void indexToDrawGlyph(int index, Ctrl* control, Servo* servZ){

      const char *data = futural[index]; // obtém o array de coordenadas para as linhas que definem o símbolo
      int size = futural_size[index]; // quantidade de coordenadas das linhas que definem o símbolo
      char width = futural_width[index]; // largura total do símbolo
      // o tipo char foi utilizado ao invés de int porque char ocupa apenas 1 byte e int ocupa 4 bytes. Assim, já que apenas um byte é suficiente, para os valores possíveis, isso é preferível.

      int lastX2 = -1;
      int lastY2 = -1;

      if (data != NULL) {
        for (int i = 0; i < size; i += 4) {

          int x1 = data[i + 0] * FONT_SCALE + control->xOffset;
          int y1 = data[i + 1] * FONT_SCALE + control->yOffset;
          int x2 = data[i + 2] * FONT_SCALE + control->xOffset;
          int y2 = data[i + 3] * FONT_SCALE + control->yOffset;

          if(lastX2 != x1 || lastY2 != y1){
            control->liftPen(servZ);
            control->lineMove(x1, y1);
            control->lowerPen(servZ);
          }

          control->lineMove(x2, y2);
          lastX2 = x2;
          lastY2 = x2;
        }
        control->xOffset += width * FONT_SCALE;
      }
    }

    void stringToDraw(std::string str, Ctrl* control, Servo* servZ){

      for(int i = 0; i < str.length(); i++){
        indexToDrawGlyph(charToIndex(str[i]), control, servZ);
      }

    }

    void listenToNewText(){
      // fica constantemente ouvindo a entrada serial para detectar se a rasp enviou algum texto
    }
};

class Display{
  public:

    char* lastDisplay;

    void setup(){
      // 1. Turn on power to the peripheral and display rail
      pinMode(DISPLAY_POWER_ON, OUTPUT);
      digitalWrite(DISPLAY_POWER_ON, HIGH);
      delay(100);

      // 2. Initialize display
      tft.init();
      tft.setRotation(1); // 1 or 3 for landscape (320x170), 0 or 2 for portrait (170x320)
      tft.fillScreen(TFT_BLACK);
    }
    void showText(char* text){
      if(lastDisplay == text){
        return;
      }else{
        lastDisplay = text;
      }
      tft.fillScreen(TFT_BLACK);
      tft.drawRect(10, 10, 300, 150, TFT_BLUE);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setTextSize(2);
      tft.drawString(text, 20, 30);
    }
};

Ctrl control;
Text_Conversion textConversion;
Display display;

void setup(){
  
  Serial.begin(115200);

  stepperX.setMaxSpeed(MAX_SPEED);
  stepperY.setMaxSpeed(MAX_SPEED);

  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);

  servZ.attach(SERVO);

  botFCX.setup();
  botFCY.setup();

  control.zeroPosition();

  display.setup();
  display.showText("VAMO TELLER JULIA BIAAA");
  
}

void loop(){
  
  textConversion.listenToNewText();
  // caso escute algo no serial, passar para as funções de desenho e ir acumulando em um array de coisas para escrever, como uma fila
  
}