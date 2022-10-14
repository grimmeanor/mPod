////////////////////////////////////////////////////////////////////////////////
// mPod v1.0
//
// It's the 2020's and the iPod has gone the way of the dodo bird. But some of
// us really want their iPod, or SoMeThInG exactly like an iPod. This is not
// that SoMeThInG. But I did try to make something nice for a one-off, homemade,
// side project with whatever tinkering, programming, electronics, and 3-D
// modeling and assembly skills I may (or may not) have. Enjoy.

////////////////////////////////////////////////////////////////////////////////
// Copyright 2022 Christopher James Baker
// MIT License - https://opensource.org/licenses/MIT

////////////////////////////////////////////////////////////////////////////////
// This code is specifically configured for the following hardware & libraries.
// Please pay attention to any required changes you may need to make to the
// pinouts and library #includes should you attempt to use this code with other
// hardware or libraries.
//   Hardware
//     Adafruit ESP32 Feather V2 (MAIN BOARD)
//       https://www.adafruit.com/product/5400
//     Adafruit VS1053 Codec + MicroSD Breakout - v4
//       MP3, AAC, Ogg Vorbis, WMA, MIDI, FLAC, WAV Decoder
//       https://www.adafruit.com/product/1381
//     ANO Directional Navigation and Scroll Wheel Rotary Encoder
//       https://www.adafruit.com/product/5001
//     Adafruit 1.3" 240x240 Wide Angle TFT LCD Display with MicroSD - ST7789
//       https://www.adafruit.com/product/4313
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <RotaryEncoder.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

////////////////////////////////////////////////////////////////////////////////
// PINOUTS - MAIN BOARD
#define VBAT          35  // LiPoly battery monitor
// Adafruit VS1053 Codec + MicroSD Breakout - v4
#define VS1053_CLK    5   // SPI Clock, shared with SD card
#define VS1053_MISO   21  // Input data, from VS1053/SD card
#define VS1053_MOSI   19  // Output data, to VS1053/SD card
#define VS1053_RESET  20 //12  // VS1053 reset pin (output)
#define VS1053_CS     7   // VS1053 chip select pin (output)
#define VS1053_DCS    27  // VS1053 Data/command select pin (output)
#define VS1053_SD_CS  8   // Card chip select pin
#define VS1053_DREQ   13  // VS1053 Data request, ideally an Interrupt pin
// ANO Directional Navigation and Scroll Wheel Rotary Encoder
#define ANO_ENCA      33  // Rotary Encoder Channel A
#define ANO_ENCB      15  // Rotary Encoder Channel B
#define ANO_SWC       32  // Directional Navigation Center Button
#define ANO_SWL       34 //26  // Directional Navigation Left Button
#define ANO_SWU       39 //25  // Directional Navigation Up Button
#define ANO_SWR       36 //34  // Directional Navigation Right Button
#define ANO_SWD       37 //39  // Directional Navigation Down Button
// Adafruit 1.3" 240x240 Wide Angle TFT LCD Display with MicroSD - ST7789
#define TFT_CS        22
#define TFT_DC        4
#define TFT_MOSI      12
#define TFT_CLK       14
#define TFT_LITE      26

////////////////////////////////////////////////////////////////////////////////
// Serial
const unsigned int serialBaudRate = 230400;
const unsigned int serialMaxInputLength = 255;
const char serialTermChar = '\n';
const char inputTermChar = '\0';

////////////////////////////////////////////////////////////////////////////////
// Adafruit 1.3" 240x240 Wide Angle TFT LCD Display with MicroSD - ST7789
Adafruit_ST7789 *screen = NULL;
SPIClass *tftSPI = NULL;
const unsigned int screenWidth = 240;
const unsigned int screenHeight = 240;
const int colorWhite = 0xFFFF;
const int colorBlack = 0x0001;
const int colorRed    = 0b1111100000000000;
const int colorGreen  = 0b0000011111100000;
const int colorBlue   = 0b0000000000011111;
const int colorYellow = colorRed|colorGreen;
const int colorAqua   = colorBlue|colorGreen;
const int colorPurple = colorRed|colorBlue;
const int colorVioletFemme = 0x9CDF;
const int colorPink = 0xFB33;
const int colorLightPink = 0xFE7F;
const int colorNavy = 0x000C;
const int colorTransparent = 0x0000; // Background only!
const byte vAlignTop    = 0b00000001;
const byte vAlignCenter = 0b00000010;
const byte vAlignBottom = 0b00000100;
const byte hAlignLeft   = 0b00001000;
const byte hAlignCenter = 0b00010000;
const byte hAlignRight  = 0b00100000;

////////////////////////////////////////////////////////////////////////////////
// Adafruit VS1053 Codec + MicroSD Breakout - v4
Adafruit_VS1053_FilePlayer *mPod = nullptr;
const byte supportedMusicFileTypeCount = 9;
const char supportedMusicFileTypes[] = "M4A\0" "MP3\0" "MID\0" "MIDI\0" "WAV\0"
  "AAC\0" "OGG\0" "WMA\0" "FLAC";
const int defaultVolume = 20;
int currVolume;
const char msgVS1053FaultSerial[] = "FATAL ERROR: VS1053 not found";
const char msgVS1053FaultScreen[] = "VS1053 HARDWARE FAULT";

////////////////////////////////////////////////////////////////////////////////
// ANO Directional Navigation and Rotary Encoder
RotaryEncoder *scrollWheel = nullptr;

////////////////////////////////////////////////////////////////////////////////
// Files
// Specifies intent of file access is to just to create if necessary
const char FILE_EXISTS = 'e'; 

////////////////////////////////////////////////////////////////////////////////
// Catalog
const char playerDirectory[] = "/.mPod";
const char playerFileMaster[] = "settings.t";
const char playerFileIndex[] = "settings.x";
const char playerFileTempMaster[] = "temp.t";
const char playerFileTempIndex[] = "temp.x";
const int playerIndexCount = 3;
const int playerIndexCatalog = 0; // Current catalog (or default)
const int playerIndexIndexType = 1; // Current index (or default)
const int playerIndexTrack = 2; // Current track (or first entry by default)
const unsigned int excludeDirectoryCount = 2;
const unsigned int excludeDirectoryMaxLen = 16;
const char excludeDirectory[excludeDirectoryCount][excludeDirectoryMaxLen] = {
  "/.mPod", "/.Trash-1000"
};
const char catalogDefault[] = "Default";
const unsigned int catalogNameMaxLen = 16;
char catalogCurrent[catalogNameMaxLen+1];
const char catalogFileMaster[] = "catalog.t";
const char catalogFileSpec[] = "catalog.s";
bool catalogInitialized = false;
bool catalogReset = false;
const unsigned int catalogSpecItems = 4;
const unsigned int catalogSpecLength = 0;
const unsigned int catalogSpecItemCount = 1; 
const unsigned int catalogSpecItemMaxLength = 2;
const unsigned int catalogSpecItemNameMaxLength = 3;
unsigned int catalogSpec[catalogSpecItems];
const int catalogIndexItems = 6;
const int catalogIndexFileStart = 0;
const int catalogIndexFileLen = 1;
const int catalogIndexFileNameStart = 2;
const int catalogIndexFileNameLen = 3;
const int catalogIndexFileExtStart = 4;
const int catalogIndexFileExtLen = 5;
const unsigned int catalogIndexTypeCount = 3;
const unsigned int catalogIndexTypeDefault = 0;
const unsigned int catalogIndexTypeSortAlphanum = 1;
const unsigned int catalogIndexTypeSortLeadAlpha = 2;
const unsigned int catalogIndexTypeFileNameMaxLen = 11;
const char catalogIndexTypeFileName[catalogIndexTypeCount][catalogIndexTypeFileNameMaxLen] = {
  "default.x", "alphanum.x", "leadalph.x"
};
const unsigned int catalogIndexTypeNameMaxLen = 14;
const char catalogIndexTypeName[catalogIndexTypeCount][catalogIndexTypeNameMaxLen] = {
  "Default", "Alphanumeric", "Leading Alpha"
};
unsigned int catalogIndexTypeCurrent;
const char errCatalogCreateDirSerial[] = "Failed to create catalog directory for %s";
const char errCatalogCreateDirScreen[] = "CATALOG ERROR 0";
const char errCatalogOpenIndexSerial[] = "Failed to open %s catalog index file %s";
const char errCatalogOpenIndexScreen[] = "CATALOG ERROR 3";
const char errCatalogRefreshFileSerial[] = "Failed to refresh default catalog file";
const char errCatalogRefreshFileScreen[] = "CATALOG ERROR 4";

