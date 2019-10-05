#include "config.h"

#ifdef DEBUG
#include "debugMsg.h"
#endif

#include "vRecorder.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Virtual Recorder - виртуальный магнитофон
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vRecorder::vRecorder() { }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vRecorder::init() {
  pinMode(PLAY_BTN_PIN, INPUT);
  pinMode(BTN_GND, OUTPUT);

  digitalWrite(PLAY_BTN_PIN, HIGH);  // pullup resistors
  digitalWrite(BTN_GND, LOW);

  playBtnPressed = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vRecorder::playBtnReleased() {
  
  if (imageFile.isOpen()) {
#ifdef DEBUG
    Serial.println("PLAY STARTED");
#endif
    unsigned int nAddr = 0;

    byte tData[8];

    while (nAddr < m_imageSize) {

      char cName[4+1];              // Название блока
      unsigned short  cSize;        // Размер блока
      unsigned short  cAux;         // Доп. данные
  
      imageFile.seekSet(nAddr);
      imageFile.read(tData, 8);
  
      memcpy(cName, tData, 4);
      cName[4] = 0x00;
      
      cSize = tData[4] + tData[5]*256;
      cAux = tData[6] + tData[7]*256;

#ifdef DEBUG
      Serial.print("Name:");
      Serial.println(cName);
  
      Serial.print("Size:");
      Serial.println(cSize);
  
      Serial.print("Aux:");
      Serial.println(cAux);
#endif 

      nAddr += 8;

      if(strcmp(cName, "FUJI") == 0) {
#ifdef DEBUG
        Serial.println("Cassette format detected!");
#endif
      } else if(strcmp(cName, "baud") == 0) {
      
#ifdef DEBUG
        Serial.print("Change port speed to: ");
        Serial.println(cAux);
//        ATARI_SIO.begin(cAux);
#endif

      } else if(strcmp(cName, "data") == 0) {

#ifdef DEBUG
        Serial.print("Wait (GAP, ms): ");
        Serial.println(cAux);
#endif

        delay(cAux);
        
        byte sData[cSize];
        imageFile.seekSet(nAddr);
        imageFile.read(sData, cSize);

        digitalWrite(LED_BUILTIN, HIGH);

#ifdef DEBUG
        String sendResult = "";
#endif
  
        byte* b = sData;
        for (int i=0; i < cSize; i++) {
    
#ifdef DEBUG
          sendResult += DebugMsg::getHexString(*b, i);
#endif
          ATARI_SIO.write(*b);
          b++;       
        }

        byte crc = 0x00;
#ifdef DEBUG
        DebugMsg::logSendDev(sendResult, crc);
#endif
    
        digitalWrite(LED_BUILTIN, LOW);
      
        // !!! Обязательно !!!
        ATARI_SIO.flush();
      
#ifdef DEBUG
        Serial.print("Send data size:");
        Serial.println(cSize);
#endif     
      }
      
      nAddr += cSize;

    }

  } else {
    
#ifdef DEBUG
    Serial.println("[ERROR] Image not mounted!");
#endif

  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vRecorder::keysCheck() {

    bool playBtnActive = digitalRead(PLAY_BTN_PIN) ? false : true; // LOW (with pull-up res) when button pressed (on) 
    
    if (playBtnActive) {
      playBtnPressed = true;
      playBtnTimeStamp = millis();
    } else {
      if (playBtnPressed) {
        if (millis() - playBtnTimeStamp > DEBOUNCE_DELAY) {
          playBtnPressed = false;
          playBtnReleased();
        }
      }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vRecorder::mountImage(char* fileName) {

  // Установка скорости порта для работы с ATARI SIO (Магнитофон)
  ATARI_SIO.begin(600);
  
#ifdef DEBUG  
    DebugMsg::tryOpen("CAS", fileName);
#endif

  imageFile.open(fileName, O_RDONLY);
  if (!imageFile.isOpen()) {

#ifdef DEBUG
    LOG.println("ERROR");
#endif

    return;
  
  } else {

#ifdef DEBUG
    LOG.println("OK");
#endif

    File f(fileName, O_RDONLY);
    m_imageSize = f.size();

#ifdef DEBUG
    LOG.println("TURN ON YOUR ATARI (SELECT+OPTIONS)PRESS RETURN AND AFTER SIGNAL PRESS PLAY");
#endif
  }

}
