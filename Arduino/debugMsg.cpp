#include "config.h"
#include "debugMsg.h"

#include "vDrive.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug Message — отладочная информация
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DebugMsg::DebugMsg() {
}

void DebugMsg::logSioState(byte id) {
  char *sioStateMsg[] = {
    "Ready",
    "Wait for command start",
    "Read command",
    "Read data",
    "Wait for command end"
  };
  
  LOG.print("State: ");
  LOG.println(sioStateMsg[id]);
}

void DebugMsg::logReceivedCmdData(CommandFrame cf, byte crc) {
  LOG.println("--------------------------------------");
  LOG.println(">Received data:");
  LOG.print(DebugMsg::getHexZeroLeading(cf.devId));
  LOG.print(" ");
  LOG.print(DebugMsg::getHexZeroLeading(cf.cmdId));
  LOG.print(" ");
  LOG.print(DebugMsg::getHexZeroLeading(cf.aByte1));
  LOG.print(" ");
  LOG.print(DebugMsg::getHexZeroLeading(cf.aByte2));
  LOG.print(" ");
  LOG.println(DebugMsg::getHexZeroLeading(cf.crc));

  LOG.print("CRC: Calculated: $");
  LOG.print(crc, HEX);
  LOG.print(",  Received: $");
  LOG.print(cf.crc, HEX);
  LOG.print(",  Status:");
  if (crc == cf.crc) {
    LOG.println("OK");
  } else {
    LOG.println("ERROR");
  }
 
  LOG.print("Device:  $");
  LOG.print(cf.devId, HEX);
  LOG.print(" (\"");
  LOG.print(DebugMsg::getDevName(cf.devId));
  LOG.println("\")");

  LOG.print("Command: $");
  LOG.print(cf.cmdId, HEX);
  LOG.print(" (\"");
  LOG.print(DebugMsg::getCmdName(cf.cmdId, cf.devId)); 
  LOG.println("\")");

  if (cf.devId >= DEV_D1 && cf.devId <= DEV_DO) {
    switch(cf.cmdId) {
      case FCMD_READ:
        LOG.println(" ");      
        LOG.print("[S] Requested data for sector: ");
        LOG.println(cf.aByte2*256 + cf.aByte1);
        break;
        
      case FCMD_CHUNK_INFO:
        LOG.println(" ");
        LOG.print("[I] Requested info for chunk: ");
        LOG.println(cf.aByte2*256 + cf.aByte1);
        break;
  
      case FCMD_CHUNK_DATA:
        LOG.println(" ");
        LOG.print("[C] Requested data for chunk: ");
        LOG.println(cf.aByte2*256 + cf.aByte1);
        break;
    }
   } else if (cf.devId == DEV_SDRIVE) {
    
   }
  
  LOG.println("");
}

void DebugMsg::logSendDev(String sendResult, byte crc) {
  LOG.println("<Send data:");
  LOG.println(sendResult);

  LOG.print("<Send data CRC: $");
  LOG.println(crc, HEX);
  LOG.println("");
}

//---------------------------------------------------------------------
String DebugMsg::getHexString(byte b, unsigned int i) {
  String result = "";
  if (i>0) {
        result += " ";
        if (i%16 == 0) {
          result += "\n";
        }
    }

    if (b < 16) { result += "0"; };
    result += String(b, HEX);

    return result;
}

String DebugMsg::getHexZeroLeading(byte b) {
  String result = "";
  if (b < 16) { result += "0"; };
  result += String(b, HEX);
  return result;
}


char* DebugMsg::getDevName(unsigned int id) {
  char *dis = "Unknown";
  for(int i = 0; i < sizeof(devNames)/sizeof(HashStruct); ++i) {   
    if (id == devNames[i].id) {
      dis = devNames[i].dis;
      break;
    }    
  }
  return dis;
}

char* DebugMsg::getCmdName(unsigned int id, unsigned int devId) {
  if (devId == DEV_SDRIVE) {
    return getSCmdName(id);
  };
  return getFCmdName(id);
}

char* DebugMsg::getFCmdName(unsigned int id) {
  char *dis = "Unknown";
  for(int i = 0; i < sizeof(floppyCommands)/sizeof(HashStruct); ++i) {   
    if (id == floppyCommands[i].id) {
      dis = floppyCommands[i].dis;
      break;
    }    
  }
  return dis;
}

char* DebugMsg::getSCmdName(unsigned int id) {
  char *dis = "Unknown";
  for(int i = 0; i < sizeof(sDriveCommands)/sizeof(HashStruct); ++i) {   
    if (id == sDriveCommands[i].id) {
      dis = sDriveCommands[i].dis;
      break;
    }    
  }
  return dis;
}

void DebugMsg::dumpAllRecords(Diskette* dp, unsigned int recCount) {
    for (unsigned int i=0; i < recCount; i++) {      
      LOG.print("Record:");
      LOG.print(i);

      Diskette* d = dp + i;
      DebugMsg::dumpRecord(d);
  }
}

void DebugMsg::dumpRecord(Diskette* d) {

  LOG.print(" | Side: ");
  LOG.print(d->side);

  LOG.print(" | Track: ");
  LOG.print(d->track);

  LOG.print(" | Sector: ");
  LOG.print(d->sector);

  LOG.print(" | Offset: +$");
  LOG.print(d->offset, HEX);
  LOG.print('(');
  LOG.print(d->offset);
  LOG.print(')');
  
  LOG.print(" | Load Addr: $");
  LOG.print(d->loadAddr, HEX);
  
  LOG.print(" | Load Size: $");
  LOG.print(d->loadSize, HEX);
  LOG.print('(');
  LOG.print(d->loadSize);
  LOG.println(')');
}

void DebugMsg::xexInfo(unsigned short runAddr, unsigned short initAddr) {
  LOG.println(' ');
  LOG.print("Run Addr: $");
  LOG.println(runAddr, HEX);
  LOG.print("Init Addr: $");
  LOG.println(initAddr, HEX);
  LOG.println(' ');
}


void DebugMsg::tryOpen(char* fileType, char* fileName) {
  LOG.print("Try to Open ");
  LOG.print(fileType);
  LOG.print(" file \"");
  LOG.print(fileName);
  LOG.print("\" - ");
}

void DebugMsg::logBlockInfo(unsigned short blockSize, unsigned int nAddr, unsigned short startAddr, unsigned short endAddr) {
  LOG.print("Block size: $");
  LOG.print(blockSize, HEX);
  LOG.print('(');
  LOG.print(blockSize);
  LOG.println(')');
  
  LOG.print("+$");      
  LOG.print(nAddr, HEX);
  LOG.print(" : $");
  LOG.print(startAddr, HEX);
  LOG.print('(');
  LOG.print(startAddr);
  LOG.print(')');
  LOG.println(" - Start Address");

  LOG.print("+$");
  LOG.print(nAddr+2, HEX);
  LOG.print(" : $");
  LOG.print(endAddr, HEX);
  LOG.print('(');
  LOG.print(endAddr);
  LOG.print(')');
  LOG.println(" - End Address"); 
}
