#ifndef DEBUGMSG_h
#define DEBUGMSG_h

#include <Arduino.h>

#include "sio.h"
#include "emuSDrive.h"

typedef struct { 
  byte id;
  char* dis;
} HashStruct;

const HashStruct devNames[] {
    {DEV_D1,    "Floppy Drive D1"},
    {DEV_D2,    "Floppy Drive D2"},
    {DEV_D3,    "Floppy Drive D3"},
    {DEV_D4,    "Floppy Drive D4"},
    {DEV_D5,    "Floppy Drive D5"},
    {DEV_D6,    "Floppy Drive D6"},
    {DEV_D7,    "Floppy Drive D7"},
    {DEV_D8,    "Floppy Drive D8"},
    {DEV_D9,    "Floppy Drive D9"},
    {DEV_DJ,    "Floppy Drive J"},
    {DEV_DK,    "Floppy Drive K"},
    {DEV_DL,    "Floppy Drive L"},
    {DEV_DM,    "Floppy Drive M"},
    {DEV_DN,    "Floppy Drive N"},
    {DEV_DO,    "Floppy Drive O"},

    {DEV_P1,    "Printer P1"},
    {DEV_P2,    "Printer P2"},
    {DEV_P3,    "Printer P3"},
    {DEV_P4,    "Printer P4"},

    {DEV_BUS,   "Device Bus"},

    {DEV_R1,    "RS232 R1"},
    {DEV_R2,    "RS232 R2"},
    {DEV_R3,    "RS232 R3"},
    {DEV_R4,    "RS232 R4"},

    {DEV_C,     "Cassette C"},
    
    {DEV_SDRIVE,"SDrive"},
};

const HashStruct floppyCommands[] {
    {0x20,            "Format Auto"},
    {FCMD_FORMAT,     "Format Drive"},
    {FCMD_FORMAT_MD,  "Format Medium Density"},
    {0x23,            "Service"},
    {0x24,            "Diagnostic"},
    {FCMD_POLL,       "Get high-speed-index"},
    {0x40,            "Bus poll"},
    {0x41,            "Add/Remove Command"},
    {0x44,            "Configure Drive"},
    {0x48,            "Happy Command"},
    {0x4b,            "Slow/Fast Config"},
    {0x4c,            "Jump without Message"},
    {0x4d,            "Jump with Message"},
    {0x4e,            "Read PERCOM Block"},
    {0x4f,            "Write PERCOM Block"},
    {FCMD_PUT,        "Write Sector"},
    {0x51,            "Quit"},
    {FCMD_READ,       "Read Sector"},
    {FCMD_STATUS,     "Get Status"},
    {0x54,            "Read Memory or  Get drive variables"},
    {0x55,            "Motor ON"},
    {0x56,            "Verify Sector"},
    {FCMD_WRITE,      "Write Sector with Verify"},
    {0x60,            "Write Track"},
    {0x62,            "Read Track"},
    {0x66,            "Format Disk with Special Sector-Skew"},
    {0x68,            "Get SIO Length"},
    {0x69,            "Get SIO Routine"},
    {0x74,            "Return prepared buffer"},
    {0x75,            "Upload & execute code"},
    {FCMD_NOTIFY_RUN, "Notify run"},
    {FCMD_CHUNK_DATA, "Get chunk data"},
    {FCMD_CHUNK_INFO, "Get chunk info"}
};

const HashStruct sDriveCommands[] {
    {SCMD_GET_STATUS,   "Get Status vD0"},
    {SCMD_GET20,        "Get next 20 files"},
    {SCMD_IDENT,        "Identificate device"},
    {SCMD_INIT,         "Init"},
    {SCMD_CHDIR_VDN,    "Change dir vdn"},
    {SCMD_GET_ENTRIES,  "Get entries"},
    {SCMD_SWAP_VDN,     "Swap vdn"},
    {SCMD_GETPARAMS,    "Get params"},
    {SCMD_MOUNT_D0,     "Mount D0"},
    {SCMD_MOUNT_D1,     "Mount D1"},
    {SCMD_MOUNT_D2,     "Mount D2"},
    {SCMD_MOUNT_D3,     "Mount D3"},
    {SCMD_MOUNT_D4,     "Mount D4"},
    {SCMD_CHDIR_UP,     "Change dir up"},
    {SCMD_CHROOT,       "Change root"},
    {SCMD_CHDIR,        "Change dir"}
};

class DebugMsg {
public:
  DebugMsg();
  static void     logSioState(byte id);                           // Вывести в лог состояние статуса
  static void     logReceivedCmdData(CommandFrame cf, byte crc);  // Вывести в лог данные команды
  static void     logSendDev(String sendResult, byte crc);        // Вывести в лог данные отправки

  static void     xexInfo(unsigned short runAddr, unsigned short initAddr); // Вывести в лог информацию о XEX
  static void     dumpAllRecords(Diskette* dp, unsigned int recCount);  // Вывести в лог дамп всех записей
  static void     dumpRecord(Diskette* d);                        // Вывести в лог дамп записи
  static void     tryOpen(char* fileType, char* fileName);        // Вывести в лог отрыть файл

  static void     logBlockInfo(unsigned short blockSize, unsigned int nAddr, unsigned short startAddr, unsigned short endAddr); // Вывести в лог информацию блока (XEX)
  
  static String   getHexString(byte b, unsigned int i);           // Вернуть строку с шестнадцатиричным числом
  static String   getHexZeroLeading(byte b);                      // Вернуть строку HEX с дополеным нулём
private:
  static char*    getDevName(unsigned int id);                    // Получить название устройства по коду из списка
  static char*    getCmdName(unsigned int id, unsigned int devId);  // Получить название команды для устройства по коду из списка
  static char*    getFCmdName(unsigned int id);                   // Получить название команды для дисковода
  static char*    getSCmdName(unsigned int id);                   // Получить название команды для SDrive
};

#endif
