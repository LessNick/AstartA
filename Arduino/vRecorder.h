#ifndef VRECORDER_h
#define VRECORDER_h

#include <Arduino.h>
#include "SdFat.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class vRecorder {
public:
  vRecorder();
  void            init();                             // Инициализация
  void            keysCheck();                        // Для опроса кнопок

  void            mountImage(char* fileName);         // Подключить образ кассеты
  
private:
  bool            playBtnPressed;                     // Статус кнопки «Play»
  unsigned long   playBtnTimeStamp;                   // Время когда была нажата кнопка «Play»

  SdFile          imageFile;                          // Объект для чтения/записи образа диска
  int             m_imageSize;                        // Размер образа
  
//-------------------------------------------------------------------------------------------------------------------
  void            playBtnReleased();
};

#endif
