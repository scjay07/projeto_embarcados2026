#include "futural.h"
#include "scripts.h"
#include "timesi.h"
#include <AccelStepper.h>
#include <MultiStepper.h>

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

// IMPORTANT: Wire order passed to AccelStepper MUST be: IN1, IN3, IN2, IN4
AccelStepper stepperX(MOTOR_INTERFACE_TYPE, X_IN1, X_IN3, X_IN2, X_IN4);
AccelStepper stepperY(MOTOR_INTERFACE_TYPE, Y_IN1, Y_IN3, Y_IN2, Y_IN4);

MultiStepper steppers;

int xOffset = 0; // para levar em conta próximas letras e palavras
int yOffset = 0; // para levar em conta mudanças de linha

const int MAX_X_OFFSET = 100000; // alterar para algo compatível com a estrutura final
const int MAX_Y_OFFSET = 100000; // alterar para algo compatível com a estrutura final

int translationCharToIndex(char c);
void indexToDrawGlyph(int index);
void liftPen();
void lowerPen();
void lineMove(int x, int y);
void zeroPosition();

void setup(){
  
  Serial.begin(115200);

  stepperX.setMaxSpeed(600.0);
  stepperY.setMaxSpeed(600.0);

  steppers.addStepper(stepperX);
  steppers.addStepper(stepperY);

  zeroPosition();
  
}




void loop(){
  
  lineMove(10000, 0);
  delay(1000);
  lineMove(0, 0);
  // fazer aqui a máquina de estados
  
}

int translationCharToIndex(char c){

  // já que começamos no char=32 e terminamos no char=127, em ordem,
  // para converter de char para index basta subtrair 32:
  
  return (unsigned char)c - 32;
  
}

void indexToDrawGlyph(int index){

  const char *data = futural[index]; // obtém o array de coordenadas para as linhas que definem o símbolo
  int size = futural_size[index]; // quantidade de coordenadas das linhas que definem o símbolo
  char width = futural_width[index]; // largura total do símbolo

  // o tipo char foi utilizado ao invés de int porque char ocupa apenas 1 byte e int ocupa 4 bytes. Assim, já que apenas um byte é suficiente, para os valores possíveis, isso é preferível.

  int lastX2 = -1;
  int lastY2 = -1;

  if (data != NULL) {
      for (int i = 0; i < size; i += 4) {

        int x1 = data[i + 0] + xOffset;
        int y1 = data[i + 1] + yOffset;
        int x2 = data[i + 2] + xOffset;
        int y2 = data[i + 3] + yOffset;

        if(lastX2 != x1 || lastY2 != y1){
          liftPen();
          lineMove(x1, y1);
          lowerPen();
        }

        lineMove(x2, y2);
        lastX2 = x2;
        lastY2 = x2;
      }
      xOffset += width;
  }

}

void liftPen(){};

void lowerPen(){};

void lineMove(int x, int y){

  long target[2];

  // as coordenadas são absolutas. NÃO SÃO RELATIVAS
  target[0] = x;
  target[1] = y;

  steppers.moveTo(target);
  steppers.runSpeedToPosition();

}

void zeroPosition(){
  // enquanto botão fim de curso x não grita: vai pra origem x
  // enquanto botão fim de curso y não grita: vai pra origem y
  stepperX.setCurrentPosition(0);
  stepperY.setCurrentPosition(0);
}

