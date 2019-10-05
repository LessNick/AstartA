#include "config.h"

#include "digitLed.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Digital Led - Управление сдвоенным семисегментным идикатором
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Таблица соотвествия сегментов для отображения цифры
int digit[][7] {
    // A    B     C     D     E     F     G
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW  },     // 0
    { LOW,  HIGH, HIGH, LOW,  LOW,  LOW,  LOW  },     // 1
    { HIGH, HIGH, LOW,  HIGH, HIGH, LOW,  HIGH },     // 2
    { HIGH, HIGH, HIGH, HIGH, LOW,  LOW,  HIGH },     // 3
    { LOW,  HIGH, HIGH, LOW,  LOW,  HIGH, HIGH },     // 4
    { HIGH, LOW,  HIGH, HIGH, LOW,  HIGH, HIGH },     // 5
    { HIGH, LOW,  HIGH, HIGH, HIGH, HIGH, HIGH },     // 6
    { HIGH, HIGH, HIGH, LOW,  LOW,  LOW,  LOW  },     // 7
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH },     // 8
    { HIGH, HIGH, HIGH, HIGH, LOW,  HIGH, HIGH }      // 9
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DigitLed::DigitLed() {
  charH = 0;
  charL = 0;
  ts = 0;

  drawFirstLed = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::init() {

  // Пины для переключения транзисторов
  pinMode(LED_BASE_1, OUTPUT);
  pinMode(LED_BASE_2, OUTPUT);
  pinMode(LED_GND, OUTPUT);

  digitalWrite(LED_BASE_1, HIGH);
  digitalWrite(LED_BASE_2, HIGH);
  digitalWrite(LED_GND, LOW);
  
  // Пины для индикатора
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(LED_C, OUTPUT);
  pinMode(LED_D, OUTPUT);
  pinMode(LED_E, OUTPUT);
  pinMode(LED_F, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_P, OUTPUT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::refresh() {
  if (millis() - ts >= 10) {
     ts = millis();
     updateLed();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::setValue(byte val) {
  if (val > 99) val = 99;
  charH = val / 10;
  charL = val % 10;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::setFirstDot(bool firstDot) {
  m_firstDot = firstDot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::setSecondDot(bool secondDot) {
  m_secondDot = secondDot;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::updateLed() {
  if (drawFirstLed) {
    drawFirstLed = false;
    digitalWrite(LED_BASE_1, HIGH);
    digitalWrite(LED_BASE_2, LOW);
  
    setDigit(charH);

    if (m_firstDot) {
      digitalWrite(LED_P, HIGH);
    } else {
      digitalWrite(LED_P, LOW);
    }
  
  } else {
    drawFirstLed = true;
    digitalWrite(LED_BASE_1, LOW);
    digitalWrite(LED_BASE_2, HIGH);
  
    setDigit(charL);
    
    if (m_secondDot) {
      digitalWrite(LED_P, HIGH);
    } else {
      digitalWrite(LED_P, LOW);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DigitLed::setDigit(char num) {  
  digitalWrite(LED_A, digit[num][0]);
  digitalWrite(LED_B, digit[num][1]);
  digitalWrite(LED_C, digit[num][2]);
  digitalWrite(LED_D, digit[num][3]);
  digitalWrite(LED_E, digit[num][4]);
  digitalWrite(LED_F, digit[num][5]);
  digitalWrite(LED_G, digit[num][6]); 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
