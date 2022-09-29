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
// Specifies intent of file acces is to create if necessary and open it
const char FILE_EXISTS = 'e'; 

////////////////////////////////////////////////////////////////////////////////
// Catalog
const unsigned int catalogSpecItems = 4;
const unsigned int catalogSpecLength = 0;
const unsigned int catalogSpecItemCount = 1; 
const unsigned int catalogSpecItemMaxLength = 2;
const unsigned int catalogSpecItemNameMaxLength = 3;
unsigned int catalogSpec[catalogSpecItems];
const unsigned int catalogFileNameMaxLen = 16;
char catalogCurrent[catalogFileNameMaxLen];
char indexCurrent[catalogFileNameMaxLen];
const char catalogDefault[] = "mPod";
const char catalogFileMaster[] = "catalog.t";
const char catalogFileIndex[] = "catalog.x";
const char catalogFileSortAlphaNum[] = "alphanum.x";
const char catalogFileSortLeadAlpha[] = "leadalph.x";
const char catalogFileSpec[] = "catalog.s";
bool catalogInitialized = false;
bool catalogReset = false;
const int catalogIndexItems = 6;
const int catalogIndexFileStart = 0;
const int catalogIndexFileLen = 1;
const int catalogIndexFileNameStart = 2;
const int catalogIndexFileNameLen = 3;
const int catalogIndexFileExtStart = 4;
const int catalogIndexFileExtLen = 5;
const char errCatalogCreateDirSerial[] = "Failed to create catalog directory";
const char errCatalogCreateDirScreen[] = "CATALOG ERROR 0";
const char errCatalogOpenDirSerial[] = "Failed to open catalog directory";
const char errCatalogOpenDirScreen[] = "CATALOG ERROR 1";
const char errCatalogOpenFileSerial[] = "Failed to open catalog file";
const char errCatalogOpenFileScreen[] = "CATALOG ERROR 2";
const char errCatalogOpenIndexSerial[] = "Failed to open catalog index";
const char errCatalogOpenIndexScreen[] = "CATALOG ERROR 3";
const char errCatalogRefreshFileSerial[] = "Failed to refresh catalog file";
const char errCatalogRefreshFileScreen[] = "CATALOG ERROR 4";
const char errCatalogRefreshIndexSerial[] = "Failed to refresh catalog index";
const char errCatalogRefreshIndexScreen[] = "CATALOG ERROR 5";

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
  Serial.println(F(
    "\n--------------------------------------------------------------------------------"
    "\n|                            Welcome to mPod v1.0                            |"
    "\n--------------------------------------------------------------------------------\n"
  ));

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
  // Create music catalog
  if (!catalogCreate(catalogDefault)) {
    halt(errCatalogCreateDirSerial, errCatalogCreateDirScreen);
  }
}

