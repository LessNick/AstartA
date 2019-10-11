////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AstartA Project v0.02 alpha
// Copyright 2019 © LessNick aka breeze/fishbonce crew
// https://github.com/LessNick/AstartA
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Required: Arduino DUE + Ethernet Shield
// Supported: ATR (Read/Write), XEX & CAS (read only)
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
String inputArg1 = "";
String inputArg2 = "";
bool iCmdComplete = false;
bool iDrvComplete = false;
bool inputComplete = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {

  // Установка скорости порта для работы с ATARI SIO
  ATARI_SIO.begin(19200);

#ifdef DEBUG
  // Установка скорости порта консоли для отладки
  LOG.begin(115200);
  LOG.println("Astarta v0.02 alpha");
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

  // Инициализация SIO: номер дисковода + имя образа для загрузки
  atariSio.init(1, "");
//  atariSio.init(1, "autorun.atr");

    rootPath = "/";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  // Обработка команд Atari SIO
  atariSio.update();

  if (inputComplete) {
    if (inputCmd == "cd") {
      cmdCd(inputArg1);
    } else if (inputCmd == "dir") {
      cmdDir();
    } else if (inputCmd == "mount") {
      cmdMount(inputArg1, inputArg2);
    }
      
    inputCmd = "";
    inputArg1 = "";
    inputArg2 = "";
    inputComplete = false;
    iCmdComplete = false;
    iDrvComplete = false;
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
    if (!iCmdComplete && inChar == ' ') {
      iCmdComplete = true;
      
    } else if (!iDrvComplete && inChar == ' ') {
      iDrvComplete = true;
      
    } else if (inChar == '\n') {
      inputComplete = true;
    
    } else {
      if (!iCmdComplete) {
        inputCmd += inChar;
      } else if (!iDrvComplete) {
        inputArg1 += inChar;
      } else {
        inputArg2 += inChar;
      }
    }    
  }
}

void cmdCd(String dirName) {
  LOG.println("============================================");
  LOG.print("[cd] - ");  
  LOG.println(dirName);
  LOG.println("============================================");
  
  String newRootPath = rootPath + "/" + dirName;
  if (dirName == "/") {
    newRootPath = "/";
    
  } else if (dirName == "..") {
    if (rootPath.length() > 0) {
      for (int i=rootPath.length(); i>=0; i--) {
        if (rootPath.charAt(i) == '/' || rootPath.charAt(i) == '\\') {
          if (i == 0) {
            newRootPath = "/";
          } else {
            newRootPath = rootPath.substring(0,i);
          }
          break;
        }
      }
    } else {
      newRootPath = "/";
    }
    
  } else if (rootPath == "/") {
    newRootPath = "/" + dirName;
  }
  char buf[255];
  newRootPath.toCharArray(buf, 255);

    LOG.print("Try change dir to \"");
    LOG.print(newRootPath);
    LOG.print("\" - ");
  
  if (dirFile.open(buf, O_RDONLY)) {
    rootPath = newRootPath;

    LOG.println("OK");
        
    dirFile.close();
  } else {
    LOG.println("FAILED");
  }
}

void cmdDir() {
  LOG.println("============================================");
  LOG.print("[dir] - ");
  LOG.println(rootPath);
  LOG.println("============================================");
  
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

void cmdMount(String mountDrvId, String mountFileName) {
  LOG.println("============================================");
  LOG.print("[mount] - Drive: ");
  LOG.print(mountDrvId);
  LOG.print(" - ");  
  
  if (rootPath == "/") {
    mountFileName = "/" + mountFileName;
  } else {
    mountFileName = rootPath + "/" + mountFileName;
  }

  LOG.println(mountFileName);
  LOG.println("============================================");
  
  char buf[255];
  mountFileName.toCharArray(buf, 255);
  atariSio.init(mountDrvId.toInt(), buf);
}