////////////////////////////////////////////////////////////////////////////////
// SETUP
void setup() {
  pinMode(ANO_ENCA, INPUT);
  pinMode(ANO_ENCB, INPUT);
  pinMode(ANO_SWC, INPUT_PULLUP);
  pinMode(ANO_SWL, INPUT_PULLUP);
  pinMode(ANO_SWU, INPUT_PULLUP);
  pinMode(ANO_SWR, INPUT_PULLUP);
  pinMode(ANO_SWD, INPUT_PULLUP);
  pinMode(TFT_LITE, OUTPUT);
  
  Serial.begin(serialBaudRate);
  while(!Serial);
  serialBanner("mPod Startup");
  
  //----------------------------------------------------------------------------
  // Start Adafruit 1.3" 240x240 Wide Angle TFT LCD Display
  tftSPI = new SPIClass(HSPI);
  tftSPI->begin(TFT_CLK, -1, TFT_MOSI, TFT_CS);
  pinMode(tftSPI->pinSS(), OUTPUT);
  screen = new Adafruit_ST7789(tftSPI, TFT_CS, TFT_DC, -1);
  screen->init(screenWidth, screenHeight);
  digitalWrite(TFT_LITE, HIGH);
  screenShowWelcome();
  
  //----------------------------------------------------------------------------
  // Start mPod music player
  // Adafruit_VS1052_FilePlayer from Adafruit_VS1053 library can be configured 
  // to use either hardware SPI or software SPI to connect to the main board.
  // Leave the appropriate constructor uncommented based on your connections,
  // and comment out the other. See the following for reference:
  //   https://learn.adafruit.com/adafruit-vs1053-mp3-aac-ogg-midi-wav-play-and-record-codec-tutorial/library-reference
  // Software SPI configuration
  // Adafruit_VS1053_FilePlayer mPod = Adafruit_VS1053_FilePlayer(
  //   VS1053_MOSI, VS1053_MISO, VS1053_CLK, VS1053_RESET, VS1053_CS, 
  //   VS1053_DCS, VS1053_DREQ, VS1053_SD_CS
  // );
  // Hardware SPI configuration
  // Adafruit_VS1053_FilePlayer mPod = Adafruit_VS1053_FilePlayer(
  //   VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_SD_CS
  // );
  mPod = new Adafruit_VS1053_FilePlayer(
    VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, VS1053_SD_CS
  );
  if (!mPod->begin()) {
    halt(msgVS1053FaultSerial, msgVS1053FaultScreen);
  }
  Serial.println(F("VS1053 started"));
  if (!SD.begin(VS1053_SD_CS)) {
    halt(msgVS1053FaultSerial, msgVS1053FaultScreen);
  }
  Serial.println(F("SD Card found"));
  mPodInitialize();

  //----------------------------------------------------------------------------
  // Start ANO Directional Navigation and Rotary Encoder
  scrollWheel = new RotaryEncoder(
    ANO_ENCB, ANO_ENCA, RotaryEncoder::LatchMode::TWO03
  );
  attachInterrupt(digitalPinToInterrupt(ANO_ENCA), scrollWheelChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ANO_ENCB), scrollWheelChange, CHANGE);
  
  //----------------------------------------------------------------------------
  // Load player settings
  if (!playerSettingsLoad()) {
    halt("Unable to load player settings from SD card", "SETTINGS LOAD ERROR");
  }
  
  //----------------------------------------------------------------------------
  // Create default music catalog from files on SD card
  if (!catalogCreate(catalogDefault)) {
    const unsigned int msgTemplateLen = strlen(errCatalogCreateDirSerial);
    unsigned int catalogLen = strlen(catalogDefault);
    char msg[(msgTemplateLen + catalogLen)];
    int r = sprintf(msg, errCatalogCreateDirSerial, catalogDefault);
    halt(msg, errCatalogCreateDirScreen);
  }
  
  //----------------------------------------------------------------------------
  // Initialize playlists
  if (!catalogRefreshStalePlaylists()) {
    halt("Unable to sync playlists with default catalog", "CATALOG ERROR 5");
  }
  
  if (!catalogResetTo(catalogCurrent)) {
    if (!catalogResetTo(catalogDefault)) {
      halt("Unable to open catalog from settings or default!", "CATALOG ERROR 1");
    }
  }
  
  serialBanner("Welcome to mPod v1.0");
}

////////////////////////////////////////////////////////////////////////////////
// MAIN PROGRAM LOOP
void loop() {
  // Load playlist from catalog specs for player to use
  char playlist[catalogSpec[catalogSpecLength]];
  unsigned int playlistItem[catalogSpec[catalogSpecItemCount]][catalogIndexItems];
  catalogIndexTypeCurrent = catalogIndexTypeSortAlphanum;
  // TODO: Validate playlist files against master prior to loading
  if (!catalogLoad(playlist, playlistItem)) {
    // TODO: Some sort of error here
    Serial.println("Unable to load playlist from catalog!");
    haltImmediate();
  }
  catalogReset = false;
  // Player operation loop
  while (true) {
    catalogListSerial(catalogSpec, playlist, playlistItem);
    halt("Testing completed!", "TEST COMPLETE");
    if (catalogReset) {
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Create a new catalog (or create/refresh the default catalog)
bool catalogCreate(const char *catalogName) {
  bool retval = true;
  // Different actions between creating new playlist and loading default catalog
  if (strcmp(catalogName, catalogDefault) == 0) {
    if (!catalogInitialized) {
      // Initialize catalog with music files from SD card
      unsigned int specs[catalogSpecItems];
      for (int i=0; i<catalogSpecItems; i++) {
        specs[i] = 0;
      }
      File file = catalogOpenFile(catalogName, catalogFileMaster, FILE_WRITE);
      File index = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexTypeDefault], FILE_WRITE);
      catalogDefaultInitialize(file, index, SD.open("/"), specs, "", 0);
      index.close();
      file.close();
      File spec = catalogOpenFile(catalogName, catalogFileSpec, FILE_WRITE);
      if (!specFileWrite(spec, catalogSpecItems, specs)) {
        Serial.println(F("Error populating catalog spec file!"));
        return false;
      }
      spec.close();
      catalogInitialized = true;
      if (!catalogCreateSortIndexes(catalogName)) {
        Serial.print("Error creating sort indexes for ");
        Serial.println(catalogName);
        return false;
      }
    } else {
      Serial.println(F("Attempted to create new catalog with same name as default catalog"));
      retval = false;
    }
  } else {
    // Ensure catalog master, index and spec files are available
    File file = catalogOpenFile(catalogName, catalogFileMaster, &FILE_EXISTS);
    File index = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexTypeDefault], &FILE_EXISTS);
    File spec = catalogOpenFile(catalogName, catalogFileSpec, &FILE_EXISTS);
    if (!file || !index || !spec) {
      Serial.println(F("Unable to create/verify required catalog files"));
      retval = false;
    }
    spec.close();
    index.close();
    file.close();
  }
  return retval;
}

