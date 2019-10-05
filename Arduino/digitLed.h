#ifndef DIGITLED_h
#define DIGITLED_h

#include <Arduino.h>

class DigitLed {
public:
  DigitLed();
  void            init();                             // Настройка параметров
  void            refresh();                          // Перерисовка идикатора
  void            setValue(byte val);                 // Установить новое значение
  void            setFirstDot(bool firstDot);         // Установить/cбросить первую точку
  void            setSecondDot(bool secondDot);       // Установить/cбросить вторую точку
  
private:
  void            updateLed();                        // Перерисовка цифр
  void            setDigit(char num);                 // Установит значения сегментов
  char            charH;                              // Значение цифры слева (hight)
  char            charL;                              // Значение цифры справал (low)
  bool            m_firstDot;                         // Состояние точки слева
  bool            m_secondDot;                        // Состояние точки справа
  bool            drawFirstLed;                       // Отрисовка первого символа (или второго)

  uint32_t        ts;

  int             m_base_1;                           // Управляющий PIN (земля) для отображения 1й цифры
  int             m_base_2;                           // Управляющий PIN (земля) для отображения 2й цифры

  int             m_led_A;                            // PIN к которому подключен сегмент A
  int             m_led_B;                            // PIN к которому подключен сегмент B
  int             m_led_C;                            // PIN к которому подключен сегмент C
  int             m_led_D;                            // PIN к которому подключен сегмент D
  int             m_led_E;                            // PIN к которому подключен сегмент E
  int             m_led_F;                            // PIN к которому подключен сегмент F
  int             m_led_G;                            // PIN к которому подключен сегмент G
  int             m_led_P;                            // PIN к которому подключен сегмент точка

  int             m_gnd;                              // PIN земли. на него подаётся LOW уровень

};

#endif
