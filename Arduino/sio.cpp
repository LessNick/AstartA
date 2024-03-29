#include "config.h"

#include "sio.h"

#ifdef DEBUG
#include "debugMsg.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIO — основной блок прёма/передачи данных с ATARI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIO::SIO() {
  // Инициализация контрольной сумма буфера
  m_cmdBufferCrc = 0;

  // Инициализация времени последнего попринятого байта
  m_timeReceivedByte = 0;

  //Инициализация vDrive
  m_vDrive1.init();
  m_vDrive2.init();
  m_vDrive3.init();
  m_vDrive4.init();

  //Инициализация vRecorder
  m_virtualRec.init();

  // Инициализация сектора
  m_dataBuffer[0] = 0;
  m_dataBufferPtr = m_dataBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::init(int drvId, char* mountFileName) {

  byte mountDriveId = 0x30 + drvId;
  
  // Инициализация vRecorder
  // Иначе колбасит кнопку «Play»
  m_virtualRec.init();
  
  // Установить пин Atari Cmd на вход (приём данных)
  pinMode(ATARI_CMD_PIN, INPUT);

  // initialize digital pin as an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Начальная инициализация SIO
  changeState(SIO_READY);

  vDrive* workDrive = getVDriveById(mountDriveId);

  if (mountFileName != "") {
    byte mountFileType = getFileType(mountFileName);
    if (mountFileType == IMG_TYPE_XEX) {
        workDrive->init();
        workDrive->mountXex(mountFileName);
        
    } else if (mountFileType == IMG_TYPE_CAS) {
      //Инициализация vRecorder
      m_virtualRec.init();
      m_virtualRec.mountImage(mountFileName);
      
    } else {
        workDrive->init();
        workDrive->mountImage(mountFileName, mountFileType);
    }
  } else {
#ifdef DEBUG
    LOG.println("Empty init");
#endif
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vDrive* SIO::getVDriveById(byte drvId) {
  switch (drvId) {
    case DEV_D2:
      return &m_vDrive2;

    case DEV_D3:
      return &m_vDrive3;

    case DEV_D4:
      return &m_vDrive4;

  }
  return &m_vDrive1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte SIO::getFileType(char* fileName) {
  unsigned short dotPtr = strlen(fileName);
  unsigned short extSize = 0;
  for (unsigned short i=strlen(fileName); i>0; i--) {
    extSize++;
    if (*(fileName+i-1) == '.') break;
  }
  byte type = 0x00;
  char* p = fileName + (strlen(fileName)-extSize+1);
  // Сделано специально так, что бы прокатило как ATR или atr, так и Atr aTr aTR итд
  if ((*p=='A' || *p=='a') && (*(p+1)=='T' || *(p+1)=='t') && (*(p+2)=='R' || *(p+2)=='r')) {
    type = IMG_TYPE_ATR;
  } else if ((*p=='X' || *p=='x') && (*(p+1)=='E' || *(p+1)=='e') && (*(p+2)=='X' || *(p+2)=='x')) {
    type = IMG_TYPE_XEX;
  } else if ((*p=='C' || *p=='c') && (*(p+1)=='A' || *(p+1)=='a') && (*(p+2)=='S' || *(p+2)=='s')) {
    type = IMG_TYPE_CAS;
  }
  return type;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::updateIndicator() {
    //Для обновления индикации
    switch(m_cmdBuffer.devId) {
      case DEV_D2:
        m_vDrive2.refresh();
        break;
      
      case DEV_D3:
        m_vDrive3.refresh();
        break;
      
      case DEV_D4:
        m_vDrive4.refresh();
        break;
      
      default:
        m_vDrive1.refresh();
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::update() {
  
    updateIndicator();
    
    // Для опроса клавиш
    m_virtualRec.keysCheck();
    
    // Обработка статуса SIO
    switch (m_sioState) {
      // SIO готова к приёму команд
      case SIO_READY:
        checkCmdPinHigh();
        break;
        
      // Ожидание начала SIO команды
      case SIO_WAIT_CMD_START:
        checkCmdPinLow();
        break;

      // Чтение SIO команды
      case SIO_READ_CMD:
        checkReadCmdComplete();
        break;

      // Чтение SIO данных
      case SIO_READ_DATA:
        checkReadDataComplete();
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::changeState(byte state) {
  m_sioState = state;
  
#ifdef DEBUG
  DebugMsg::logSioState(m_sioState);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::checkCmdPinHigh() {
  if (digitalRead(ATARI_CMD_PIN) == HIGH) {
    changeState(SIO_WAIT_CMD_START);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::checkCmdPinLow() {
  if (digitalRead(ATARI_CMD_PIN) == LOW) {    
    delay(ATARI_CMD_DELAY);
    clearCmdBuffer();
    changeState(SIO_READ_CMD);
  } 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::clearCmdBuffer() {
  // Очистка буфера команд перед приёмом новой
  memset(&m_cmdBuffer, 0, sizeof(m_cmdBuffer));
  m_cmdBufferPtr = (byte*)&m_cmdBuffer;
  // Контрольная сумма буфера в 0
  m_cmdBufferCrc = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::checkReadDataComplete() { 
  if (m_dataBufferPtr - (byte*)&m_dataBuffer == m_currentSectorSize+1) {   
    // Данные получены
    delay(DELAY_ACK);
    ATARI_SIO.write(SEND_ACK);
   
    // Расчёт CRC нового пакета
    calcDataCrc();

    byte receivedCrc = m_dataBuffer[m_currentSectorSize];

#ifdef DEBUG
    DebugMsg::logReceivedData(m_dataBuffer, m_currentSectorSize, m_dataBufferCrc, receivedCrc);
#endif
    if (m_dataBufferCrc == receivedCrc) {
        // Приём завершён
        delay(DELAY_COMPLETE);
        ATARI_SIO.write(SEND_COMPLETE);

        if (m_cmdBuffer.devId >= DEV_D1 && m_cmdBuffer.devId <= DEV_DO) {
            //TODO: SAVE
            vDrive* workDrive = getVDriveById(m_cmdBuffer.devId);

            // Первые 3 сектора не зависимо от размеров всего 128б это бут область!
            m_currentSectorSize = 128;
            if (m_currentSector > 3) {
              m_currentSectorSize = workDrive->getSectorSize();
            }
            
            //workDrive->setSectorData(m_currentSector, (byte*)&m_dataBuffer);
            workDrive->setSectorData(m_currentSector, m_dataBuffer);
        
        } else if (m_cmdBuffer.devId == DEV_SDRIVE) {
            //TODO: SAVE
        }
        
        
        // !!! Обязательно !!!
        ATARI_SIO.flush();
    
        changeState(SIO_WAIT_CMD_START);
    
    } else {
      // Если ошибка контрольной суммы, то повторить запрос
      ATARI_SIO.write(SEND_NAK);
      changeState(SIO_READY);
    }
    
  // Иначе по истечению таймаута сбросить данные и ждать команду
  } else if (millis() - m_timeReceivedByte > READ_CMD_TIMEOUT) {
    clearCmdBuffer();
    changeState(SIO_READY);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::checkReadCmdComplete() { 
  // Если прилетели все 5 байт, пакет данных команды завершен
  if (m_cmdBufferPtr - (byte*)&m_cmdBuffer == 5) {    
    // Расчёт CRC нового пакета
    calcCmdDataCrc();

#ifdef DEBUG
    DebugMsg::logReceivedCmdData(m_cmdBuffer, m_cmdBufferCrc);
#endif

    if (m_cmdBufferCrc == m_cmdBuffer.crc) {
      
      changeState(SIO_WAIT_CMD_START);
      
      if (m_cmdBuffer.devId >= DEV_D1 && m_cmdBuffer.devId <= DEV_DO) {
        fCmdProcessing();
              
      } else if (m_cmdBuffer.devId == DEV_SDRIVE) {
        sCmdProcessing();
      }
      
    } else {
      // Если ошибка контрольной суммы, то повторить запрос
      ATARI_SIO.write(SEND_NAK);
      changeState(SIO_READY);
    } 
    
  // Иначе по истечению таймаута сбросить данные и ждать команду
  } else if (millis() - m_timeReceivedByte > READ_CMD_TIMEOUT) {
    clearCmdBuffer();
    changeState(SIO_READY);
  } 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::incomingByte() {
  byte b = ATARI_SIO.read();
  
  m_timeReceivedByte = millis();

  if (m_sioState == SIO_READ_CMD) {
    *m_cmdBufferPtr = b;
      m_cmdBufferPtr++;
  
  } else if (m_sioState == SIO_READ_DATA) {
    *m_dataBufferPtr = b;
    m_dataBufferPtr++;
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::calcDataCrc() {
  m_dataBufferCrc = calcCrc((byte*)&m_dataBuffer, m_currentSectorSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::calcCmdDataCrc() {
  m_cmdBufferCrc = calcCrc((byte*)&m_cmdBuffer, 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte SIO::calcCrc(byte* chunk, unsigned short len) {
  unsigned short crc = 0;
  for (int i=0; i < len; i++) {
    crc += chunk[i];
    if (crc > 255) {
      crc -= 255;
    }
  }
  return (byte)crc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::fCmdProcessing() {
  switch (m_cmdBuffer.cmdId) {
    case FCMD_READ:
        sendDevSector();
      break;
      
    case FCMD_WRITE:
      //TODO: 
      break;
          
    case FCMD_PUT:
      recieveDevSector();
      break;
      
    case FCMD_STATUS:
      sendDevStatus();
      break;
      
    case FCMD_FORMAT:
      //TODO: 
      break;
      
    case FCMD_FORMAT_MD:
      //TODO: 
      break;

    case FCMD_DEVPOOL3:
      //TODO: 
      break;
    
    case FCMD_CHUNK_INFO:
      sendDevChunkInfo();
      break;

    case FCMD_CHUNK_DATA:
      sendDevChunkData();
      break;

    case FCMD_NOTIFY_RUN:
      sendNotifyRun();
      break;
  } 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sCmdProcessing() {
  switch (m_cmdBuffer.cmdId) {
    case SCMD_GET20:
      sendSDriveNext20();
      break;
      
    case SCMD_IDENT:
      sendSDriveIdent();
      break;
      
    case SCMD_INIT:
      sendSDriveNop();
      break;
      
    case SCMD_CHDIR_VDN:
      sendSDriveZero(14);
      break;
      
    case SCMD_GET_ENTRIES:
      sendSDriveZero(m_cmdBuffer.aByte1*12);
      break;
      
    case SCMD_SWAP_VDN:
      sendSDriveNop();
      break;
      
    case SCMD_GETPARAMS:
      sendSDriveParams();
      break;
      
    case SCMD_MOUNT_D0:
      //TODO: 
      break;
      
    case SCMD_MOUNT_D1:
      //TODO: 
      break;
      
    case SCMD_MOUNT_D2:
      //TODO: 
      break;
      
    case SCMD_MOUNT_D3:
      //TODO: 
      break;
      
    case SCMD_MOUNT_D4:
      //TODO: 
      break;
      
    case SCMD_CHDIR_UP:
      //TODO: 
      break;
      
    case SCMD_CHROOT:
      sendSDriveNop();
      break;
      
    case SCMD_CHDIR:
      //TODO: 
      break;
  } 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendSDriveIdent() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  char* identData = m_sDrive->getIdent();
  sendBytes(strlen(identData), (byte*)identData);

  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendSDriveParams() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  SDParam* SDParam = m_sDrive->getSDParam();
  sendBytes(sizeof(SDParam), (byte*)SDParam);

  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendSDriveNop() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  // Приём завершён
  delay(DELAY_COMPLETE);
  ATARI_SIO.write(SEND_COMPLETE);

  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendSDriveZero(byte count) {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  // Приём завершён
  delay(DELAY_COMPLETE);
  ATARI_SIO.write(SEND_COMPLETE);

  for (int i=0; i < count; i++) {
    ATARI_SIO.write((byte)0x00);
  }
  ATARI_SIO.write((byte)0x00);

  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendSDriveNext20() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendDevStatus() {

  vDrive* workDrive = getVDriveById(m_cmdBuffer.devId);

//  byte mt = m_vDrive1.getMountedImgType();
  byte mt = workDrive->getMountedImgType();
#ifdef DEBUG
    LOG.print("Dev ID: ");
    LOG.println(m_cmdBuffer.devId, HEX);
    LOG.print("Mount type: ");
    LOG.println(mt, HEX);
#endif
  // Если тип примонтированного образа не ATR и не XEX, то молчать и не выдавать своё присутствие
  if (mt == IMG_TYPE_ATR || mt == IMG_TYPE_XEX) {
    
    // Данные получены
    delay(DELAY_ACK);
    ATARI_SIO.write(SEND_ACK);
   
    // Приём завершён
    delay(DELAY_COMPLETE);
    ATARI_SIO.write(SEND_COMPLETE);

    StatusFrame* m_statusFrame = workDrive->getStatusFrame();
    byte frameLength = sizeof(m_statusFrame);
  
#ifdef DEBUG
    String sendResult = "";
#endif
   
    byte* b = (byte*)m_statusFrame;
    for (int i=0; i < frameLength; i++) {
  
#ifdef DEBUG
      sendResult += DebugMsg::getHexString(*b, i);
#endif       
      ATARI_SIO.write(*b);
      b++;
    }
  
    byte crc = calcCrc((byte*)m_statusFrame, frameLength);
    ATARI_SIO.write(crc);
    
#ifdef DEBUG
    DebugMsg::logSendDev(sendResult, crc);
#endif
    
    // !!! Обязательно !!!
    ATARI_SIO.flush();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendDevSector() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  m_currentSector = m_cmdBuffer.aByte2*256 + m_cmdBuffer.aByte1;

  vDrive* workDrive = getVDriveById(m_cmdBuffer.devId);
  
  // Первые 3 сектора не зависимо от размеров всего 128б это бут область!
  m_currentSectorSize = 128;
  if (m_currentSector > 3) {
    m_currentSectorSize = workDrive->getSectorSize();
  }

  byte* sectorData = workDrive->getSectorData(m_currentSector);
  sendBytes(m_currentSectorSize, sectorData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::recieveDevSector() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  m_currentSector = m_cmdBuffer.aByte2*256 + m_cmdBuffer.aByte1;

  changeState(SIO_READ_DATA);

  memset(m_dataBuffer, 0, sizeof(m_dataBuffer));
  m_dataBufferPtr = m_dataBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendBytes(unsigned short sectorSize, byte* sectorData) {
  // Приём завершён
  delay(DELAY_COMPLETE);
  ATARI_SIO.write(SEND_COMPLETE);

#ifdef DEBUG
  String sendResult = "";
#endif

  digitalWrite(LED_BUILTIN, HIGH);
  
  byte* b = sectorData;
  for (int i=0; i < sectorSize; i++) {

#ifdef DEBUG
    sendResult += DebugMsg::getHexString(*b, i);
#endif
        
    ATARI_SIO.write(*b);  
    b++;

    // Обновление индикации
    updateIndicator();
  }

  byte crc = calcCrc(sectorData, sectorSize);
  ATARI_SIO.write(crc);

#ifdef DEBUG
  DebugMsg::logSendDev(sendResult, crc);
#endif

  digitalWrite(LED_BUILTIN, LOW);
  
  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendDevChunkInfo() {
  // Данные получены
  delay(DELAY_ACK);
//  m_atariSIO->write(SEND_ACK);
  ATARI_SIO.write(SEND_ACK);

  unsigned short m_currentChunk = m_cmdBuffer.aByte2*256 + m_cmdBuffer.aByte1;

  LOG.print("Get chunk info for:" );
  LOG.println(m_currentChunk);

  vDrive* workDrive = getVDriveById(m_cmdBuffer.devId);
  
  Diskette d = workDrive->getRecordInfo(m_currentChunk);
  unsigned int recCount = workDrive->getDiskRecCount();  // Количество чанков всего
  
  byte chunkInfo[6];
  chunkInfo[0] = d.loadAddr % 256;                       // +0x00: 2 байта: Адрес загрузки чанка
  chunkInfo[1] = d.loadAddr / 256;                       // +0x02: 1 байт: Всегда 1 (нафига?)
  chunkInfo[2] = 1;
  chunkInfo[3] = (recCount != m_currentChunk + 1);       // +0x03: 1 байт: Если последний блок (чанк) из передаваемых, то 0
                                                         //                В противном случае всегда 1. Нужно для игнора RUN-блоков, которые
                                                         //                грузятся в адреса 0x02e0+
  chunkInfo[4] = d.loadSize % 256;                       // +0x04: 2 байта: Размер загружаемого чанка
  chunkInfo[5] = d.loadSize / 256;
  
  byte frameLength = sizeof(chunkInfo);
 
#ifdef DEBUG
  String sendResult = "";
#endif

  byte crc = calcCrc((byte*)&chunkInfo, frameLength);
  
  // Приём завершён
  delay(DELAY_COMPLETE);
  ATARI_SIO.write(SEND_COMPLETE);
  
  byte* b = (byte*)&chunkInfo;
  for (int i=0; i < frameLength; i++) {

#ifdef DEBUG
    sendResult += DebugMsg::getHexString(*b, i);
#endif       

    ATARI_SIO.write(*b);
    b++;
  }

#ifdef DEBUG
  DebugMsg::logSendDev(sendResult, crc);
#endif

  ATARI_SIO.write(crc);

  // !!! Обязательно !!!
  ATARI_SIO.flush();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendDevChunkData() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

  unsigned short m_currentChunk = m_cmdBuffer.aByte2*256 + m_cmdBuffer.aByte1;

#ifdef DEBUG
  LOG.print("Get chunk data for:" );
  LOG.println(m_currentChunk);
#endif

  vDrive* workDrive = getVDriveById(m_cmdBuffer.devId);
  Diskette d = workDrive->getRecordInfo(m_currentChunk);

  byte* chunkData = workDrive->getRecordData(m_currentChunk);
  sendBytes(d.loadSize, chunkData);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SIO::sendNotifyRun() {
  // Данные получены
  delay(DELAY_ACK);
  ATARI_SIO.write(SEND_ACK);

#ifdef DEBUG
  LOG.println("ATARI Run code notification");
#endif
  
  // Приём завершён
  delay(DELAY_COMPLETE);
  ATARI_SIO.write(SEND_COMPLETE);
}
