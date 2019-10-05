#ifndef EMUSDRIVE_h
#define EMUSDRIVE_h

#include <Arduino.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const byte SCMD_GET_STATUS   = 0x53;
const byte SCMD_GET20        = 0xC0;
const byte SCMD_IDENT        = 0xE0;
const byte SCMD_INIT         = 0xE1;
const byte SCMD_CHDIR_VDN    = 0xE3;
const byte SCMD_GET_ENTRIES  = 0xEB;
const byte SCMD_SWAP_VDN     = 0xEE;
const byte SCMD_GETPARAMS    = 0xEF;
const byte SCMD_MOUNT_D0     = 0xF0;
const byte SCMD_MOUNT_D1     = 0xF1;
const byte SCMD_MOUNT_D2     = 0xF2;
const byte SCMD_MOUNT_D3     = 0xF3;
const byte SCMD_MOUNT_D4     = 0xF4;
const byte SCMD_CHDIR_UP     = 0xFD;
const byte SCMD_CHROOT       = 0xFE;
const byte SCMD_CHDIR        = 0xFF;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Параметры данны устройства
struct SDParam {
  byte param1;                                                  // ???
  byte param2;                                                  // ???
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Структура списка файлов
struct FileEntry {
  char name[11];
  bool isDirectory;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class sDrive {
public:
  sDrive();
  char*           getIdent();                                   // Получить указатель на блок данных для идентификации устройства
  SDParam*        getSDParam();                                 // Получить указатель на блок данных параметров устройства
private:
  SDParam         m_SDParam;

  FileEntry       fe;
};

#endif
