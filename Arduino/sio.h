#ifndef SIO_h
#define SIO_h

#include <Arduino.h>
#import "vDrive.h"
#import "vRecorder.h"
#import "digitLed.h"

#import "emuSDrive.h"

#import "devices.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const byte FCMD_FORMAT         = 0x21;                //
const byte FCMD_FORMAT_MD      = 0x22;
const byte FCMD_POLL           = 0x3F;
const byte FCMD_PUT            = 0x50;                // Команда записать сектор
const byte FCMD_READ           = 0x52;
const byte FCMD_STATUS         = 0x53;                // Команда получить статус устройства
const byte FCMD_WRITE          = 0x57;

const byte FCMD_DEVPOOL3       = 0x40;                // Команда пулинга устройств

const byte FCMD_NOTIFY_RUN     = 0xfd;                // Команда информирования о запуске кода
const byte FCMD_CHUNK_DATA     = 0xfe;                // Команда получить данные чанка
const byte FCMD_CHUNK_INFO     = 0xff;                // Команда получить информацию о чанке

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const byte SIO_READY          = 1;                    // Готовность к приёму комманды
const byte SIO_WAIT_CMD_START = 2;                    // Ожидание начала команды
const byte SIO_READ_CMD       = 3;                    // Чтение команды
const byte SIO_READ_DATA      = 4;                    // Чтение данных

const byte SEND_ACK           = 0x41;                 // Отправить код данные получены
const byte SEND_NAK           = 0x4E;                 // Отправить код ошибка, повторить отправку
const byte SEND_COMPLETE      = 0x43;                 // Отправить код приём завершен
const byte SEND_ERR           = 0x45;                 // Отправить код ошибка чтения диска

const byte DELAY_ACK          = 5;                    // Задержка(Mc) при отправке ACK (иначе ATARI не успевает принять команду)
const byte DELAY_COMPLETE     = 5;                    // Задержка(Mc) при отправке COMPLETE

const unsigned long READ_CMD_TIMEOUT     = 500;       // Если в течение этого времени данные не переданы полностью,
                                                      // прекратить приём и перейти в режим ожидания

// Блок команды (со стороны ATARI)
struct CommandFrame {
  byte            devId;                              // Идентефикатор устройства
  byte            cmdId;                              // Код команды
  byte            aByte1;                             // Вспомогательный байт 1
  byte            aByte2;                             // Вспомогательный байт 2
  byte            crc;                                // Контрольная сумма фрейма
};

//struct BusPoll3 {
//  unsigned short  handlerSize;                        // +0x00: 2 байта: Размер заголовка
//  byte            devId;                              // +0x02: 1 байт: Идетентификатор устройства
//  byte            verId;                              // +0x03: 1 байт: Версия
//};

class SIO {
public:
  SIO();
  void            init(int drvId, char* mountFileName);  // Настройка параметров
  void            update();                           // Обработка статуса
  void            incomingByte();                     // Байт входящих данных
  
private:
  vDrive          m_vDrive1;                          // Виртуальный привод D1
  vDrive          m_vDrive2;                          // Виртуальный привод D2
  vDrive          m_vDrive3;                          // Виртуальный привод D3
  vDrive          m_vDrive4;                          // Виртуальный привод D4
  
  vRecorder       m_virtualRec;                       // Виртуальный магнитофон
  
  DigitLed*       m_dl;                               // Цифровой индикатор
 
  byte            m_sioState;                         // Текущее состояние SIO

  int             m_timeReceivedByte;                 // Время последнено принятого байта

  CommandFrame    m_cmdBuffer;                        // Буфер для приёма данных команды
  byte*           m_cmdBufferPtr;                     // Указатель на начала буфера
  byte            m_cmdBufferCrc;                     // Контрольная сумма буфера команды

  unsigned short  m_currentSector;                    // Текущий сектор (чтения/запись);
  unsigned short  m_currentSectorSize;                // Размер текущего сектора

  byte            m_dataBuffer[1024];                 // Буфер для получения данных cо стороны ATARI
  byte            *m_dataBufferPtr;                   // Указатель на буфер для получения данных cо стороны ATARI
  byte            m_dataBufferCrc;                    // Контрольная сумма буфера данных

  sDrive*         m_sDrive;                           // Эмуляция для прикидки SDrive
  
//  BusPoll3        busPoll3;                           // Ответ для пулинга устройств
  
//-------------------------------------------------------------------------------------------------------------------
  vDrive*         getVDriveById(byte drvId);          // Получить указатель на текущий виртуальный привод про ID
  void            updateIndicator();                  // Обновление идикации для последнего активного виртуального привода
  
  void            checkCmdPinHigh();                  // Проверка состояния пина Command (HIGH)
  void            checkCmdPinLow();                   // Проверка состояния пина Command (LOW)
  void            clearCmdBuffer();                   // Подготовка буфера комманды к приёму данных
  void            checkReadCmdComplete();             // Проверка окончания приёма данных команды
  void            checkReadDataComplete();            // Проверка окончания приёма данных

  
  void            changeState(byte state);            // Изменить текущее состояние SIO
  void            logReceivedCmdData();               // Логирование данных пакета (для отладки)

  void            calcCmdDataCrc();                   // Расчёт контрольной суммы блока команды
  void            calcDataCrc();                      // Расчёт контрольной суммы блока данных
  byte            calcCrc(byte* chunk, unsigned short len);    // Расчёт контрольной суммы блока данных

  void            fCmdProcessing();                   // Обработка поступившей команды для устройств типа дисковод
  void            sCmdProcessing();                   // Обработка поступившей команды для устройств типа SDrive
  void            sendDevStatus();                    // Вернуть статус запрошенного устройства

  void            sendDevSector();                    // Отправить ATARI сектор данных для запрошенного устройства
  void            recieveDevSector();                 // Подготовится к приёму сектора для записи

  void            sendDevChunkInfo();                 // Отправить ATARI информацию о чанке для запрошенного устройства
  void            sendDevChunkData();                 // Отправить ATARI данные чанка для запрошенного устройства
  void            sendNotifyRun();                    // Информирование о том, что ATARI запускает код
  
  void            sendBytes(unsigned short sectorSize, byte* sectorData); // Отправить блок данных ATARI

  void            sendSDriveIdent();                  // Отправить ATARI информацию об устройстве SDrive
  void            sendSDriveNop();                    // Отправить ATARI ничего :) Временная заглушка нереализованных функций SDrive (тупо skip)
  void            sendSDriveParams();                 // Отправить ATARI параметры устройства SDrive
  void            sendSDriveZero(byte count);         // Отправить ATARI заглушку из нулей указанных в count
  void            sendSDriveNext20();                 // Отправить ATARI список файлов SDrive

  byte            getFileType(char* fileName);        // Вернуть тип файла
};

#endif