////////////////////////////////////////////////////////////////////////////////
// MAIN PROGRAM LOOP
void loop() {
  // Load playlist from current catalog specs for player to use
  char playlist[catalogSpec[catalogSpecLength]];
  unsigned int playlistItem[catalogSpec[catalogSpecItemCount]][catalogIndexItems];
  if (!playlistFill(playlist, playlistItem)) {
    // TODO: Some sort of error here
    Serial.println("Unable to load playlist from catalog!");
    haltImmediate();
  }
  catalogReset = false;
  // Player operation loop
  while (true) {
    Serial.print("In main loop:\n\tPlaylist: ");
    Serial.print(playlist);
    Serial.print("\n\tLength: ");
    Serial.print(catalogSpec[catalogSpecLength]);
    Serial.print("\n\tTracks: ");
    Serial.print(catalogSpec[catalogSpecItemCount]);
    Serial.print("\n\tMax track length: ");
    Serial.print(catalogSpec[catalogSpecItemMaxLength]);
    Serial.print("\n\tMax track name length: ");
    Serial.println(catalogSpec[catalogSpecItemNameMaxLength]);
    for (int i=0; i<catalogSpec[catalogSpecItemCount]; i++) {
      Serial.print("\t");
      for (int j=0; j<playlistItem[i][catalogIndexFileNameLen]; j++) {
        Serial.print(playlist[(j + playlistItem[i][catalogIndexFileNameStart])]);
      }
      Serial.print(" is a ");
      for (int j=0; j<playlistItem[i][catalogIndexFileExtLen]; j++) {
        Serial.print(playlist[(j + playlistItem[i][catalogIndexFileExtStart])]);
      }
      Serial.print(" in directory ");
      for (int j=playlistItem[i][catalogIndexFileStart]; j<playlistItem[i][catalogIndexFileNameStart]; j++) {
        Serial.print(playlist[j]);
      }
      Serial.print("\n");
    }
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
      File index = catalogOpenFile(catalogName, catalogFileIndex, FILE_WRITE);
      catalogDefaultInitialize(file, index, SD.open("/"), specs, "", 0);
      index.close();
      file.close();
      File spec = catalogOpenFile(catalogName, catalogFileSpec, FILE_WRITE);
      if (!catalogSetSpecs(spec, specs)) {
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
      catalogResetTo(catalogName);
    } else {
      Serial.println(F("Attempted to create new catalog with same name as default catalog"));
      retval = false;
    }
  } else {
    // Ensure catalog master, index and spec files are available
    File file = catalogOpenFile(catalogName, catalogFileMaster, &FILE_EXISTS);
    File index = catalogOpenFile(catalogName, catalogFileIndex, &FILE_EXISTS);
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
  if (!catalogGetSpecs(catalogName, specs)) {
    Serial.print("Unable to load specs for ");
    Serial.println(catalogName);
    return false;
  }
  spec.close();
  char sourceData[specs[catalogSpecLength]];
  File file = catalogOpenFile(catalogName, catalogFileMaster, FILE_READ);
  if (!catalogLoadMaster(catalogName, specs, sourceData)) {
    Serial.print("Unable to load catalog master file data for ");
    Serial.println(catalogName);
    return false;
  }
  file.close();
  unsigned int sourceIndex[specs[catalogSpecItemCount]][catalogIndexItems];
  File index = catalogOpenFile(catalogName, catalogFileIndex, FILE_READ);
  if (!catalogLoadIndex(catalogName, specs, sourceIndex)) {
    Serial.print("Failed to create sort files due to being unable to open index file for ");
    Serial.println(catalogName);
    return false;
  }
  index.close();
  File sortAlphaNum = catalogOpenFile(catalogName, catalogFileSortAlphaNum, FILE_WRITE);
  if (!catalogCreateSortIndexAlphanum(sortAlphaNum, specs, sourceData, sourceIndex)) {
    Serial.print("Failed creating alphanum sort index for ");
    Serial.println(catalogName);
    return false;
  }
  sortAlphaNum.close();
  // Should enhance this call (and target func) to pass array of specs to fill (ex: catalogSpec)
  File sortLeadAlpha = catalogOpenFile(catalogName, catalogFileSortLeadAlpha, FILE_WRITE);
  if (!catalogCreateSortIndexLeadAlpha(sortLeadAlpha, specs, sourceData, sourceIndex)) {
    Serial.print("Failed creating leading alpha sort index for ");
    Serial.println(catalogName);
    return false;
  }
  sortLeadAlpha.close();
  return retval;
}

bool catalogCreateSortIndexAlphanum(File sort, unsigned int *specs, char *sourceData, unsigned int sourceIndex[][catalogIndexItems]) {
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
        int result = strcmp(sourceItem, targetItem);
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
        Serial.print("ERROR: In catalogCreateSortIndexAlphanum; Attempted to assign source index ");
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
        Serial.print("ERROR: In catalogCreateSortIndexAlphanum; Source index ");
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
      setBytesFromUint(sortedData[i], &msb, &lsb);
      size += sort.write(msb);
      size += sort.write(lsb);
    }
    if (size != (specs[catalogSpecItemCount] * 2)) {
      Serial.print("Error in catalogCreateSortIndexAlphanum: Expected size of sort file was ");
      Serial.print(specs[catalogSpecItemCount] * 2);
      Serial.print(" but actual number of bytes written was ");
      Serial.println(size);
      return false;
    }
    Serial.print("Sort alphanum results:");
    for (int i=0; i<specs[catalogSpecItemCount]; i++) {
      Serial.print("\n\t");
      for (int j=0; j<sourceIndex[sortedData[i]][catalogIndexFileNameLen]; j++) {
        Serial.print(sourceData[(j + sourceIndex[sortedData[i]][catalogIndexFileNameStart])]);
      }
      Serial.print(" is a ");
      for (int j=0; j<sourceIndex[sortedData[i]][catalogIndexFileExtLen]; j++) {
        Serial.print(sourceData[(j + sourceIndex[sortedData[i]][catalogIndexFileExtStart])]);
      }
      Serial.print(" in directory ");
      for (int j=sourceIndex[sortedData[i]][catalogIndexFileStart]; j<sourceIndex[sortedData[i]][catalogIndexFileNameStart]; j++) {
        Serial.print(sourceData[j]);
      }
    }
    Serial.print("\n");
  }
  return retval;
}

