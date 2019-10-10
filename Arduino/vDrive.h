#ifndef VDRIVE_h
#define VDRIVE_h

#include <Arduino.h>
#include "SdFat.h"

#include "digitLed.h"

#import "devices.h"
#import "imgTypes.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const unsigned short MAX_DISK_RECORDS = 200;          // Сколько максимальных записей (секторов/чанком) может быть
const unsigned short MAX_CHUNK_SIZE = 1024;           // Максимальный размер чанка

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Состояние привода
struct DriveStatus {
  byte            unused_0                : 1;        // bit 0 Не задействован
  byte            unused_1                : 1;        // bit 1 Не задействован
  byte            writeError              : 1;        // bit 2 Неудачное выполнение команды PUT
  byte            writeProtected          : 1;        // bit 3 Ошибка из-за защиты от записи
  byte            motorRunning            : 1;        // bit 4 Мотор вращается
  byte            unused_5                : 1;        // bit 5 Не задействован
  byte            unused_6                : 1;        // bit 6 Не задействован
  byte            unused_7                : 1;        // bit 7 Не задействован
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Состояние контроллера дисковода гибких дисков (инвертировано из FDC)
struct ControllerStatus {
  byte            busy                    : 1;        // bit 0 Устройство занято
  byte            dataPending             : 1;        // bit 1 Ожидание данных
  byte            lostData                : 1;        // bit 2 Данные утеряны
  byte            crcError                : 1;        // bit 3 Ошибка контрольной суммы
  byte            recordNotFound          : 1;        // bit 4 Запись не найдена (ошибка сектора)
  byte            deletedSector           : 1;        // bit 5 Удалённый сектор (сектор отмечен как удаленный в заголовке сектора)
  byte            writeProtectError       : 1;        // bit 6 Ошибка защита от записи
  byte            notReady                : 1;        // bit 7 Нет диска в приводе
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Блок ответа статуса $10 $FF, $E0, $00
struct StatusFrame {
  DriveStatus       drvStatus;                        // Статус устройства
  ControllerStatus  ctrlStatus;                       // Статус контроллера
  byte              defaultTimeout;                   // $E0 Время ожидания по умолчанию ($E0 = 224 vertical blanks)
  byte              unused;                           // $00 Не задействован
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Блок флагов образа диска
struct DiskFlags  {
  byte              unused_0              : 1;        // bit 0 Не задействован
  byte              unused_1              : 1;        // bit 1 Не задействован
  byte              unused_2              : 1;        // bit 2 Не задействован
  byte              unused_3              : 1;        // bit 3 Не задействован
  byte              copyProtected         : 1;        // bit 4 Защищён от копирования (имеются сектора с ошибками)
  byte              writeProtected        : 1;        // bit 5 Защищён от записи
  byte              unused_6              : 1;        // bit 6 Не задействован
  byte              unused_7              : 1;        // bit 7 Не задействован
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Заголовок дискеты в формате Atr
struct AtrHeader  {
  unsigned short  signature;                          // +0x00: 2 байта: Идентификатор формата дискеты 0x0296
  unsigned short  diskSize;                           // +0x02: 2 байта: Размер образа диска. Размер указывается в «параграфах»
  unsigned short  secSize;                            // +0x04: 2 байта: Размер сектора. 0x80 - 128 байт, 0x100 — 256байт
  unsigned short  highPartSize;                       // +0x06: 2 байта: Размер старшей части в параграфах. Добавлено в rev.3.00
  DiskFlags       diskFlags;                          // +0x08: 1 байт: Флаги образа диска
  unsigned short  firstBadSector;                     // +0x09: 2 байта: Номер первого сектора с ошибками
  byte            unused_0;                           // +0x0b: 1 байт: Не задействован
  byte            unused_1;                           // +0x0c: 1 байт: Не задействован
  byte            unused_2;                           // +0x0d: 1 байт: Не задействован
  byte            unused_3;                           // +0x0e: 1 байт: Не задействован
  byte            unused_4;                           // +0x0f: 1 байт: Не задействован
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// Структура данных для чанков
struct Diskette {
  byte side;                                          // Номер стороны диска
  byte track;                                         // Номер трека стороны
  unsigned short  sector;                             // Номер сектора трека
  unsigned int    offset;                             // Смещение относительно файла с образом
  unsigned short  loadAddr;                           // Адрес загрузки (только для чанков)
  unsigned short  loadSize;                           // Размер загружаемых данных в байтах (сектор/чанк)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class vDrive {
public:
  vDrive();
  void            init();                             // Инициализация
  void            refresh();                          // Для обновление индикации
  void            mountImage(char* imageName, byte imageType); // Подключить образ дискеты, типа
  void            mountXex(char* fileName);           // Подключить исполняемый файл как образ

  byte            getMountedImgType();                // Получить тип примонтированного диска
  
  unsigned short  getSectorSize();                    // Получить размер сектора
  StatusFrame*    getStatusFrame();                   // Получить указатель на блок данных статуса устройства
  byte*           getSectorData(int sectorPos);       // Получить указатель на блок данных сектора

  byte            getCurrentTrack();                  // Получить текущий трек (условно)

  Diskette        getRecordInfo(unsigned int recId);  // Получить данные о записи (сектор/чанк)
  byte*           getRecordData(unsigned int recId);  // Получить данные записи (сектор/чанк)

  unsigned int    getDiskRecCount();                  // Получить количество записей (секторов / чанков) в диске
  
  void            dumpRecs();


private:
  StatusFrame     m_statusFrame;                      // Блок данных (4 байта) ответа статуса устройства

  unsigned short  m_sectorSize;                       // Размер сектора
  unsigned short  m_sectorCount;                      // Количество секторов
  unsigned short  m_secPerTrack;                      // Количество секторов на дорожку (условно)
  unsigned short  m_trackCount;                       // Количество треков (условно)
  byte            m_diskSideCount;                    // Количество сторон у дискеты

  int             m_currentSectorPos;                 // Номер текущего сектора
  int             m_currentTrackPos;                  // Номер текущего трека
  
  int             m_imageSize;                        // Размер образа
  byte            m_imageType;                        // Тип образа

  SdFile          imageFile;                          // Объект для чтения/записи образа диска
  AtrHeader*      atrHeader;                          // Заголовок примонтированной дискеты формата ATR

  Diskette        m_disk[MAX_DISK_RECORDS];           // Список секторов на дискете
  Diskette        *m_diskPtr;                         // Указатель на список секторов на дискете
  
  unsigned int    m_diskRecCount;                     // Количество записей (секторов / чанков) в диске
  
  byte            sector[1024];                       // Буфер для чтения данных (сектор / чанк)
  byte            *sectorPtr;                         // Указатель на буфер для чтения данных (сектор / чанк)

  DigitLed        dl;                                 // Цифровой сдвоенный сегмисегментный идикатор
  
//-------------------------------------------------------------------------------------------------------------------
  void            dumpRecord(unsigned int recId);
  
};

#endif