bool catalogCreateSortIndexes(const char *catalogName) {
  bool retval = true;
  unsigned int specs[catalogSpecItems];
  File spec = catalogOpenFile(catalogName, catalogFileSpec, FILE_READ);
  if (!specFileRead(spec, catalogSpecItems, specs)) {
    Serial.print("Failed to load specs for catalog ");
    Serial.print(catalogName);
    return false;
  }
  spec.close();
  char sourceData[specs[catalogSpecLength]];
  File file = catalogOpenFile(catalogName, catalogFileMaster, FILE_READ);
  if (!catalogFileMasterRead(catalogName, specs, sourceData)) {
    Serial.print("Unable to load catalog master file data for ");
    Serial.println(catalogName);
    return false;
  }
  file.close();
  unsigned int sourceIndex[specs[catalogSpecItemCount]][catalogIndexItems];
  File index = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexTypeDefault], FILE_READ);
  if (!catalogFileIndexRead(catalogName, catalogIndexTypeDefault, specs, sourceIndex)) {
    Serial.print("Failed to create sort files due to being unable to open index file for ");
    Serial.println(catalogName);
    return false;
  }
  index.close();
  File sortAlphaNum = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexTypeSortAlphanum], FILE_WRITE);
  if (!catalogCreateSortIndex(catalogIndexTypeSortAlphanum, sortAlphaNum, specs, sourceData, sourceIndex)) {
    Serial.print("Failed creating alphanum sort index for ");
    Serial.println(catalogName);
    return false;
  }
  sortAlphaNum.close();
  // Should enhance this call (and target func) to pass array of specs to fill (ex: catalogSpec)
  File sortLeadAlpha = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexTypeSortLeadAlpha], FILE_WRITE);
  if (!catalogCreateSortIndex(catalogIndexTypeSortLeadAlpha, sortLeadAlpha, specs, sourceData, sourceIndex)) {
    Serial.print("Failed creating leading alpha sort index for ");
    Serial.println(catalogName);
    return false;
  }
  sortLeadAlpha.close();
  return retval;
}

bool catalogCreateSortIndex(unsigned int sortType, File sort, unsigned int *specs, char *sourceData, unsigned int sourceIndex[][catalogIndexItems]) {
  bool retval = true;
  unsigned int sortedData[specs[catalogSpecItemCount]];
  bool sortAssigned[specs[catalogSpecItemCount]];
  for (unsigned int i=0; i<specs[catalogSpecItemCount]; i++) {
    sortAssigned[i] = false;
  }
  for (unsigned int i=0; i<specs[catalogSpecItemCount]; i++) {
    char sourceItem[sourceIndex[i][catalogIndexFileNameLen] + 1];
    catalogGetUpperDataBySizedRange(sourceItem, sourceIndex[i][catalogIndexFileNameStart], sourceIndex[i][catalogIndexFileNameLen], sourceData);
    unsigned int sortedPos = 0;
    unsigned int duplicates = 0;
    for (unsigned int j=0; j<specs[catalogSpecItemCount]; j++) {
      if (j != i) {
        char targetItem[sourceIndex[j][catalogIndexFileNameLen] + 1];
        catalogGetUpperDataBySizedRange(targetItem, sourceIndex[j][catalogIndexFileNameStart], sourceIndex[j][catalogIndexFileNameLen], sourceData);
        int result;
        if (sortType == catalogIndexTypeSortAlphanum) {
          result = strcmp(sourceItem, targetItem);
        } else if (sortType == catalogIndexTypeSortLeadAlpha) {
          result = strcmpLeadAlpha(sourceItem, sourceIndex[i][catalogIndexFileNameLen] + 1, targetItem, sourceIndex[j][catalogIndexFileNameLen] + 1);
        } else {
          // Bad error here
          Serial.println("This crappy message means an unknown sort type was passed to catalogCreateSortIndex()");
          return false;
        }
        if (result >= 0) {
          sortedPos++;
        }
        if (result == 0) {
          duplicates++;
        }
      }
    }
    if (duplicates == 0) {
      if (!sortAssigned[sortedPos]) {
        sortedData[sortedPos] = i;
        sortAssigned[sortedPos] = true;
      } else {
        Serial.print("ERROR: In catalogCreateSortIndex; Attempted to assign source index ");
        Serial.print(i);
        Serial.print(" (");
        Serial.print(sourceItem);
        Serial.print(") to sort index ");
        Serial.print(sortedPos);
        Serial.print(" which has already been assigned value ");
        Serial.print(sortedData[sortedPos]);
        Serial.print("\n");
        return false;
      }
    } else {
      bool assigned = false;
      for (unsigned int j=sortedPos-duplicates; j<sortedPos+1; j++) {
        if (!sortAssigned[j]) {
          sortedData[j] = i;
          sortAssigned[j] = true;
          assigned = true;
          break;
        }
      }
      if (!assigned) {
        Serial.print("ERROR: In catalogCreateSortIndex; Source index ");
        Serial.print(i);
        Serial.print(" (");
        Serial.print(sourceItem);
        Serial.print(") with ");
        Serial.print(duplicates);
        Serial.print(" duplicates, was unable to be assigned in the expected sort range ");
        Serial.print(sortedPos-duplicates);
        Serial.print(" to ");
        Serial.print(sortedPos);
        Serial.print(" because all values in the range have already been assigned\n");
        return false;
      }
    }
  }
  bool sorted = true;
  for (int i=0; i<specs[catalogSpecItemCount]; i++) {
    if (!sortAssigned[i]) {
      Serial.print("Sort index ");
      Serial.print(i);
      Serial.println(" was left unassigned after sort operation!");
      sorted = false;
    }
  }
  if (sorted) {
    byte msb, lsb;
    unsigned int size = 0;
    for (int i=0; i<specs[catalogSpecItemCount]; i++) {
      for (int j=0; j<catalogIndexItems; j++) {
        setBytesFromUint(sourceIndex[sortedData[i]][j], &msb, &lsb);
        size += sort.write(msb);
        size += sort.write(lsb);
      }
    }
    if (size != (specs[catalogSpecItemCount] * catalogIndexItems * 2)) {
      Serial.print("Error in catalogCreateSortIndex: Expected size of sort file was ");
      Serial.print(specs[catalogSpecItemCount] * 2);
      Serial.print(" but actual number of bytes written was ");
      Serial.println(size);
      return false;
    }
  }
  return retval;
}

void catalogDefaultInitialize(File file, File index, File dir, unsigned int *specs, char *dirName, unsigned int dirLen) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    // can't use strrchr() with entry.name() so doing this for len and pos
    unsigned int entryLen = 0;
    unsigned int lastDot = 0;
    while (entry.name()[entryLen] != '\0') {
      if (entry.name()[entryLen] == '.') {
        lastDot = entryLen;
      }
      entryLen++;
    }
    unsigned int entryPathLen = dirLen + entryLen + 1; // add '/' between
    char entryName[entryPathLen];
    strcpy(entryName, dirName);
    strcat(entryName, "/");
    strcat(entryName, entry.name());
    if (entry.isDirectory()) {
      if (!catalogDirectoryExcluded(entryName, entryPathLen)) {
        catalogDefaultInitialize(file, index, entry, specs, entryName, entryPathLen);
      }
    } else {
      unsigned int indexFileItem[catalogIndexItems];
      indexFileItem[catalogIndexFileStart] = file.position();
      indexFileItem[catalogIndexFileLen] = entryPathLen;
      indexFileItem[catalogIndexFileNameStart] = indexFileItem[catalogIndexFileStart] + dirLen + 1;
      indexFileItem[catalogIndexFileNameLen] = lastDot;
      indexFileItem[catalogIndexFileExtStart] = indexFileItem[catalogIndexFileNameStart] + lastDot + 1;
      indexFileItem[catalogIndexFileExtLen] = entryLen - (lastDot + 1);
      specs[catalogSpecLength] += file.print(entryName);
      byte lsb, msb;
      for (int i=0; i<catalogIndexItems; i++) {
        setBytesFromUint(indexFileItem[i], &msb, &lsb);
        index.write(msb);
        index.write(lsb);
      }
      specs[catalogSpecItemCount]++;
      if (indexFileItem[catalogIndexFileLen] > specs[catalogSpecItemMaxLength]) {
        specs[catalogSpecItemMaxLength] = indexFileItem[catalogIndexFileLen];
      }
      if (indexFileItem[catalogIndexFileNameLen] > specs[catalogSpecItemNameMaxLength]) {
        specs[catalogSpecItemNameMaxLength] = indexFileItem[catalogIndexFileNameLen];
      }
    }
    entry.close();
  }
  dir.close();
}

bool catalogDirectoryExcluded(char *dir, unsigned int dirLen) {
  bool retval = false;
  for (int i=0; i<excludeDirectoryCount; i++) {
    if (strcmp(dir, excludeDirectory[i]) == 0) {
      retval = true;
    }
  }
  return retval;
}