bool catalogCreateSortIndexLeadAlpha(File sort, unsigned int *specs, char *sourceData, unsigned int sourceIndex[][catalogIndexItems]) {
  bool retval = true;
  return retval;
}

void catalogDefaultInitialize(File file, File index, File dir, unsigned int *specs, char *dirName, unsigned int dirLen) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
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
  // Currently the hard-coded exclusions are max 8 chars
  char compare[8];
  for (unsigned int i=0; i<8; i++) {
    if ((dir[i] == '/') || (dir[i] == '\0')) {
      compare[i] = '\0';
    } else {
      compare[i] = dir[i];
    }
  }
  if ((strcmp(compare, "/mPod") == 0) || (strcmp(compare, "/.Trash") == 0)) {
    retval = true;
  }
  return retval;
}

bool catalogDirectoryReady(char *dirName) {
  bool retval = true;
  if (!SD.exists(dirName)) {
    if (!SD.mkdir(dirName)) {
      halt(errCatalogCreateDirSerial, errCatalogCreateDirScreen);
    }
  }
  // Ensure the passed catalog is a directory
  File dir = SD.open(dirName);
  if (!dir) {
    halt(errCatalogOpenDirSerial, errCatalogOpenDirScreen);
  }
  if (!dir.isDirectory()) {
    retval = false;
  }
  dir.close();
  return retval;
}

unsigned int catalogGetPathLen(const char *dirName) {
  unsigned int retval = 0;
  unsigned int len;
  if (strcmp(dirName, catalogDefault) != 0) {
    // Will need to prepend catalogDefault path name
    len = 0;
    while (catalogDefault[len] != '\0') {
      len++;
      retval++;
    }
    retval += 1; // for '/' between catalogDefault and dirName
  }
  len = 0;
  while (dirName[len] != '\0') {
    len++;
    retval++;
  }
  retval += 2; // Leading root '/' and ending term char
  return retval;
}

