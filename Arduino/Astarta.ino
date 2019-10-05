////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AstartA Project v0.01 very alpha
// Copyright 2019 © LessNick aka breeze/fishbonce crew
// https://github.com/LessNick/AstartA
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Required: Arduino DUE + Ethernet Shield
// Supported: ATR, XEX, CAS (read only)
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <SPI.h>

#include "SdFat.h"
#include "sdios.h"

#include "sio.h"

// Глобальные переменные

SdFat sd;
SdFile file;
SdFile dirFile;

String rootPath;

SIO atariSio;

String inputCmd = "";
String inputArg = "";
bool cmdComplete = false;
bool inputComplete = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  // Установка скорости порта для работы с ATARI SIO
  ATARI_SIO.begin(19200);

#ifdef DEBUG
  // Установка скорости порта консоли для отладки
  LOG.begin(115200);
  LOG.println("Astarta v.001 very alpha");
  LOG.println("-------------------------------");
#endif

#ifdef DEBUG  
  LOG.print("Try to init SDCard ");
#endif

 // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(SD_PIN_SELECT, SD_SCK_MHZ(50))) {
#ifdef DEBUG  
    LOG.println("ERROR");
#endif    
    sd.initErrorHalt();
  } else {

#ifdef DEBUG  
    LOG.println("OK");
#endif    
  }

#ifdef DEBUG  
  LOG.println("Setup Complete!");
#endif

  // Инициализация SIO + имя образа для загрузки
  atariSio.init("");
//  atariSio.init("autorun.atr");

    rootPath = "/";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Обработка команд Atari SIO
  atariSio.update();

  if (inputComplete) {
    if (inputCmd == "cd") {
      cmdCd(inputArg);
    } else if (inputCmd == "dir") {
      cmdDir();
    } else if (inputCmd == "mount") {
      cmdMount(inputArg);
    }
      
    inputCmd = "";
    inputArg = "";
    inputComplete = false;
    cmdComplete = false;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback вызываемы при получении данных со стороны Atari
void ATARI_SIO_EVENT() {
  atariSio.incomingByte();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback вызываемы при получении данных со стороны отладочной консоли
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (!cmdComplete && inChar == ' ') {
      cmdComplete = true;
    
    } else if (inChar == '\n') {
      inputComplete = true;
    
    } else {
      if (cmdComplete) {
        inputArg += inChar;
      } else {
        inputCmd += inChar;
      }
    }    
  }
}

void cmdCd(String dirName) {
  LOG.print("[cd] - ");  
  LOG.println(dirName);

  String newRootPath = rootPath + "/" + dirName;
  if (dirName == "/") {
    newRootPath = "/";
  }else if (rootPath == "/") {
    newRootPath = "/" + dirName;
  }
  char buf[255];
  newRootPath.toCharArray(buf, 255);
  
  if (dirFile.open(buf, O_RDONLY)) {
    rootPath = newRootPath;
    LOG.print("dir changed to \"");
    LOG.print(rootPath);
    LOG.println("\"");
    
    dirFile.close();
  } else {
    LOG.println("change dir failed");
  }
}

void cmdDir() {
  LOG.print("[dir] - ");
  LOG.println(rootPath);
  char buf[255];
  rootPath.toCharArray(buf, 255);
  
  if (!dirFile.open(buf, O_RDONLY)) {
    LOG.print("open dir \"");
    LOG.print(rootPath);
    LOG.println("\" failed");
  } else {
    while (file.openNext(&dirFile, O_RDONLY)) {
      if (file.isSubDir()) {
        LOG.print("[D] ");
      } else {
        LOG.print("[F] ");
      }
      file.printName(&LOG);
      LOG.println();
      file.close();
    }
    dirFile.close();
  }
}

void cmdMount(String mountFileName) {
  LOG.print("[mount] - ");
  LOG.println(rootPath);
  if (rootPath == "/") {
    mountFileName = "/" + mountFileName;
  } else {
    mountFileName = rootPath + "/" + mountFileName;
  }
  char buf[255];
  mountFileName.toCharArray(buf, 255);
  atariSio.init(buf);
}