bool catalogFileIndexRead(const char *catalogName, unsigned int catalogIndexType, unsigned int *specs, unsigned int catalogItemDef[][catalogIndexItems]) {
  bool retval = true;
  File index = catalogOpenFile(catalogName, catalogIndexTypeFileName[catalogIndexType], FILE_READ);
  if (index.available() >= (specs[catalogSpecItemCount] * catalogIndexItems * 2)) {
    for (int i=0; i<specs[catalogSpecItemCount]; i++) {
      catalogFileIndexReadEntry(index, i, catalogItemDef);
    }
  } else {
    Serial.println(F("Unable to load catalog index, catalog index is smaller than allocated memory buffer!"));
    retval = false;
  }
  index.close();
  return retval;
}

void catalogFileIndexReadEntry(File index, unsigned int entryIndex, unsigned int catalogItemDef[][catalogIndexItems]) {
  byte msb, lsb;
  for (int i=0; i<catalogIndexItems; i++) {
    msb = index.read();
    lsb = index.read();
    catalogItemDef[entryIndex][i] = getUintFromBytes(msb, lsb);
  }
}

bool catalogFileMasterRead(const char *catalogName, unsigned int *specs, char *catalog) {
  bool retval = true;
  File file = catalogOpenFile(catalogName, catalogFileMaster, FILE_READ);
  if (file.available() >= specs[catalogSpecLength]) {
    for (unsigned int i=0; i<specs[catalogSpecLength]; i++) {
      catalog[i] = char(file.read());
    }
  } else {
    Serial.println(F("Unable to load catalog master data, File is smaller than allocated memory buffer!"));
    retval = false;
  }
  file.close();
  return retval;
}

// Passed charArray should expect term char to be added as last char (specified by length)
bool catalogFileMasterReadChars(File file, unsigned int start, unsigned int length, char *charArray) {
  bool retval = true;
  file.seek(0);
  if (file.available() < (start + length)) {
    Serial.print("Attempted overflow read of data from file ");
    Serial.print(file.name());
    Serial.print(" with size ");
    Serial.print(file.available());
    Serial.print(" by reading ");
    Serial.print(length);
    Serial.print(" bytes starting at position ");
    Serial.println(start);
    return false;
  }
  if (!file.seek(start)) {
    Serial.print("Unable to seek position ");
    Serial.print(start);
    Serial.print(" in file ");
    Serial.println(file.name());
    return false;
  }
  for (unsigned int i=0; i<length; i++) {
    charArray[i] = file.read();
  }
  charArray[length] = '\0';
  return retval;
}

// Buffer must contain extra char after range for term char
bool catalogGetDataByIndexedRange(char *buffer, unsigned int from, unsigned int to, char *data) {
  unsigned int length = to - from + 1;
  return catalogGetDataBySizedRange(buffer, from, length, data);
}

// Buffer must contain extra char after given length for term char
bool catalogGetDataBySizedRange(char *buffer, unsigned int from, unsigned int length, char *data) {
  bool retval = true;
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = data[(i + from)];
  }
  buffer[length] = '\0';
  return retval;
}

// Buffer must contain extra char after range for term char
bool catalogGetUpperDataByIndexedRange(char *buffer, unsigned int from, unsigned int to, char *data) {
  unsigned int length = to - from + 1;
  return catalogGetUpperDataBySizedRange(buffer, from, length, data);
}

// Buffer must contain extra char after given length for term char
/**
 * Neato description here
 * @param char *buffer    
 * @return success or failure
*/
bool catalogGetUpperDataBySizedRange(char *buffer, unsigned int from, unsigned int length, char *data) {
  bool retval = true;
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = toupper(data[(i + from)]);
  }
  buffer[length] = '\0';
  return retval;
}