void catalogGetPathName(char *buffer, unsigned int buffSize, const char *dirName) {
  strcpy(buffer, "/");
  strcat(buffer, catalogDefault);
  if (strcmp(dirName, catalogDefault) != 0) {
    strcat(buffer, "/");
    strcat(buffer, dirName);
  }
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
Test Test TEst
*/
bool catalogGetUpperDataBySizedRange(char *buffer, unsigned int from, unsigned int length, char *data) {
  bool retval = true;
  for (unsigned int i = 0; i < length; i++) {
    buffer[i] = toupper(data[(i + from)]);
  }
  buffer[length] = '\0';
  return retval;
}

bool catalogGetSpecs(const char *catalogName, unsigned int *specs) {
  bool retval = true;
  File spec = catalogOpenFile(catalogName, catalogFileSpec, FILE_READ);
  if (!spec) {
    Serial.print("Unable to load spec file for ");
    Serial.println(catalogName);
    return false;
  }
  if (spec.available() != (catalogSpecItems * 2)) {
    Serial.print("Size of contents in spec file (");
    Serial.print(spec.available());
    Serial.print(") does not match expected size of ");
    Serial.print(catalogSpecItems * 2);
    Serial.println(" bytes");
    spec.close();
    return false;
  }
  byte msb, lsb;
  for (int i=0; i<catalogSpecItems; i++) {
    msb = spec.read();
    lsb = spec.read();
    specs[i] = getUintFromBytes(msb, lsb);
  }
  spec.close();
  return retval;
}

bool catalogLoadIndex(const char *catalogName, unsigned int *specs, unsigned int catalogItemDef[][catalogIndexItems]) {
  bool retval = true;
  File index = catalogOpenFile(catalogName, catalogFileIndex, FILE_READ);
  if (index.available() >= (specs[catalogSpecItemCount] * catalogIndexItems * 2)) {
    byte msb, lsb;
    for (int i=0; i<specs[catalogSpecItemCount]; i++) {
      for (int j=0; j<catalogIndexItems; j++) {
        msb = index.read();
        lsb = index.read();
        catalogItemDef[i][j] = getUintFromBytes(msb, lsb);
      }
    }
  } else {
    Serial.println(F("Unable to load catalog index, catalog index is smaller than allocated memory buffer!"));
    retval = false;
  }
  index.close();
  return retval;
}

bool catalogLoadMaster(const char *catalogName, unsigned int *specs, char *catalog) {
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

File catalogOpenFile(const char *catalogName, const char *fileName, const char *fileMode) {
  File file;
  char newFileName[24]; // Max possible with 8.3 naming
  // Retrieve directory name for catalog
  unsigned int dirLength = catalogGetPathLen(catalogName);
  char dirName[dirLength];
  catalogGetPathName(dirName, dirLength, catalogName);
  // Validate working directory for catalog
  if (!catalogDirectoryReady(dirName)) {
    Serial.print(F("Unable to create/verify working directory "));
    Serial.println(dirName);
    return file;
  }
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
    Serial.print(F("Error opening file: "));
    Serial.print(dirName);
    Serial.print(F("/"));
    Serial.println(fileName);
    halt(errCatalogOpenIndexSerial, errCatalogOpenIndexScreen);
  }
  return file;
}

bool catalogResetTo(const char *catalogName) {
  bool retval = true;
  if (!catalogGetSpecs(catalogName, catalogSpec)) {
    Serial.print("Unable to load specs for ");
    Serial.println(catalogName);
    return false;
  }
  strcpy(catalogCurrent, catalogName);
  catalogReset = true;
  return retval;
}

bool catalogSetSpecs(File spec, unsigned int *specs) {
  bool retval = true;
  byte msb, lsb;
  if (spec.seek(0)) {
    for (int i=0; i<catalogSpecItems; i++) {
      setBytesFromUint(specs[i], &msb, &lsb);
      spec.write(msb);
      spec.write(lsb);
    }
  } else {
    retval = false;
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

bool playlistFill(char *playlist, unsigned int playlistItem[][catalogIndexItems]) {
  bool retval = true;
  if (!catalogLoadMaster(catalogCurrent, catalogSpec, playlist)) {
    Serial.print("Unable to load catalog master file data for ");
    Serial.println(catalogCurrent);
    return false;
  }
  if (!catalogLoadIndex(catalogCurrent, catalogSpec, playlistItem)) {
    Serial.print("Failed to load index file for ");
    Serial.println(catalogCurrent);
    return false;
  }
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

////////////////////////////////////////////////////////////////////////////////
// Return two indivudual bytes representing an unsigned integer
void setBytesFromUint(unsigned int val, byte *msb, byte *lsb) {
  *lsb = val & 0xFF;
  *msb = val >> 8;
}