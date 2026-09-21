#include "futural.h"
#include "scripts.h"
#include "timesi.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
#include <string>
#include <ESP32Servo.h>

// 4 = 4-wire full-step (faster, ~2048 steps/rev)
// 8 = 4-wire half-step (smoother, ~4096 steps/rev)
#define MOTOR_INTERFACE_TYPE 4

// Motor X Pins: IN1, IN2, IN3, IN4
#define X_IN1 4
#define X_IN2 15
#define X_IN3 2
#define X_IN4 14

// Motor Y Pins: IN1, IN2, IN3, IN4
#define Y_IN1 12
#define Y_IN2 13
#define Y_IN3 16
#define Y_IN4 0

#define BOT_FC_X 8
#define BOT_FC_Y 9

#define SERVO 7

Servo servZ;

const int MAX_X_OFFSET = 100000; // alterar para algo compatível com a estrutura final
const int MAX_Y_OFFSET = 100000; // alterar para algo compatível com a estrutura final

// IMPORTANT: Wire order passed to AccelStepper MUST be: IN1, IN3, IN2, IN4
AccelStepper stepperX(MOTOR_INTERFACE_TYPE, X_IN1, X_IN3, X_IN2, X_IN4);
AccelStepper stepperY(MOTOR_INTERFACE_TYPE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);

MultiStepper steppers;

class Ctrl{
  public:

    int xOffset = 0; // para levar em conta próximas letras e palavras
    int yOffset = 0; // para levar em conta mudanças de linha

    void zeroPosition(){
      // enquanto botão fim de curso x não grita: vai pra origem x
      // enquanto botão fim de curso y não grita: vai pra origem y
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

    void checkLimits(){
      // a partir dos valores de offset maximo, calcula se a palavra atual iria "vazar"
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

          int x1 = data[i + 0] + control->xOffset;
          int y1 = data[i + 1] + control->yOffset;
          int x2 = data[i + 2] + control->xOffset;
          int y2 = data[i + 3] + control->yOffset;

          if(lastX2 != x1 || lastY2 != y1){
            control->liftPen(servZ);
            control->lineMove(x1, y1);
            control->lowerPen(servZ);
          }

          control->lineMove(x2, y2);
          lastX2 = x2;
          lastY2 = x2;
        }
        control->xOffset += width;
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

class Bot_FC_X{
  public:
    void setup(){
      pinMode(BOT_FC_X, INPUT);
    }
    bool isPressed(){
      if(digitalRead(BOT_FC_X) == HIGH){
        return true;
      }else{
        return false;
      }
    }
};

class Bot_FC_Y{
  public:
    void setup(){
      pinMode(BOT_FC_Y, INPUT);
    }
    bool isPressed(){
      if(digitalRead(BOT_FC_Y) == HIGH){
        return true;
      }else{
        return false;
      }
    }
};

class Display{
  public:
    void setup(){
      // vou ter que ver como o display da julinha funciona
    }
    void display(std::string text){
      // vou ter que ver como o display da julinha funciona
    }
};

Bot_FC_X botFCX;
Bot_FC_Y botFCY;
Ctrl control;
Text_Conversion textConversion;
Display display;

void setup(){
  
  Serial.begin(115200);

  stepperX.setMaxSpeed(600.0);
  stepperY.setMaxSpeed(600.0);

  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);

  servZ.attach(SERVO);

  botFCX.setup();
  botFCY.setup();

  control.zeroPosition();

  display.setup();
  
}

void loop(){
  
  textConversion.listenToNewText();
  
}