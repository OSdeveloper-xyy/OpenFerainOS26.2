#ifndef      BASICSERVER_H
#define      BASICSERVER_H
/*     Include Head files      */
#include <stdint.h>
#include "driver/i2s.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include <SPI.h>
#include <FFat.h>
#include <SD.h>
#include <FS.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "Audio.h"
//#include <Adafruit_SPIFlash.h>
//#include <SdFat_Adafruit_Fork.h>
#include "define.h" 
typedef bool StateFlag;
StateFlag SD_Flag = false,Flash_Flag = false,UDisk_Flag = false;
/*        Inf Define           */
#if RGB_SWITCH == 1
#define     NUMPIXELS      1
Adafruit_NeoPixel rgb(NUMPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
#endif

#if CODEC_SWITCH == 1
Audio audio;
int AudioSetStatus(bool Status){
  #if CODEC_SWITCH == 1
  if(Status){
    if(audio.isRunning()){
      Serial.println("[Codec] Codec is play!");
    }else{
      if(audio.pauseResume())Serial.println("[Codec] Codec set play ok");
      else Serial.println("[Codec] Codec set play failed");
    }
  }else{
    if(!audio.isRunning()){
      Serial.println("[Codec] Codec is pause!");
    }else{
      if(audio.pauseResume())Serial.println("[Codec] Codec set pause ok");
      else Serial.println("[Codec] Codec set pause failed");
    }
  }
  return true;
  #endif
  return false;
}
#if CODEC_TYPE == TYPE_ES8311
#include "es8311.h"
ES8311 es;
#endif
#endif

#if LTE_SWITCH == 1
HardwareSerial AT_4G(1);
#endif

#if USBHOST_SWITCH == 1
#include <EspUsbHost.h>
EspUsbHost usb;
#endif

#if SCREEN_SWITCH == 1
TwoWire ScreenI2C(1);
#endif

#if GNSS_SWITCH == 1
#define GNSS_TYPE          DXCT511N
#endif

#if FLASH_SWITCH == 1
/*Adafruit_FlashTransport_SPI flashTransport(CS_FLASH,SPI);
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;*/
#endif

const char* wifiname  =    "FerainESP32" ;
const char* blename   =    "FerainESPBLE";
int USBINIT(){
  #if USBHOST_SWITCH == 1
  if(!usb.begin())return false;
  return true;
  #endif
  return false;
}void SetInt(uint8_t IntNum,void(*func)(void)){
  detachInterrupt(digitalPinToInterrupt(IntNum));
  pinMode(IntNum,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IntNum),func,FALLING);
  return;
}
struct TouchStruct{
 uint8_t BusyFlag = false;
 uint32_t x;
 uint32_t y;
 uint32_t width;
 uint32_t length;
};
typedef struct TouchStruct TouchStruct;
int FileOpen(String Path,fs::File &f){
  if(Path.substring(0,4) == "Sys/"){
    Path = Path.substring(3);
    String Dir = Path;
    int DirDepth = Dir.indexOf('/',1);
    while(DirDepth != -1){
      FFat.mkdir(Dir.substring(0,DirDepth));
      DirDepth = Dir.indexOf('/',DirDepth + 1);
    }
    f = FFat.open(Path.c_str(),"r+");
    if(!f){
      f = FFat.open(Path.c_str(),FILE_WRITE);
      if(!f){
        Serial.println("Chip Flash Create fail");
        return false;
      }
    }
  }
  /*else if(Path.substring(0,6) == "Flash/"){
    if(Flash_Flag == false){
      Serial.println("No SPI Flash!");
      return false;
    }
    Path = Path.substring(5);
    f = fatfs.open(Path.c_str(),"r+");
    if(!f){
      f = fatfs.open(Path.c_str(),FILE_WRITE);
      if(!f){
        Serial.println("SPI Flash Create fail");
        return false;
      }
    }
  }*/
  else if(Path.substring(0,3) == "SD/"){
    if(SD_Flag == false){
      Serial.println("No SD Card!");
      return false;
    }
    Path = Path.substring(2);
    String Dir = Path;
    int DirDepth = Dir.indexOf('/',1);
    while(DirDepth != -1){
      SD.mkdir(Dir.substring(0,DirDepth));
      DirDepth = Dir.indexOf('/',DirDepth + 1);
    }
    f = SD.open(Path.c_str(),"r+");
    if(!f){
      f = SD.open(Path.c_str(),FILE_WRITE);
      if(!f){
        Serial.println("SD Create fail");
        return false;
      }
    }
  }
  /*else if(Path.substring(0,6) == "UDisk/"){
      Path = Path.substring(5);
  }*/
  return true;
}
struct LTEOperate{
  String TempPath,PINGPath,HTTPPath,HTTPSPath;
  int INIT(){
    #if LTE_SWITCH == 1
    AT_4G.begin(115200,SERIAL_8N1,LTE_RX,LTE_TX);
    #if LTE_TYPE == DX_CT511
    AT_4G.setTimeout(5000);
    AT_4G.println("ATE0");
    delay(200);
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    AT_4G.println("AT");
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("AT failed");
      return false;
    }
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif
    File f;
    if(FileOpen("Sys/Temp/LTE.tmp",f) == false){
      Serial.println("[LTE] Temp File Can Not Open!");
      TempPath = "Sys/LTE/temp.tmp";
      return false;
    }else{
      f.seek(0);
      TempPath = f.readStringUntil('\n');
      Serial.println(TempPath.c_str());
      Serial.println("[LTE] Temp File Path Read!");
      if(TempPath == ""){
        Serial.println("[LTE] Temp File Frist Create");
        f.seek(0);
        f.print("Sys/LTE/temp.tmp\n");
        TempPath = "Sys/LTE/temp.tmp";
        Serial.println("[LTE] Frist Create OK");
      }
    }
    f.close();
    File tmpf;
    Serial.println("[LTE] Every Part Temp File Check");
    FileOpen(TempPath,tmpf);
    tmpf.seek(0);
    String Data = tmpf.readStringUntil('\n');
    if(Data == ""){
      Serial.println("[LTE] Every Part Temp File First Creat");
      PINGPath  = "Sys/LTE/ping.tmp";
      HTTPPath  = "Sys/LTE/http.tmp";
      HTTPSPath = "Sys/LTE/https.tmp";
      tmpf.print("PING/TEMP:");
      tmpf.println(PINGPath);
      tmpf.print("HTTP/TEMP:");
      tmpf.println(HTTPPath);
      tmpf.print("HTTPS/TEMP:");
      tmpf.println(HTTPSPath);
    }else{
      while(Data.substring(0,10) != "PING/TEMP:"){
        if(Data == ""){
          PINGPath  = "Sys/LTE/ping.tmp";
          tmpf.print("PING/TEMP:");
          tmpf.println(PINGPath);
          break;
        }
        Data = tmpf.readStringUntil('\n');
      }
      PINGPath = Data.substring(10);
      tmpf.seek(0);
      Data = tmpf.readStringUntil('\n');
      while(Data.substring(0,10) != "HTTP/TEMP:"){
        if(Data == ""){
          HTTPPath  = "Sys/LTE/http.tmp";
          tmpf.print("HTTP/TEMP:");
          tmpf.println(HTTPPath);
          break;
        }
        Data = tmpf.readStringUntil('\n');
      }
      HTTPPath = Data.substring(10);
      tmpf.seek(0);
      Data = tmpf.readStringUntil('\n');
      while(Data.substring(0,11) != "HTTPS/TEMP:"){
        if(Data == ""){
          HTTPSPath  = "Sys/LTE/https.tmp";
          tmpf.print("HTTPS/TEMP:");
          tmpf.println(HTTPSPath);
          break;
        }
        Data = tmpf.readStringUntil('\n');
      }
      HTTPSPath  = Data.substring(11);
    }
    Serial.println("[LTE] Every Part Temp File Check OK");
    tmpf.close();
    return true;
    #endif
    return false;
  }int NetSwitch(uint8_t Switch){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
        AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    if(Switch == true){
      AT_4G.println("AT+NETOPEN");
      AT_4G.readStringUntil('\n');
      if(AT_4G.readStringUntil('\n') != "OK"){
        Serial.println("OK false");
        return false;
      }
      AT_4G.readStringUntil('\n');
      if(AT_4G.readStringUntil('\n') != "+NETOPEN:SUCCESS"){
        Serial.println("NETOPEN false");
        return false;
      }
    }else if(Switch == false){
      AT_4G.println("AT+NETCLOSE");
      AT_4G.readStringUntil('\n');
      if(AT_4G.readStringUntil('\n') != "OK"){
        Serial.println("OK false");
        return false;
      }
      AT_4G.readStringUntil('\n');
      if(AT_4G.readStringUntil('\n') != "+NETCLOSE:SUCCESS"){
        Serial.println("NETCLOSE false");
        return false;
      }
    }
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }int GetTime(){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    AT_4G.println("AT+CCLK?");
    AT_4G.readStringUntil('\n');
    String InData = AT_4G.readStringUntil('\n');
    //......
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return false;
    }
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }int GetStrength(){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    AT_4G.println("AT+CSQ");
    AT_4G.readStringUntil('\n');
    String InData = AT_4G.readStringUntil('\n');
    //......
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return false;
    }
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }int SetAPN(uint8_t Cid,uint8_t ContextType,String APNname,String UserName,String PassWord){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    String OutData = "AT+QICSGP=";
    OutData += String(Cid);
    OutData += ",";
    OutData += String(ContextType);
    OutData += ",\"";
    OutData += APNname;
    OutData += "\",\"";
    OutData += UserName ;
    OutData += "\",\"";
    OutData += PassWord;
    OutData += "\"";
    AT_4G.println(OutData);
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return false;
    }
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }String GetUrlIP(String URL){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    String OutData = "AT+MDNSGIP=";
    OutData += URL;
    AT_4G.println(OutData);
    AT_4G.readStringUntil('\n');
    String InData = AT_4G.readStringUntil('\n');
    if(InData.substring(0,9) != "+MDNSGIP:"){
      Serial.println("No real IP or true url!");
      return "FALSE";
    }
    InData = InData.substring(URL.length() + 10);
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return "FALSE";
    }
    return InData;
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    #endif
    return "FALSE";
  }int PING(String Addr,uint8_t AddrType,uint8_t PingNum = 4,uint16_t PacketSize = 32,uint8_t WaitTime = 3){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    String OutData = "AT+MPING=";
    OutData += Addr;
    OutData += ",";
    OutData += String(AddrType);
    OutData += ",";
    OutData += String(PingNum);
    OutData += ",";
    OutData += String(PacketSize);
    OutData += ",";
    OutData += String(WaitTime);
    AT_4G.println(OutData);
    AT_4G.readStringUntil('\n');
    //......
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }int HTTP(String WebLink){
    #if LTE_SWITCH == 1

    #if LTE_TYPE == DX_CT511
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    //SEND......
    File tmpf;
    FileOpen(HTTPPath,tmpf);
    tmpf.seek(0);
    String Data = AT_4G.readStringUntil('\n');
    while(Data != ""){
      tmpf.println(Data);
      Data = AT_4G.readStringUntil('\n');
    }
    tmpf.close();
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }int HTTPS(String WebLink){
    #if LTE_SWITCH == 1

    #if LTE_TYPE == DX_CT511
    while (AT_4G.available()){
      AT_4G.read(); 
    }

    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    return true;
    #endif
    return false;
  }String GetIccid(){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    AT_4G.println("AT+ICCID");
    AT_4G.readStringUntil('\n');
    String InData = AT_4G.readStringUntil('\n');
    if(InData.substring(0,7) != "+ICCID:"){
      Serial.println("Na a true iccid!");
      return "FALSE";
    }
    InData = InData.substring(7);
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return "FALSE";
    }
    return InData;
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    #endif
    return "FALSE";
  }String GetImei(){
    #if LTE_SWITCH == 1
    while (AT_4G.available()){
      AT_4G.read(); 
    }
    #if LTE_TYPE == DX_CT511
    AT_4G.println("ATI");
    AT_4G.readStringUntil('\n');
    AT_4G.readStringUntil('\n');
    AT_4G.readStringUntil('\n');
    AT_4G.readStringUntil('\n');
    String InData = AT_4G.readStringUntil('\n');
    if(InData.substring(0,6) != "IMEI: "){
      Serial.println("Na a true imei!");
      return "FALSE";
    }
    InData = InData.substring(6);
    AT_4G.readStringUntil('\n');
    if(AT_4G.readStringUntil('\n') != "OK"){
      Serial.println("OK false");
      return "FALSE";
    }
    return InData;
    #elif LTE_TYPE == EC_FAMILY
    #elif LTE_TYPE == SIMCOM
    #elif LTE_TYPE == AIR_FAMILY
    #endif

    #endif
    return "FALSE";
  }
};
typedef struct LTEOperate ServerLTE;
struct GNSSOperate{
  String TempPath;
  int INIT(){
    #if GNSS_SWITCH == 1

    #if GNSS_TYPE == DX_FAMILY
    #elif GNSS_TYPE == BE_FAMILY
    #endif
    File f;
    if(FileOpen("Sys/Temp/GNSS.tmp",f) == false){
      Serial.println("[GNSS] Temp File Can Not Open!");
      TempPath = "Sys/GNSS/temp.tmp";
    }else{
      f.seek(0);
      TempPath = f.readStringUntil('\n');
      Serial.println("[GNSS] Temp File Path Read!");
      if(TempPath == ""){
        Serial.println("[GNSS] Temp File Frist Create");
        f.seek(0);
        f.print("Sys/GNSS/temp.tmp\n");
        TempPath = "Sys/GNSS/temp.tmp";
        Serial.println("[GNSS] Frist Create OK");
      }
    }
    f.close();
    return true;
    #endif
    return false;
  }int GetGNSS(){
    #if GNSS_SWITCH == 1

    #if GNSS_TYPE == DX_FAMILY
    #elif GNSS_TYPE == BE_FAMILY
    #endif
    return true;

    #endif
    return false;
  }int SetBootMode(uint8_t Mode){
    #if GNSS_SWITCH == 1

    #if GNSS_TYPE == DX_FAMILY
    #elif GNSS_TYPE == BE_FAMILY
    #endif
    return true;

    #endif
    return false;
  }
};
typedef struct GNSSOperate ServerGNSS;
struct ScreenOperate{
  uint32_t CursorX;
  uint32_t CursorY;
  TouchStruct TouchPoint[128];
  int INIT(){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    ScreenI2C.begin(SCREEN_SDA,SCREEN_SCL,400000);
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int Rect(uint32_t x,uint32_t y,uint32_t width,uint32_t length,uint16_t cursor){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int Line(uint32_t startx,uint32_t starty,uint32_t endx,uint32_t endy,uint16_t color){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int DrawFile(uint32_t x,uint32_t y,String Path){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int Print(String str){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int Println(String str){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int TextSize(uint8_t size){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int TextColor(uint16_t color){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }int GetTouch(){
    #if SCREEN_SWITCH == 1

    #if SCREEN_TYPE == LT7680
    #elif SCREEN_TYPE == ST7735
    #elif SCREEN_TYPE == ST7789
    #elif SCREEN_TYPE == ILI9341
    #endif

    return true;
    #endif
    return false;
  }
};
typedef struct ScreenOperate ServerScreen;
struct CodecOperate{
  String TempPath;
  int INIT(){
    #if CODEC_SWITCH == 1
    audio.setPinout(I2S_SCLK,I2S_LRCK,I2S_DOUT,I2S_MCLK);
    audio.setVolume(21);
    File f;
    if(FileOpen("Sys/Temp/Codec.tmp",f) == false){
      Serial.println("[Codec] Temp File Can Not Open!");
      TempPath = "Sys/Codec/temp.tmp";
    }else{
      f.seek(0);
      TempPath = f.readStringUntil('\n');
      Serial.println("[Codec] Temp File Path Read!");
      if(TempPath == ""){
        Serial.println("[Codec] Temp File Frist Create");
        f.seek(0);
        f.print("Sys/Codec/temp.tmp\n");
        TempPath = "Sys/Codec/temp.tmp";
        Serial.println("[Codec] Frist Create OK");
      }
    }
    f.close();
    File tmpf;
    FileOpen(TempPath,tmpf);
    tmpf.seek(0);
    String Data = tmpf.readStringUntil('\n');
    Serial.println("[CODEC] Finding Saved Volume......");
    while(Data.substring(0,7) != "Volume:"){
      if(Data == ""){
        Serial.println("[CODEC] Can not Find Saved Volume!");
        Serial.println("[CODEC] Set Find Saved Volume Default(50)");
        tmpf.print("Volume:50\n");
        Data = "Volume:50";
        break;
      }
      Data = tmpf.readStringUntil('\n');
    }
    Serial.print("[CODEC] Find Saved Volume Succeed:");
    Data.substring(7);
    int volume = Data.toInt();
    Serial.println(volume);
    tmpf.close();
    #if CODEC_TYPE == TYPE_ES8311
    
    if(!es.begin(CODEC_SDA,CODEC_SCL,400000)){
      Serial.println("[CODEC] ES8311 Not Connect!");
      return false;
    }
    es.setVolume(volume);
    es.setBitsPerSample(16);
    #endif
    return true;
    #endif
    return false;
  }int SetVolume(uint8_t Volume){
    #if CODEC_SWITCH == 1
    Serial.println("[CODEC] Set Volume...");
    String str = String(Volume);
    File tmpf;
    FileOpen(TempPath,tmpf);
    uint32_t seek_pos = 0;
    tmpf.seek(seek_pos);
    String Data = tmpf.readStringUntil('\n');
    Serial.println("[CODEC] Find Temp File Volume Part......");
    while(Data.substring(0,7) != "Volume:"){
      if(Data == ""){
        Serial.println("[CODEC] Temp File Do Not Have Volume Part!");
        Serial.println("[CODEC] It will record volume now...");
        break;
      }
      seek_pos = tmpf.position();
      Data = tmpf.readStringUntil('\n');
    }
    Serial.println("[CODEC] Record Volume...");
    tmpf.seek(seek_pos);
    tmpf.print("Volume:");
    tmpf.println(str);
    tmpf.close();
    #if CODEC_TYPE == TYPE_ES8311
    es.setVolume(Volume);
    #endif
    
    return true;
    #endif
    return false;
  }int Instart(){
    #if CODEC_SWITCH == 1

    #if CODEC_TYPE == TYPE_ES8311
    #endif

    return true;
    #endif
    return false;
  }int InEnd(){
    #if CODEC_SWITCH == 1

    #if CODEC_TYPE == TYPE_ES8311
    #endif

    return true;
    #endif
    return false;
  }String InStr(){
    #if CODEC_SWITCH == 1

    #if CODEC_TYPE == TYPE_ES8311
    #endif

    #endif
    return "NOMOUDLESOFALSE";
  }int SetStatus(bool Status){
    #if CODEC_SWITCH == 1
    AudioSetStatus(Status);
    return true;
    #endif
    return false;
  }int SetOutFile(String Path){
    #if CODEC_SWITCH == 1
    File f;
    FileOpen(Path,f);
    f.close();
    if(Path.substring(0,4) == "Sys/"){
      Path = Path.substring(3);
      audio.connecttoFS(FFat,Path.c_str());
    }else if(Path.substring(0,3) == "SD/"){
      if(SD_Flag == false){
        Serial.println("No SD Card!");
        return false;
      }
      Path = Path.substring(2);
      audio.connecttoFS(SD,Path.c_str());
    }
    /*else if(Path.substring(0,6) == "UDisk/"){
      Path = Path.substring(5);
    }*/
    AudioSetStatus(false);
    #if CODEC_TYPE == TYPE_ES8311
    #endif
    return true;
    #endif
    return false;
  }
};
typedef struct CodecOperate ServerCodec;

#endif
