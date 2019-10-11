#include "config.h"

#ifdef DEBUG
#include "debugMsg.h"
#endif

#include "vDrive.h"
#include "xexloader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Virtual Drive - виртуальный дисковод
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vDrive::vDrive() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::init() {
  // Установка скорости порта для работы с ATARI SIO (дисковод)
  ATARI_SIO.begin(19200);

  // Инициализация индикатора
  dl.init();

  // Мотор выкл
  m_statusFrame.drvStatus.motorRunning = 0;
  
  // Инвертированные значения
  m_statusFrame.ctrlStatus.busy = 1;
  m_statusFrame.ctrlStatus.dataPending = 1;
  m_statusFrame.ctrlStatus.lostData = 1;
  m_statusFrame.ctrlStatus.crcError = 1;
  m_statusFrame.ctrlStatus.recordNotFound = 1;
  m_statusFrame.ctrlStatus.deletedSector = 1;
  m_statusFrame.ctrlStatus.writeProtectError = 1;
  // Диска нет в приводе(инвертированное значение)
  m_statusFrame.ctrlStatus.notReady = 0;
  
  // Установить таймаут ожидания
  m_statusFrame.defaultTimeout = 0xE0;

  // Размер сектора по умолчанию 128 байт
  m_sectorSize = 128;

  // Тип по умолчанию ATR
  m_imageType = IMG_TYPE_ATR;

  // Количество секторов на треке
  m_secPerTrack = 0;

  // Текущий трек
  m_currentTrackPos = 0;
  // Текущий сектор
  m_currentSectorPos = 0;

  // Количесво сторон диска
  m_diskSideCount = 1;

  // Количество записей
  m_diskRecCount = 0;

  // Инициализация сектора
  sector[0] = 0;
  sectorPtr = sector;
  
  // Инициализация массива 
  m_disk[0].side = 0;
  m_disk[0].track = 0;
  m_disk[0].sector = 0;
  m_disk[0].offset = 0;
  m_disk[0].loadAddr = 0;
  m_disk[0].loadSize = 0;  
  
  m_diskPtr = m_disk;

  m_imageType = IMG_TYPE_NONE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::refresh() {
  dl.setValue(m_currentTrackPos);
  dl.refresh();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte vDrive::getMountedImgType() {
  return m_imageType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::mountXex(char* fileName) {
  init();
  // Мотор вкл
  m_statusFrame.drvStatus.motorRunning = 1;
  // Диск в приводе (инвертированное значение)
  m_statusFrame.ctrlStatus.notReady = 1;
  
  m_imageType = IMG_TYPE_XEX;
  
#ifdef DEBUG  
    DebugMsg::tryOpen("XEX", fileName);
#endif

  if (imageFile.isOpen()) {
    imageFile.close();
  }
  
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

    byte tData[4];

    unsigned short runAddr = 0;
    unsigned short initAddr = 0;
    unsigned int nAddr = 2;                    //skip header #FFFF
    unsigned int chunkId = 0;

    while(nAddr < m_imageSize) {
      imageFile.seekSet(nAddr);
      imageFile.read(tData, 4);

      unsigned short startAddr = tData[0] + (tData[1] * 256);

      if (startAddr == 0xFFFF) {
#ifdef DEBUG        
        LOG.println("----------------------------------");
        LOG.println("Addr Shit detected #FFFF!! Skiping");
        LOG.println("----------------------------------");
#endif
        nAddr += 2;
        imageFile.seekSet(nAddr);
        imageFile.read(tData,4);
        
        startAddr = tData[0] + (tData[1] * 256);
      }
      
      unsigned short endAddr = tData[2] + (tData[3] * 256);
      unsigned short blockSize = endAddr - startAddr + 1;

#ifdef DEBUG       
        DebugMsg::logBlockInfo(blockSize, nAddr, startAddr, endAddr);
#endif

      Diskette* d = m_diskPtr + chunkId;

      if (blockSize <= MAX_CHUNK_SIZE) {
        d->side = 1;
        d->track = chunkId;
        d->sector = 1;
        d->offset = nAddr+4;
        d->loadAddr = startAddr;
        d->loadSize = blockSize;
        chunkId++;
        
      } else {

        unsigned short chunkSize = blockSize;
        unsigned short nPos = nAddr+4;
        unsigned short aOffset = 0;

        while (chunkSize > MAX_CHUNK_SIZE) {
          d = m_diskPtr + chunkId;
          
          d->side = 1;
          d->track = chunkId;
          d->sector = 1;
          d->offset = nPos;
          d->loadAddr = startAddr + aOffset;
          d->loadSize = MAX_CHUNK_SIZE;
                    
          chunkSize -= MAX_CHUNK_SIZE;
          chunkId++;
          nPos += MAX_CHUNK_SIZE;
          aOffset += MAX_CHUNK_SIZE;
        }

        d = m_diskPtr + chunkId;
        
        d->side = 1;
        d->track = chunkId;
        d->sector = 1;
        d->offset = nPos;
        d->loadAddr = startAddr + aOffset;
        d->loadSize = chunkSize;
        
        chunkId++;
      }

      if (startAddr == 0x02e0) {
        imageFile.read(tData,4);
        runAddr = tData[0] + tData[1] * 256;
        initAddr = tData[2] + tData[3] * 256;
      }
  
      // 4 - skip start & end addrs
      nAddr += blockSize + 4;
    }

      m_diskRecCount = chunkId;

#ifdef DEBUG
    DebugMsg::dumpAllRecords(m_diskPtr, m_diskRecCount);
    DebugMsg::xexInfo(runAddr, initAddr);
#endif

  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::mountImage(char* imageName, byte imageType) {

  init();
  
  // Мотор вкл
  m_statusFrame.drvStatus.motorRunning = 1;
  // Диск в приводе (инвертированное значение)
  m_statusFrame.ctrlStatus.notReady = 1;
  
  m_imageType = imageType;
    
#ifdef DEBUG  
    DebugMsg::tryOpen("Image", imageName);
#endif

  if (imageFile.isOpen()) {
    imageFile.close();
  }
  
  //imageFile.open(imageName, O_RDONLY);
  imageFile.open(imageName, O_RDWR);

  if (!imageFile.isOpen()) {

#ifdef DEBUG
    LOG.println("ERROR");
#endif

    return;

  } else {

#ifdef DEBUG
    LOG.println("OK");
    LOG.println("Read file header");
#endif

    File f(imageName, O_RDONLY);
    m_imageSize = f.size() - 16;

    // SD
    if (m_imageSize > 0 && m_imageSize <= 92160) {
      m_secPerTrack = 18;
      m_diskSideCount = 1;

    // QD
    } else if (m_imageSize > 92160 && m_imageSize <= 133120) {
      m_secPerTrack = 26;
      m_diskSideCount = 1;

    // DD
    } else if (m_imageSize > 133120 && m_imageSize <= 183936) {
      m_secPerTrack = 18;
      m_diskSideCount = 2;

    // DD
    } else if (m_imageSize > 183936 && m_imageSize <= 368256) {
      m_secPerTrack = 36;
      m_diskSideCount = 2;
    }

    char header[16];
    imageFile.read(header, sizeof(header));
    
    memset(&atrHeader, 0, sizeof(atrHeader));
    atrHeader = (AtrHeader*)&header;
    m_sectorSize = atrHeader->secSize;

    int imSizeNoBoot = m_imageSize - (128*3);

    m_sectorCount = imSizeNoBoot/m_sectorSize + 3;

    m_trackCount = m_sectorCount / m_secPerTrack;

#ifdef DEBUG
    LOG.print("$");
    LOG.print(atrHeader->signature, HEX);
    LOG.println(" - Identification");
    LOG.print(atrHeader->diskSize);
    LOG.println(" - Size of disk image. The size is expressed in \"paragraphs\".");
    LOG.print(atrHeader->secSize);
    LOG.println(" - Sector size. 128 or 256 bytes per sector.");
    LOG.print(atrHeader->highPartSize);
    LOG.println(" - High part of size in paragraphs (added by REV 3.00)");

    LOG.println("Disk flags:");
    LOG.print(atrHeader->diskFlags.copyProtected);
    LOG.println(" - Bit 4: copy protected (has bad sectors)");
    LOG.print(atrHeader->diskFlags.writeProtected);
    LOG.println(" - Bit 5: write protected");
    LOG.print(atrHeader->firstBadSector);
    LOG.println(" - word which contains the number of the first bad sector");

    LOG.print("Disk size (without image header): ");
    LOG.print(m_imageSize);
    LOG.println(" Bytes");

    LOG.print("Disk size (without boot sectors): ");
    LOG.print(imSizeNoBoot);
    LOG.println(" Bytes");  

    LOG.print("Sector size: ");
    LOG.print(m_sectorSize);
    LOG.println(" Bytes");

    LOG.print("Sector count: ");
    LOG.println(m_sectorCount);

    LOG.print("Sector per Track: ");
    LOG.println(m_secPerTrack);

    LOG.print("Track count: ");
    LOG.println(m_trackCount);

    LOG.print("Disk has side: ");
    LOG.println(m_diskSideCount);

    LOG.println("");
#endif    
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned short vDrive::getSectorSize() {
  return m_sectorSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatusFrame* vDrive::getStatusFrame() {
  return &m_statusFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte* vDrive::getSectorData(int sectorPos) {
  // Текущий сектор
  m_currentSectorPos = sectorPos;
  // Текущий трек (псевдо расчёт)
  m_currentTrackPos = m_currentSectorPos/m_secPerTrack;

  // Первые 3 сектора не зависимо от размеров всего 128б это бут область!
  unsigned short sSec = 128;
  if (sectorPos > 3) {
    sSec = m_sectorSize;
  }
  int sPos = sSec * (sectorPos - 1);
  
  // Если размер сектора > 128
  if (m_sectorSize > 128 && sectorPos > 3) {
    sPos = sSec * (sectorPos - 1 - 3) + 128 *3;
  }
  if (m_imageType == IMG_TYPE_XEX) {
    byte* xldr = &xexLoader[0];
    memcpy(sector, xldr+sPos, 128);
    
  } else {
    imageFile.seekSet(sPos + 16);     //16 skip header
    imageFile.read(sector, sSec);
  }  
  return sectorPtr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::setSectorData(unsigned short sectorPos, byte* sectorData) {
  // Текущий сектор
  m_currentSectorPos = sectorPos;
  // Текущий трек (псевдо расчёт)
  m_currentTrackPos = m_currentSectorPos/m_secPerTrack;

  // Первые 3 сектора не зависимо от размеров всего 128б это бут область!
  unsigned short sSec = 128;
  if (sectorPos > 3) {
    sSec = m_sectorSize;
  }
  int sPos = sSec * (sectorPos - 1);
  
  // Если размер сектора > 128
  if (m_sectorSize > 128 && sectorPos > 3) {
    sPos = sSec * (sectorPos - 1 - 3) + 128 *3;
  }
  if (m_imageType == IMG_TYPE_XEX) {
#ifdef DEBUG
    LOG.println("ERROR: XEX FILE READ ONLY!");
#endif 
  } else {

#ifdef DEBUG
    LOG.print("Write sector size ");
    LOG.print(sSec);
    LOG.print(" to image at ");
    LOG.println(sPos + 16);

    byte* b = sectorData;
    for (int i=0; i < sSec; i++) {
      LOG.print(*b, HEX);
      LOG.print(" ");
      b++;
    }
    LOG.println(" ");
#endif 
    imageFile.seekSet(sPos + 16);     //16 skip header
    imageFile.write(sectorData, sSec);
  }  

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte vDrive::getCurrentTrack() {
  return m_currentTrackPos;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Diskette vDrive::getRecordInfo(unsigned int recId) {
  return m_diskPtr[recId];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void vDrive::dumpRecs() {
#ifdef DEBUG
    DebugMsg::dumpRecord(m_diskPtr);
    DebugMsg::dumpRecord(m_diskPtr+1);
    LOG.println(" ");
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte* vDrive::getRecordData(unsigned int recId) {
  Diskette d = m_diskPtr[recId];
  m_sectorSize = d.loadSize;

  m_currentSectorPos = recId;                     // TODO: Временное решение, нужно будет указывать реальный трек
  m_currentTrackPos = 0;                          // Ну как бэ все «сектора» на одном треке (впрочем хз) надо подумать ;)
  
#ifdef DEBUG
  LOG.print("Offset: +");
  LOG.println(d.offset, HEX);
#endif

  imageFile.seekSet(d.offset);
  imageFile.read(sector, d.loadSize);

  return (byte*)sector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int vDrive::getDiskRecCount() {
  return m_diskRecCount;
}