void catalogListSerial(unsigned int *specs, char *playlist, unsigned int playlistItem[][catalogIndexItems]) {
  const char bannerTemplate[] = "Playlist: %s (%d files)";
  const unsigned int bannerTemplateLen = strlen(bannerTemplate);
  const unsigned int filesLen = 5; // 2 byte unsigned integer max val is 5 digits
  unsigned int catLen = strlen(catalogCurrent);
  char bannerText[(bannerTemplateLen + catLen + filesLen)];
  int r = sprintf(bannerText, bannerTemplate, catalogCurrent, specs[catalogSpecItemCount]);
  serialBanner(bannerText);
  for (int i=0; i<specs[catalogSpecItemCount]; i++) {
    Serial.print("    ");
    if (i<10) {
      Serial.print(" ");
    }
    Serial.print(i);
    Serial.print("    ");
    for (int j=0; j<playlistItem[i][catalogIndexFileNameLen]; j++) {
      Serial.print(playlist[(j + playlistItem[i][catalogIndexFileNameStart])]);
    }
    Serial.print("\n");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Load data and index for the currently selected catalog
bool catalogLoad(char *playlist, unsigned int playlistItem[][catalogIndexItems]) {
  bool retval = true;
  if (!catalogFileMasterRead(catalogCurrent, catalogSpec, playlist)) {
    Serial.print("Unable to load catalog master file data for ");
    Serial.println(catalogCurrent);
    return false;
  }
  if (!catalogFileIndexRead(catalogCurrent, catalogIndexTypeCurrent, catalogSpec, playlistItem)) {
    Serial.print("Failed to load index file for ");
    Serial.println(catalogCurrent);
    return false;
  }
  return retval;
}

File catalogOpenFile(const char *catalogName, const char *fileName, const char *fileMode) {
  File file;
  // Retrieve directory name for catalog
  unsigned int dirLength = getPathCharLen(playerDirectory) + getPathCharLen(catalogName);
  char dirName[(dirLength)];
  strcpy(dirName, playerDirectory);
  strcat(dirName, "/");
  strcat(dirName, catalogName);
  // Validate working directory for catalog
  if (!directoryVerify(dirName)) {
    Serial.print(F("Unable to create/verify working directory "));
    Serial.println(dirName);
    return file;
  }
  dirLength += getPathCharLen(fileName);
  char newFileName[dirLength];
  strcpy(newFileName, dirName);
  strcat(newFileName, "/");
  strcat(newFileName, fileName);
  if (!catalogInitialized && (strcmp(catalogName, catalogDefault) == 0)) {
    // Default catalog files need to be recreated on initialization
    if (SD.exists(newFileName)) {
      if (!SD.remove(newFileName)) {
        halt (errCatalogRefreshFileSerial, errCatalogRefreshFileScreen);
      }
    }
  }
  if (*fileMode == FILE_EXISTS) {
    if (SD.exists(newFileName)) {
      file = SD.open(newFileName, FILE_READ);
    } else {
      file = SD.open(newFileName, FILE_WRITE);
    }
  } else {
    file = SD.open(newFileName, fileMode);
  }
  if (!file) {
    unsigned int msgTemplateLen = strlen(errCatalogOpenIndexSerial);
    unsigned int catalogLen = strlen(catalogName);
    char msg[(msgTemplateLen + dirLength + catalogLen)];
    int r = sprintf(msg, errCatalogOpenIndexSerial, catalogName, newFileName);
    halt(msg, errCatalogOpenIndexScreen);
  }
  return file;
}

bool catalogPlaylistIsStale(const char *playlistName, unsigned int *defaultSpec, unsigned int *tempSpecs, char *defaultData, unsigned int defaultIndex[][catalogIndexItems]) {
  bool retval = false;
  Serial.print("\t");
  Serial.println(playlistName);
  // Loop through playlist entries in default index order
  unsigned int playlistSpec[catalogSpecItems];
  File playlistSpecFile = catalogOpenFile(playlistName, catalogFileSpec, FILE_READ);
  if (!specFileRead(playlistSpecFile, catalogSpecItems, playlistSpec)) {
    Serial.print("Failed to sync playlist ");
    Serial.print(playlistName);
    Serial.println(" against default catalog due to problem reading playlist spec file!");
  }
  playlistSpecFile.close();
  File playlistMaster = catalogOpenFile(playlistName, catalogFileMaster, FILE_READ);
  File playlistIndex = catalogOpenFile(playlistName, catalogIndexTypeFileName[catalogIndexTypeDefault], FILE_READ);
  File playlistTempMaster = catalogOpenFile(playlistName, playerFileTempMaster, FILE_WRITE);
  File playlistTempIndex = catalogOpenFile(playlistName, playerFileTempIndex, FILE_WRITE);
  for (unsigned int i=0; i<playlistSpec[catalogSpecItemCount]; i++) {
    Serial.print("\t\tplaylist item ");
    Serial.print(i);
    Serial.print(": ");
    // TODO: Is a 2-D array necessary here?
    unsigned int playlistEntry[1][catalogIndexItems];
    catalogFileIndexReadEntry(playlistIndex, 0, playlistEntry);
    char playlistEntryName[playlistEntry[0][catalogIndexFileLen] + 1];
    catalogFileMasterReadChars(playlistMaster, playlistEntry[0][catalogIndexFileStart], playlistEntry[0][catalogIndexFileLen], playlistEntryName);
    for (unsigned int j=0; j<playlistEntry[0][catalogIndexFileLen] + 1; j++) {
      Serial.print(playlistEntryName[j], HEX);
      Serial.print(",");
      if (isLowerCase(playlistEntryName[j])) {
        playlistEntryName[j] = playlistEntryName[j] - ('a' - 'A');
      }
      Serial.print(playlistEntryName[j], HEX);
      Serial.print(" ");
    }
    Serial.println(playlistEntryName);
    // Check this entry name against default catalog
    bool match = false;
    unsigned int searchIndex = (int)(defaultSpec[catalogSpecItemCount] / 2.0);
    unsigned int searchUbound = defaultSpec[catalogSpecItemCount] - 1;
    unsigned int searchLbound = 0;
    unsigned int sameIndex = 0;
    unsigned int newSearchIndex;
    while (true) {
      Serial.print("\t\t\tComparing to default index ");
      Serial.print(searchIndex);
      Serial.print(": ");
      char defaultEntryName[defaultIndex[searchIndex][catalogIndexFileLen] + 1];
      for (unsigned int j=0; j<defaultIndex[searchIndex][catalogIndexFileLen]; j++) {
        defaultEntryName[j] = defaultData[(j + defaultIndex[searchIndex][catalogIndexFileStart])];
      }
      defaultEntryName[defaultIndex[searchIndex][catalogIndexFileLen]] = '\0';
      for (unsigned int j=0; j<defaultIndex[searchIndex][catalogIndexFileLen] + 1; j++) {
        Serial.print(defaultEntryName[j], HEX);
        Serial.print(",");
        if (isLowerCase(defaultEntryName[j])) {
          defaultEntryName[j] = defaultEntryName[j] - ('a' - 'A');
        }
        Serial.print(defaultEntryName[j], HEX);
        Serial.print(" ");
      }
      Serial.print(defaultEntryName);
      Serial.print("\n");
      int result = strcmp(playlistEntryName, defaultEntryName);
      if (result == 0) {
        match = true;
        Serial.println("\t\t\tMATCH FOUND");
        // Write the default catalog data at this searchIndex location to the keeper data/index files playlistTempMaster and playlistTempIndex
        unsigned int tempIndex[catalogIndexItems];
        tempIndex[catalogIndexFileStart] = tempSpecs[catalogSpecLength];
        tempIndex[catalogIndexFileLen] = defaultIndex[searchIndex][catalogIndexFileLen];
        tempIndex[catalogIndexFileNameStart] = tempIndex[catalogIndexFileStart] + defaultIndex[searchIndex][catalogIndexFileNameStart] - defaultIndex[searchIndex][catalogIndexFileStart];
        tempIndex[catalogIndexFileNameLen] = defaultIndex[searchIndex][catalogIndexFileNameLen];
        tempIndex[catalogIndexFileExtStart] = tempIndex[catalogIndexFileStart] + defaultIndex[searchIndex][catalogIndexFileLen] - defaultIndex[searchIndex][catalogIndexFileExtLen];
        tempIndex[catalogIndexFileExtLen] = defaultIndex[searchIndex][catalogIndexFileExtLen];
        tempSpecs[catalogSpecLength] += playlistTempMaster.print(playlistEntryName);
        Serial.print("\t\t\tExpected: ");
        Serial.print(tempIndex[catalogIndexFileStart]);
        Serial.print(", ");
        Serial.print(tempIndex[catalogIndexFileLen]);
        Serial.print(", ");
        Serial.print(tempIndex[catalogIndexFileNameStart]);
        Serial.print(", ");
        Serial.print(tempIndex[catalogIndexFileNameLen]);
        Serial.print(", ");
        Serial.print(tempIndex[catalogIndexFileExtStart]);
        Serial.print(", ");
        Serial.print(tempIndex[catalogIndexFileExtLen]);
        Serial.print("\n\t\t\tWriting: ");
        byte lsb, msb;
        for (int j=0; j<catalogIndexItems; j++) {
          setBytesFromUint(tempIndex[j], &msb, &lsb);
          playlistTempIndex.write(msb);
          playlistTempIndex.write(lsb);
          Serial.print("(");
          Serial.print(msb);
          Serial.print(",");
          Serial.print(lsb);
          Serial.print(") ");
        }
        Serial.print("\n");
        tempSpecs[catalogSpecItemCount]++;
        if (tempSpecs[catalogSpecItemMaxLength] < defaultIndex[searchIndex][catalogIndexFileLen]) {
          tempSpecs[catalogSpecItemMaxLength] = defaultIndex[searchIndex][catalogIndexFileLen];
        }
        if (tempSpecs[catalogSpecItemNameMaxLength] < defaultIndex[searchIndex][catalogIndexFileNameLen]) {
          tempSpecs[catalogSpecItemNameMaxLength] = defaultIndex[searchIndex][catalogIndexFileNameLen];
        }
        break;
      } else if (result > 0) {
        Serial.print("\t\t\tseeking higher position between ");
        searchLbound = searchIndex;
        newSearchIndex = searchIndex + (unsigned int)((searchUbound - searchIndex) / 2.0) + 1;
      } else {
        Serial.print("\t\t\tseeking lower position between ");
        searchUbound = searchIndex;
        newSearchIndex = searchLbound + (unsigned int)((searchIndex - searchLbound) / 2.0);
      }
      if (newSearchIndex == searchIndex || sameIndex > 1) {
        Serial.println("\t\t\tNO MATCH FOUND");
        retval = true;
        break; // No match found
      } else {
        if (newSearchIndex == searchUbound || newSearchIndex == searchLbound) {
          sameIndex++;
        }
        Serial.print(searchLbound);
        Serial.print(" and ");
        Serial.println(searchUbound);
        searchIndex = newSearchIndex;
      }
    }
    Serial.print("\t\tchanges:");
    Serial.println(retval);
  }
  playlistTempIndex.close();
  playlistTempMaster.close();
  playlistMaster.close();
  playlistIndex.close();
  return retval;
}

bool catalogResetTo(const char *catalogName) {
  bool retval = true;
  File currSpec = catalogOpenFile(catalogName, catalogFileSpec, FILE_READ);
  if (!specFileRead(currSpec, catalogSpecItems, catalogSpec)) {
    Serial.print("No specs available for ");
    Serial.println(catalogName);
    return false;
  }
  currSpec.close();
  strcpy(catalogCurrent, catalogName);
  catalogReset = true;
  return retval;
}

bool catalogRefreshPlaylist(const char *playlistName, unsigned int *tempSpecs) {
  bool retval = true;
  Serial.print("\t\tRebuilding playlist \"");
  Serial.print(playlistName);
  Serial.println("\" due to deltas with primary catalog");
  // remove old files and replace with contents of temp files
  unsigned int dirLength = getPathCharLen(playerDirectory) + getPathCharLen(playlistName);
  char dirName[(dirLength)];
  strcpy(dirName, playerDirectory);
  strcat(dirName, "/");
  strcat(dirName, playlistName);
  strcat(dirName, "/");
  unsigned int tempMasterLength = dirLength + getPathCharLen(playerFileTempMaster);
  unsigned int tempIndexLength = dirLength + getPathCharLen(playerFileTempIndex);
  unsigned int playlistSpecLength = dirLength + getPathCharLen(catalogFileSpec);
  unsigned int playlistMasterLength = dirLength + getPathCharLen(catalogFileMaster);
  unsigned int playlistIndexDefaultLength = dirLength + getPathCharLen(catalogIndexTypeFileName[catalogIndexTypeDefault]);
  unsigned int playlistIndexSortAlphanumLength = dirLength + getPathCharLen(catalogIndexTypeFileName[catalogIndexTypeSortAlphanum]);
  unsigned int playlistIndexSortLeadAlphaLength = dirLength + getPathCharLen(catalogIndexTypeFileName[catalogIndexTypeSortLeadAlpha]);
  char tempMasterName[(tempMasterLength)];
  char tempIndexName[(tempIndexLength)];
  char playlistSpecName[(playlistSpecLength)];
  char playlistMasterName[(playlistMasterLength)];
  char playlistIndexDefaultName[(playlistIndexDefaultLength)];
  char playlistIndexSortAlphanumName[(playlistIndexSortAlphanumLength)];
  char playlistIndexSortLeadAlphaName[(playlistIndexSortLeadAlphaLength)];
  strcpy(tempMasterName, dirName);
  strcpy(tempIndexName, dirName);
  strcpy(playlistSpecName, dirName);
  strcpy(playlistMasterName, dirName);
  strcpy(playlistIndexDefaultName, dirName);
  strcpy(playlistIndexSortAlphanumName, dirName);
  strcpy(playlistIndexSortLeadAlphaName, dirName);
  strcat(tempMasterName, playerFileTempMaster);
  strcat(tempIndexName, playerFileTempIndex);
  strcat(playlistSpecName, catalogFileSpec);
  strcat(playlistMasterName, catalogFileMaster);
  strcat(playlistIndexDefaultName, catalogIndexTypeFileName[catalogIndexTypeDefault]);
  strcat(playlistIndexSortAlphanumName, catalogIndexTypeFileName[catalogIndexTypeSortAlphanum]);
  strcat(playlistIndexSortLeadAlphaName, catalogIndexTypeFileName[catalogIndexTypeSortLeadAlpha]);
  if (!SD.remove(playlistSpecName)) {
    Serial.print("\t\tFailed to remove stale catalog file ");
    Serial.println(playlistSpecName);
  }
  if (!SD.remove(playlistMasterName)) {
    Serial.print("\t\tFailed to remove stale catalog file ");
    Serial.println(playlistMasterName);
  }
  if (!SD.remove(playlistIndexDefaultName)) {
    Serial.print("\t\tFailed to remove stale catalog file ");
    Serial.println(playlistIndexDefaultName);
  }
  if (!SD.remove(playlistIndexSortAlphanumName)) {
    Serial.print("\t\tFailed to remove stale catalog file ");
    Serial.println(playlistIndexSortAlphanumName);
  }
  if (!SD.remove(playlistIndexSortLeadAlphaName)) {
    Serial.print("\t\tFailed to remove stale catalog file ");
    Serial.println(playlistIndexSortLeadAlphaName);
  }
  // Can reuse spec, master and index file objects from earlier
  File playlistSpecFile = catalogOpenFile(playlistName, catalogFileSpec, FILE_WRITE);
  if (!specFileWrite(playlistSpecFile, catalogSpecItems, tempSpecs)) {
    Serial.print("\t\tFailed to create fresh spec file ");
    Serial.println(playlistSpecName);
  }
  playlistSpecFile.close();
  File playlistMaster = catalogOpenFile(playlistName, catalogFileMaster, FILE_WRITE);
  File playlistTempMaster = catalogOpenFile(playlistName, playerFileTempMaster, FILE_READ);
  while (playlistTempMaster.available()) {
    playlistMaster.write(playlistTempMaster.read());
  }
  playlistMaster.close();
  playlistTempMaster.close();
  SD.remove(tempMasterName);
  File playlistIndex = catalogOpenFile(playlistName, catalogIndexTypeFileName[catalogIndexTypeDefault], FILE_WRITE);
  File playlistTempIndex = catalogOpenFile(playlistName, playerFileTempIndex, FILE_READ);
  while (playlistTempIndex.available()) {
    playlistIndex.write(playlistTempIndex.read());
  }
  playlistIndex.close();
  playlistTempIndex.close();
  SD.remove(tempIndexName);
  if (!catalogCreateSortIndexes(playlistName)) {
    Serial.print("\t\tFailed to create fresh sort indexes for playlist ");
    Serial.print(playlistName);
  }
  return retval;
}

bool catalogRefreshStalePlaylists() {
  bool retval = true;
  // load default catalog and it's alphanum index into memory for more performant lookups
  unsigned int defaultSpec[catalogSpecItems];
  File defaultSpecFile = catalogOpenFile(catalogDefault, catalogFileSpec, FILE_READ);
  if (!specFileRead(defaultSpecFile, catalogSpecItems, defaultSpec)) {
    halt("Unable to load default catalog specs", "CATALOG ERROR 6");
  }
  defaultSpecFile.close();
  char defaultData[defaultSpec[catalogSpecLength]];
  unsigned int defaultIndex[defaultSpec[catalogSpecItemCount]][catalogIndexItems];
  if (!catalogFileMasterRead(catalogDefault, defaultSpec, defaultData)) {
    halt("Unable to load default catalog master data file", "CATALOG ERROR 7");
  }
  if (!catalogFileIndexRead(catalogDefault, catalogIndexTypeSortAlphanum, defaultSpec, defaultIndex)) {
    halt("Unable to load default catalog alphanum sort file", "CATALOG ERROR 8");
  }
  // Loop through playlists
  Serial.println("Checking playlists");
  File playerDir = SD.open(playerDirectory, FILE_READ);
  while (true) {
    File playlist = playerDir.openNextFile(FILE_READ);
    if (!playlist) {
      break;
    }
    if (playlist.isDirectory() && (strcmp(playlist.name(), catalogDefault) != 0)) {
      unsigned int tempSpecs[catalogSpecItems];
      for (unsigned int i=0; i<catalogSpecItems; i++) {
        tempSpecs[i]=0;
      }
      if (catalogPlaylistIsStale(playlist.name(), defaultSpec, tempSpecs, defaultData, defaultIndex)) {
        if (!catalogRefreshPlaylist(playlist.name(), tempSpecs)) {
          Serial.println("Whoopsie, that did not work");
        }
      } else {
        // Cleanup temp files used to compare playlist files to default files
        Serial.print("\t\tPlaylist \"");
        Serial.print(playlist.name());
        Serial.println(" is unchanged");
        unsigned int dirLength = getPathCharLen(playerDirectory) + getPathCharLen(playlist.name());
        char dirName[(dirLength)];
        strcpy(dirName, playerDirectory);
        strcat(dirName, "/");
        strcat(dirName, playlist.name());
        strcat(dirName, "/");
        unsigned int tempMasterLength = dirLength + getPathCharLen(playerFileTempMaster);
        unsigned int tempIndexLength = dirLength + getPathCharLen(playerFileTempIndex);
        char tempMasterName[(tempMasterLength)];
        char tempIndexName[(tempIndexLength)];
        strcpy(tempMasterName, dirName);
        strcpy(tempIndexName, dirName);
        strcat(tempMasterName, playerFileTempMaster);
        strcat(tempIndexName, playerFileTempIndex);
        if (!SD.remove(tempMasterName)) {
          Serial.print("\t\tFailed to remove comparison file ");
          Serial.println(tempMasterName);
        }
        if (!SD.remove(tempIndexName)) {
          Serial.print("\t\tFailed to remove comparison file ");
          Serial.println(tempIndexName);
        }
      }
    }
    playlist.close();
  }
  playerDir.close();
  return retval;
}

bool directoryVerify(char *dirName) {
  bool retval = true;
  if (!SD.exists(dirName)) {
    if (!SD.mkdir(dirName)) {
      char msgTemplate[] = "Fatal error creating directory %s";
      unsigned int msgTemplateLen = strlen(msgTemplate);
      unsigned int dirLen = strlen(dirName);
      char msg[(msgTemplateLen + dirLen)];
      int r = sprintf(msg, msgTemplate, dirName);
      halt(msg, "DIR VERIFY ERROR");
    }
  }
  // Ensure the passed value is a directory
  File dir = SD.open(dirName);
  if (!dir) {
    char msgTemplate[] = "Error opening directory %s";
    unsigned int msgTemplateLen = strlen(msgTemplate);
    unsigned int dirLen = strlen(dirName);
    char msg[(msgTemplateLen + dirLen)];
    int r = sprintf(msg, msgTemplate, dirName);
    halt(msg, "DIRECTORY ERROR");
  }
  if (!dir.isDirectory()) {
    retval = false;
  }
  dir.close();
  return retval;
}

unsigned int getPathCharLen(const char *dir) {
  unsigned int retval = 0;
  if (dir[0] == '\0') {
    return 1; // root dir '/'
  }
  if (dir[0] != '/') {
    retval++; // will need extra char to prepend with root '/'
  }
  unsigned int len = strlen(dir);
  retval += len;
  if (dir[(len-1) != '/']) {
    retval++; // will need extra char to append with term '/'
  }
  return retval;
}

unsigned int getUintFromBytes(byte msb, byte lsb) {
  unsigned int retval;
  retval = (unsigned int)msb;
  retval = retval << 8;
  retval += (unsigned int)lsb;
  return retval;
}

////////////////////////////////////////////////////////////////////////////////
// Halts program immediately with no notification
void haltImmediate() {
  while (true) {
  }
}

////////////////////////////////////////////////////////////////////////////////
// Halts program with notification
void halt(const char *msgSerial, const char *msgScreen) {
  Serial.println(msgSerial);
  Serial.println(F(
    "\n--------------------------------------------------------------------------------"
    "\n|                                PROGRAM HALTED                                |"
    "\n--------------------------------------------------------------------------------\n"
  ));
  screenDrawTextWithin(
    0, 0, screenWidth, (int)(screenHeight / 6.0), vAlignCenter|hAlignCenter, FreeSansBold9pt7b,
    colorBlack, colorPink, 1, false, true, msgScreen
  );
  screenDrawTextWithin(
    0, (int)(screenHeight / 3.0), screenWidth, (int)(screenHeight / 6.0),
    vAlignCenter|hAlignCenter, FreeSansBold9pt7b, colorBlack, colorPink,
    1, false, true, "*ding - dong*"
  );
  screenDrawTextWithin(
    0, (int)(screenHeight / 2.0), screenWidth, (int)(screenHeight / 6.0),
    vAlignCenter|hAlignCenter, FreeSansBold9pt7b, colorBlack, colorPink,
    1, false, true, "mPod is dead"
  );
  haltImmediate();
}

////////////////////////////////////////////////////////////////////////////////
// mPod initial configuration
void mPodInitialize() {
  currVolume = defaultVolume;
  mPod->setVolume(currVolume, currVolume);
  mPod->useInterrupt(VS1053_DREQ); // allows for background audio playing
}

////////////////////////////////////////////////////////////////////////////////
// Load saved player settings
bool playerSettingsLoad() {
  bool retval = true;
  unsigned int dirLen = getPathCharLen(playerDirectory);
  char dirPath[dirLen];
  if (!setPathChars(dirPath, dirLen, playerDirectory)) {
    halt("Failed in playerSettingsLoad() calling setPathChars()", "SETTINGS ERROR 2");
  }
  if (!directoryVerify(dirPath)) {
    halt("Failed in playerSettingsLoad() calling directoryVerify()", "SETTINGS ERROR 3");
  }
  unsigned int fileLen = strlen(playerFileMaster);
  unsigned int indexLen = strlen(playerFileIndex);
  File file;
  File index;
  char filePath[(dirLen + fileLen + 1)];
  char indexPath[(dirLen + indexLen + 1)];
  strcpy(filePath, dirPath);
  strcat(filePath, "/");
  strcat(filePath, playerFileMaster);
  strcpy(indexPath, dirPath);
  strcat(indexPath, "/");
  strcat(indexPath, playerFileIndex);
  if (SD.exists(filePath) && SD.exists(indexPath)) {
    // Read files and set variables
    file = SD.open(filePath, FILE_READ);
    index = SD.open(indexPath, FILE_READ);
    // Each index is two values (start, length) each a 2 byte uint
    unsigned int indexSize = playerIndexCount * 2 * 2;
    if (index.available() != indexSize) {
      const char msgTemplate[] = "Player settings index is %d bytes, expected %d bytes";
      unsigned int msgTemplateLen = strlen(msgTemplate);
      char msg[(msgTemplateLen + 7)];
      int r = sprintf(msg, msgTemplate, index.available(), indexSize);
      halt(msg, "SETTINGS ERROR 5");
    }
    unsigned int keys[playerIndexCount][2];
    byte msb, lsb;
    for (int i=0; i<playerIndexCount; i++) {
      msb = index.read();
      lsb = index.read();
      keys[i][0] = getUintFromBytes(msb, lsb);
      msb = index.read();
      lsb = index.read();
      keys[i][1] = getUintFromBytes(msb, lsb);
    }
    unsigned int fileSize = keys[playerIndexCount-1][0] + keys[playerIndexCount-1][1];
    if (file.available() != fileSize) {
      const char msgTemplate[] = "Player settings master is %d bytes, expected %d bytes";
      unsigned int msgTemplateLen = strlen(msgTemplate);
      char msg[(msgTemplateLen + 7)];
      int r = sprintf(msg, msgTemplate, file.available(), fileSize);
      halt(msg, "SETTINGS ERROR 6");
    }
    char fileData[fileSize];
    for (int i=0; i<fileSize; i++) {
      fileData[i] = file.read();
    }
    // Set current catalog
    if (keys[playerIndexCatalog][1] <= 1) {
      strcpy(catalogCurrent, catalogDefault);
    } else {
      for (int i=0; i<keys[playerIndexCatalog][1]; i++) {
        catalogCurrent[i] = fileData[i + keys[playerIndexCatalog][0]];
      }
      catalogCurrent[keys[playerIndexCatalog][1]] = '\0';
    }
    // set current index type
    char indexType[keys[playerIndexIndexType][1]];
    for (int i=0; i<keys[playerIndexIndexType][1]; i++) {
      indexType[i] = fileData[(i + keys[playerIndexIndexType][0])];
    }
    catalogIndexTypeCurrent = atoi(indexType);
    // TODO: Current catalog track
  } else {
    // Set variables to default and create files from defaults
    strcpy(catalogCurrent, catalogDefault);
    catalogIndexTypeCurrent = catalogIndexTypeDefault;
    // TODO: Current catalog track
    file = SD.open(filePath, FILE_WRITE);
    if (!file) {
      halt("Unable to create player master file", "SETTINGS ERROR 1");
    }
    index = SD.open(indexPath, FILE_WRITE);
    if (!index) {
      halt("Unable to create player index file", "SETTINGS ERROR 4");
    }
    unsigned int filePos = 0;
    unsigned int len;
    byte msb, lsb;
    for (unsigned int i=0; i<playerIndexCount; i++) {
      // write startPos to index
      setBytesFromUint(filePos, &msb, &lsb);
      index.write(msb);
      index.write(lsb);
      // write file data
      if (i == playerIndexCatalog) {
        len = file.print(catalogDefault);
      } else if (i == playerIndexIndexType) {
        len = file.print(catalogIndexTypeDefault);
      } else if (i == playerIndexTrack) {
        len = file.print("/");
      } else {
        const char msgTemplate[] = "Unexpected playerIndex value %d in playerSettingsLoad()";
        unsigned int msgTemplateLen = strlen(msgTemplate);
        char msg[(msgTemplateLen + 1)];
        int r = sprintf(msg, msgTemplate, i);
        // How did you let this happen? This is not good, better halt
        halt(msg, "SETTINGS ERROR 5");
      }
      filePos += len;
      // write length to index
      setBytesFromUint(len, &msb, &lsb);
      index.write(msb);
      index.write(lsb);
    }
  }
  file.close();
  index.close();
  return retval;
}

////////////////////////////////////////////////////////////////////////////////
// Draws a font with background using specified boundaries and justification
// Text is allowed to overflow out of the boundary. Passing a zero for hBound
// height will result in auto-height being used with the required text height
//   xBound     X coord of top left corner of desired boundary
//   yBound     Y coord of top left corner of desired boundary
//   wBound     Width in pixels of boundary
//   hBound     Height in pixels of boundary
//   align      Alignment of text within the boundary (default center/center)
//                V and H values can be combined (ex: vAlignTop|vAlignLeft)
//                Available V values
//                  vAlignTop, vAlignCenter, vAlignBottom
//                Availabe Y values
//                  hAlignLeft, hAlignCenter, hAlignRight
//   font       Adafruit GFX library font to use
//   foreColor  Text color
//   backColor  Color of Boundary or text background
//   size       Size of text (as in setTextSize() from Adafruit GFX)
//   wrap       Wrap text (true/false)
//   fillBounds If true, boundary will fill with backColor before text
//                otherwise only the minimum area the text occupies will
//                be filled. Optional colorTransaprent can be passed to
//                prevent filling of any boundary or background
//   text       Text to draw within the boundary
void screenDrawTextWithin(
  int xBound, int yBound, int wBound, int hBound, byte align, GFXfont font,
  int foreColor, int backColor, int size, bool wrap, bool fillBounds, const char *text
) {
  bool autoHeight = false;
  int16_t xText, yText;
  uint16_t wText, hText;
  screen->setFont(&font);
  screen->setTextSize(size);
  screen->setTextWrap(wrap);
  screen->getTextBounds(text, 0, 0, &xText, &yText, &wText, &hText);
  if (hBound == 0) {
    autoHeight = true;
    hBound = hText;
  }
  // adjust X
  if ((align & hAlignLeft) == hAlignLeft) {
    xText = xBound;
  } else if ((align & hAlignCenter) == hAlignCenter) {
    xText = xBound + (int)(wBound / 2.0) - (int)(wText / 2.0);
  } else if ((align & hAlignRight) == hAlignRight) {
    xText = xBound + wBound - wText;
  } else {
    xText = xBound;
  }
  // adjust Y and H
  if ((align & vAlignTop) == vAlignTop) {
    yText = yBound + hText;
  } else if ((align & vAlignCenter) == vAlignCenter) {
    if (autoHeight) {
      yBound -= (int)(hBound / 2.0);
    }
    yText = yBound + (int)(hBound / 2.0) + (int)(hText / 2.0);
  } else if ((align & vAlignBottom) == vAlignBottom) {
    if (autoHeight) {
      yBound -= hBound;
    }
    yText = yBound + hBound;
  } else {
    yText = yBound + hText;
  }
  // adjust H
  if (autoHeight) {
    if ((align & vAlignCenter) == vAlignCenter) {
      
    }
  }
  // background
  if (backColor != colorTransparent) {
    if (fillBounds) {
      screen->fillRect(xBound, yBound, wBound, hBound, backColor);
    } else {
      screen->fillRect(xText, yText-hText, wText, hText, backColor);
    }
  }
  // draw, partner *pew-pew-pew*
  screen->setCursor(xText, yText);
  screen->setTextColor(foreColor);
  screen->print(text);
}

////////////////////////////////////////////////////////////////////////////////
// TFT startup screen
void screenShowWelcome() {
  screen->fillScreen(colorNavy);
  screenDrawTextWithin(
    0, (int)(screenHeight/2.0), screenWidth, 0, vAlignCenter|hAlignCenter,
    FreeSansBold9pt7b, colorWhite, colorTransparent, 1, false, false,
    "Welcome to mPod"
  );
}
////////////////////////////////////////////////////////////////////////////////
// ISR for input changes on the scroll wheel
void scrollWheelChange() {
  scrollWheel->tick();
}

void serialBanner(char *text) {
  const unsigned int hCount = 80;
  const char hChar = '-';
  const unsigned int vCount = 2;
  const char vChar = '|';
  const char padChar = ' ';
  unsigned int len=0;
  while (text[len] != '\0') {
    len++;
  }
  unsigned int leftPadding = (unsigned int)(((hCount - vCount) - len) / 2.0);
  unsigned int rightPadding = hCount - (leftPadding+len) - vCount;
  Serial.print("\n");
  serialRepeatChar(hChar, hCount);
  Serial.print("\n");
  Serial.print(vChar);
  serialRepeatChar(padChar, leftPadding);
  Serial.print(text);
  serialRepeatChar(padChar, rightPadding);
  Serial.print(vChar);
  Serial.print("\n");
  serialRepeatChar(hChar, hCount);
  Serial.print("\n");
}

void serialRepeatChar(char out, unsigned int count) {
  for (int i=0; i<count; i++) {
    Serial.print(out);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Return two indivudual bytes representing an unsigned integer
void setBytesFromUint(unsigned int val, byte *msb, byte *lsb) {
  *lsb = val & 0xFF;
  *msb = val >> 8;
}

////////////////////////////////////////////////////////////////////////////////
// Create full path directory from input value
//   dirNameOut      Char array for output of full directory path
//   dirNameOutSize  Needs to include space for term char
//   dirNameIn       Name to build full directory path from (term char expected)
bool setPathChars(char *dirNameOut, unsigned int dirNameOutSize, const char *dirNameIn) {
  bool retval = true;
  unsigned int dirNameOutPos = 0;
  unsigned int dirNameInPos = 0;
  if (dirNameIn[0] != '/') {
    dirNameOutPos++;
    dirNameOut[dirNameOutPos] = '/';
  }
  while (dirNameIn[dirNameInPos] != '\0') {
    if (dirNameOutPos >= dirNameOutSize) {
      return false;
    }
    dirNameOut[dirNameOutPos] = dirNameIn[dirNameInPos];
    dirNameOutPos++;
    dirNameInPos++;
  }
  dirNameOut[dirNameOutPos] = '\0';
  return retval;
}

bool specFileRead(File specFile, unsigned int specCount, unsigned int *specArray) {
  bool retval = true;
  if (specFile.available() != (specCount * 2)) {
    Serial.print("Size of contents in spec file (");
    Serial.print(specFile.available());
    Serial.print(") does not match expected size of ");
    Serial.print(specCount * 2);
    Serial.println(" bytes");
    specFile.close();
    return false;
  }
  byte msb, lsb;
  for (int i=0; i<specCount; i++) {
    msb = specFile.read();
    lsb = specFile.read();
    specArray[i] = getUintFromBytes(msb, lsb);
  }
  return retval;
}

bool specFileWrite(File specFile, unsigned int specCount, unsigned int *specArray) {
  bool retval = true;
  byte msb, lsb;
  if (specFile.seek(0)) {
    for (int i=0; i<specCount; i++) {
      setBytesFromUint(specArray[i], &msb, &lsb);
      specFile.write(msb);
      specFile.write(lsb);
    }
  } else {
    retval = false;
  }
  return retval;
}

////////////////////////////////////////////////////////////////////////////////
// Determines if source is greater than, less than, or equal to the target based
// on the characters from the first alpha char onwards. Items with no alpha
// chars at all are placed at the end in character code order.
// I made this since I don't know of an existing C/C++ function that does this
int strcmpLeadAlpha(char *source, unsigned int sourceLen, char *target, unsigned int targetLen) {
  int retval = 0;
  bool sourceHasChar = false;
  unsigned int sourceCharAt;
  bool targetHasChar = false;
  unsigned int targetCharAt;
  for (int i=0; i<sourceLen; i++) {
    if (isalpha(source[i])) {
      sourceHasChar = true;
      sourceCharAt = i;
      break;
    }
  }
  for (int i=0; i<targetLen; i++) {
    if (isalpha(target[i])) {
      targetHasChar = true;
      targetCharAt = i;
      break;
    }
  }
  if (!sourceHasChar && !targetHasChar) {
    return strcmp(source, target);
  }
  if (sourceHasChar && !targetHasChar) {
    return -1;
  }
  if (!sourceHasChar && targetHasChar) {
    return 1;
  }
  if (sourceCharAt == 0 && targetCharAt == 0) {
    return strcmp(source, target);
  }
  for (int i=0; i<sourceLen-sourceCharAt; i++) {
    if (i >= targetLen-targetCharAt) {
      // source is greater than because source has chars remaining
      return 1;
    }
    if (source[(i+sourceCharAt)] > target[(i+targetCharAt)]) {
      return 1;
    }
    if (source[(i+sourceCharAt)] < target[(i+targetCharAt)]) {
      return -1;
    }
  }
  // Check if source ran out of chars first
  if ((sourceLen-sourceCharAt) < (targetLen-targetCharAt)) {
    // source is less than since it ran out of chars first
    return -1;
  }
  return retval;
}
