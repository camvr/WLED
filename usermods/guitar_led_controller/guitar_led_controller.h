#pragma once

#include "wled.h"

//
// Inspired by the original v2 usermods
// * usermod_v2_rotary_encoder_ui_ALT
//
// v2 usermod for controlling the Guitar LED system.
//
// Rotating the encoder clockwise selects the next preset, and 
// counter-clockwise selects the previous preset, only allowing the preset
// directly next to the current one to be selected. Pressing the button
// will change to the selected preset.
//

#ifdef USERMOD_MODE_SORT
  #error "Usermod Mode Sort is no longer required. Remove -D USERMOD_MODE_SORT from platformio.ini"
#endif

#ifndef ENCODER_DT_PIN
#define ENCODER_DT_PIN 18
#endif

#ifndef ENCODER_CLK_PIN
#define ENCODER_CLK_PIN 19
#endif

#ifndef ENCODER_SW_PIN
#define ENCODER_SW_PIN 5
#endif

// Number of modes at the start of the list to not sort
#define MODE_SORT_SKIP_COUNT 1

// Which list is being sorted
static const char **listBeingSorted;

/**
 * Modes and palettes are stored as strings that
 * end in a quote character. Compare two of them.
 * We are comparing directly within either
 * JSON_mode_names or JSON_palette_names.
 */
static int re_qstringCmp(const void *ap, const void *bp) {
  const char *a = listBeingSorted[*((byte *)ap)];
  const char *b = listBeingSorted[*((byte *)bp)];
  int i = 0;
  do {
    char aVal = pgm_read_byte_near(a + i);
    if (aVal >= 97 && aVal <= 122) {
      // Lowercase
      aVal -= 32;
    }
    char bVal = pgm_read_byte_near(b + i);
    if (bVal >= 97 && bVal <= 122) {
      // Lowercase
      bVal -= 32;
    }
    // Relly we shouldn't ever get to '\0'
    if (aVal == '"' || bVal == '"' || aVal == '\0' || bVal == '\0') {
      // We're done. one is a substring of the other
      // or something happenend and the quote didn't stop us.
      if (aVal == bVal) {
        // Same value, probably shouldn't happen
        // with this dataset
        return 0;
      }
      else if (aVal == '"' || aVal == '\0') {
        return -1;
      }
      else {
        return 1;
      }
    }
    if (aVal == bVal) {
      // Same characters. Move to the next.
      i++;
      continue;
    }
    // We're done
    if (aVal < bVal) {
      return -1;
    }
    else {
      return 1;
    }
  } while (true);
  // We shouldn't get here.
  return 0;
}


class GuitarLedController : public Usermod {
private:
  unsigned long loopTime = 0;

  unsigned char buttonState = HIGH;
  unsigned char prevButtonState = HIGH;

  int8_t pinA = ENCODER_DT_PIN;       // DT from encoder
  int8_t pinB = ENCODER_CLK_PIN;      // CLK from encoder
  int8_t pinC = ENCODER_SW_PIN;       // SW from encoder

  // Pointers the start of the mode names within JSON_mode_names
  const char **modes_qstrings = nullptr;

  // Array of mode indexes in alphabetical order.
  byte *modes_alpha_indexes = nullptr;

  // Pointers the start of the palette names within JSON_palette_names
  const char **palettes_qstrings = nullptr;

  // Array of palette indexes in alphabetical order.
  byte *palettes_alpha_indexes = nullptr;

  unsigned char Enc_A;
  unsigned char Enc_B;
  unsigned char Enc_A_prev = 0;

  bool currentEffectAndPaletteInitialized = false;
  uint8_t effectCurrentIndex = 0;
  uint8_t effectPaletteIndex = 0;
  uint8_t knownMode = 0;
  uint8_t knownPalette = 0;

  byte presetHigh = 0;
  byte presetLow = 0;

  bool moveForward = true; // defaults to next preset

  bool initDone = false;
  bool enabled = true;

  // strings to reduce flash memory usage (used more than twice)
  static const char _name[];
  static const char _enabled[];
  static const char _DT_pin[];
  static const char _CLK_pin[];
  static const char _SW_pin[];
  static const char _presetHigh[];
  static const char _presetLow[];

  /**
   * Sort the modes and palettes to the index arrays
   * modes_alpha_indexes and palettes_alpha_indexes.
   */
  void sortModesAndPalettes();

  byte *re_initIndexArray(int numModes);

  /**
   * Return an array of mode or palette names from the JSON string.
   * They don't end in '\0', they end in '"'. 
   */
  const char **re_findModeStrings(const char json[], int numModes);

  /**
   * Sort either the modes or the palettes using quicksort.
   */
  void re_sortModes(const char **modeNames, byte *indexes, int count, int numSkip);


public:
  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * You can use it to initialize variables, sensors or similar.
   */
  void setup();

  /*
   * connected() is called every time the WiFi is (re)connected
   * Use it to initialize network interfaces
   */
  void connected();

  /*
   * loop() is called continuously. Here you can check for events, read sensors, etc.
   * 
   * Tips:
   * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
   *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
   * 
   * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
   *    Instead, use a timer check as shown here.
   */
  void loop();

  void findCurrentEffectAndPalette();

  void lampUpdated();

  void changePreset(bool increase) ;

  /*
   * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
   * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
   * Below it is shown how this could be used for e.g. a light sensor
   */
  /*
  void addToJsonInfo(JsonObject& root)
  {
    int reading = 20;
    //this code adds "u":{"Light":[20," lux"]} to the info object
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");
    JsonArray lightArr = user.createNestedArray("Light"); //name
    lightArr.add(reading); //value
    lightArr.add(" lux"); //unit
  }
  */

  /*
   * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
   * Values in the state object may be modified by connected clients
   */
  /*
  void addToJsonState(JsonObject &root)
  {
    //root["user0"] = userVar0;
  }
  */

  /*
   * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
   * Values in the state object may be modified by connected clients
   */
  /*
  void readFromJsonState(JsonObject &root)
  {
    //userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
    //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
  }
  */

  /**
   * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
   */
  void addToConfig(JsonObject &root);

  //WLEDMM: add appendConfigData
  void appendConfigData();

  /**
   * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
   *
   * The function should return true if configuration was successfully loaded or false if there was no configuration.
   */
  bool readFromConfig(JsonObject &root);

  /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
  uint16_t getId()
  {
    return USERMOD_ID_GUITAR_LED;
  }
};

// strings to reduce flash memory usage (used more than twice)
const char GuitarLedController::_name[]       PROGMEM = "Guitar-LED";
const char GuitarLedController::_enabled[]    PROGMEM = "enabled";
const char GuitarLedController::_DT_pin[]     PROGMEM = "DT-pin";
const char GuitarLedController::_CLK_pin[]    PROGMEM = "CLK-pin";
const char GuitarLedController::_SW_pin[]     PROGMEM = "SW-pin";
const char GuitarLedController::_presetHigh[] PROGMEM = "preset-high";
const char GuitarLedController::_presetLow[]  PROGMEM = "preset-low";

/**
 * Sort the modes and palettes to the index arrays
 * modes_alpha_indexes and palettes_alpha_indexes.
 */
void GuitarLedController::sortModesAndPalettes() {
  //modes_qstrings = re_findModeStrings(JSON_mode_names, strip.getModeCount());
  modes_qstrings = strip.getModeDataSrc();
  modes_alpha_indexes = re_initIndexArray(strip.getModeCount());
  re_sortModes(modes_qstrings, modes_alpha_indexes, strip.getModeCount(), MODE_SORT_SKIP_COUNT);

  palettes_qstrings = re_findModeStrings(JSON_palette_names, strip.getPaletteCount());
  palettes_alpha_indexes = re_initIndexArray(strip.getPaletteCount());  // only use internal palettes

  // How many palette names start with '*' and should not be sorted?
  // (Also skipping the first one, 'Default').
  int skipPaletteCount = 1;
  while (pgm_read_byte_near(palettes_qstrings[skipPaletteCount++]) == '*') ;
  re_sortModes(palettes_qstrings, palettes_alpha_indexes, strip.getPaletteCount(), skipPaletteCount);
}

byte *GuitarLedController::re_initIndexArray(int numModes) {
  byte *indexes = (byte *)malloc(sizeof(byte) * numModes);
  for (byte i = 0; i < numModes; i++) {
    indexes[i] = i;
  }
  return indexes;
}

/**
 * Return an array of mode or palette names from the JSON string.
 * They don't end in '\0', they end in '"'. 
 */
const char **GuitarLedController::re_findModeStrings(const char json[], int numModes) {
  const char **modeStrings = (const char **)malloc(sizeof(const char *) * numModes);
  uint8_t modeIndex = 0;
  bool insideQuotes = false;
  // advance past the mark for markLineNum that may exist.
  char singleJsonSymbol;

  // Find the mode name in JSON
  bool complete = false;
  for (size_t i = 0; i < strlen_P(json); i++) {
    singleJsonSymbol = pgm_read_byte_near(json + i);
    if (singleJsonSymbol == '\0') break;
    switch (singleJsonSymbol) {
      case '"':
        insideQuotes = !insideQuotes;
        if (insideQuotes) {
          // We have a new mode or palette
          modeStrings[modeIndex] = (char *)(json + i + 1);
        }
        break;
      case '[':
        break;
      case ']':
        if (!insideQuotes) complete = true;
        break;
      case ',':
        if (!insideQuotes) modeIndex++;
      default:
        if (!insideQuotes) break;
    }
    if (complete) break;
  }
  return modeStrings;
}

/**
 * Sort either the modes or the palettes using quicksort.
 */
void GuitarLedController::re_sortModes(const char **modeNames, byte *indexes, int count, int numSkip) {
  if (!modeNames) return;
  listBeingSorted = modeNames;
  qsort(indexes + numSkip, count - numSkip, sizeof(byte), re_qstringCmp);
  listBeingSorted = nullptr;
}


// public:
/*
  * setup() is called once at boot. WiFi is not yet connected at this point.
  * You can use it to initialize variables, sensors or similar.
  */
void GuitarLedController::setup()
{
  DEBUG_PRINTLN(F("Guitar LED Controller init."));
  PinManagerPinType pins[3] = { { pinA, false }, { pinB, false }, { pinC, false } };
  if ((pinA < 0) || (pinB < 0)) {                                       //WLEDMM catch error: [  1839][E][esp32-hal-gpio.c:102] __pinMode(): Invalid pin selected
    enabled = false;
    DEBUG_PRINTLN(F("Invalid GPIO pins for Guitar LED Controller."));   //WLEDMM add debug info
    return;      
  }
  if (!enabled) return;     // WLEDMM don't allocated PINS if disabled
  if (!pinManager.allocateMultiplePins(pins, 3, PinOwner::UM_GuitarLed)) {
    // BUG: configuring this usermod with conflicting pins
    //      will cause it to de-allocate pins it does not own
    //      (at second config)
    //      This is the exact type of bug solved by pinManager
    //      tracking the owner tags....
    pinA = pinB = pinC = -1;
    enabled = false;
    DEBUG_PRINTLN(F("Failed to allocate GPIO pins for Guitar LED Controller."));   //WLEDMM add debug info
    return;
  }

  #ifndef USERMOD_GUITAR_LED_ENC_GPIO
    #define USERMOD_GUITAR_LED_ENC_GPIO INPUT_PULLUP
  #endif
  pinMode(pinA, USERMOD_GUITAR_LED_ENC_GPIO);
  pinMode(pinB, USERMOD_GUITAR_LED_ENC_GPIO);
  if (pinC >= 0) pinMode(pinC, USERMOD_GUITAR_LED_ENC_GPIO);   // WLEDMM catch error

  loopTime = millis();

  if (!initDone) sortModesAndPalettes();

  initDone = true;
  Enc_A = digitalRead(pinA); // Read encoder pins
  Enc_B = digitalRead(pinB);
  Enc_A_prev = Enc_A;
  USER_PRINTLN(F("Guitar LED setup completed."));   // WLEDMM inform user
}

/*
  * connected() is called every time the WiFi is (re)connected
  * Use it to initialize network interfaces
  */
void GuitarLedController::connected()
{
  //Serial.println("Connected to WiFi!");
}

/*
  * loop() is called continuously. Here you can check for events, read sensors, etc.
  * 
  * Tips:
  * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
  *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
  * 
  * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
  *    Instead, use a timer check as shown here.
  */
void GuitarLedController::loop()
{
  if (!enabled) return;
  unsigned long currentTime = millis(); // get the current elapsed time

  if (strip.isUpdating() && (currentTime - loopTime < 4)) return;  // WLEDMM: be nice, but not too nice

  // Initialize effectCurrentIndex and effectPaletteIndex to
  // current state. We do it here as (at least) effectCurrent
  // is not yet initialized when setup is called.
  
  if (!currentEffectAndPaletteInitialized) {
    findCurrentEffectAndPalette();
  }

  if (modes_alpha_indexes != nullptr) {  // WLEDMM bugfix
    if (modes_alpha_indexes[effectCurrentIndex] != effectCurrent || palettes_alpha_indexes[effectPaletteIndex] != effectPalette) {
      currentEffectAndPaletteInitialized = false;
    }
  }

  if (currentTime - loopTime >= 2) // 2ms since last check of encoder = 500Hz
  {
    loopTime = currentTime; // Updates loopTime

    buttonState = digitalRead(pinC);
    if (prevButtonState != buttonState)
    {
      if (buttonState == LOW)
      {
        prevButtonState = buttonState;
        // Change to the selected preset
        changePreset(moveForward);
      }
      else
      {
        prevButtonState = buttonState;
      }
    }

    Enc_A = digitalRead(pinA); // Read encoder pins
    Enc_B = digitalRead(pinB);
    if ((Enc_A) && (!Enc_A_prev))
    { // A has gone from high to low
      if (Enc_B == LOW)    //changes to LOW so that then encoder registers a change at the very end of a pulse
      { // B is high so clockwise
        // Select next preset
        moveForward = true;
      }
      else if (Enc_B == HIGH)
      { // B is low so counter-clockwise
        // Select previous preset
        moveForward = false;
      }
    }
    Enc_A_prev = Enc_A;     // Store value of A for next time
  }
}

void GuitarLedController::findCurrentEffectAndPalette() {
  if (modes_alpha_indexes == nullptr) return; // WLEDMM bugfix
  currentEffectAndPaletteInitialized = true;
  for (uint8_t i = 0; i < strip.getModeCount(); i++) {
    if (modes_alpha_indexes[i] == effectCurrent) {
      effectCurrentIndex = i;
      break;
    }
  }

  for (uint8_t i = 0; i < strip.getPaletteCount(); i++) {
    if (palettes_alpha_indexes[i] == effectPalette) {
      effectPaletteIndex = i;
      break;
    }
  }
}

void GuitarLedController::lampUpdated() {
  //call for notifier -> 0: init 1: direct change 2: button 3: notification 4: nightlight 5: other (No notification)
  // 6: fx changed 7: hue 8: preset cycle 9: blynk 10: alexa
  //setValuesFromFirstSelectedSeg(); //to make transition work on main segment (should no longer be required)
  stateUpdated(CALL_MODE_BUTTON);
  if ((millis() - lastInterfaceUpdate) > INTERFACE_UPDATE_COOLDOWN)   // WLEDMM respect cooldown times, to avoid crash in AsyncWebSocketMessageBuffer
    updateInterfaces(CALL_MODE_BUTTON);
}

void GuitarLedController::changePreset(bool increase) {
  if (presetHigh && presetLow && presetHigh > presetLow) {
    StaticJsonDocument<64> root;
    char str[64] = { '\0' };
    snprintf_P(str, 64, PSTR("%d~%d~%s"), presetLow, presetHigh, increase?"":"-");
    root["ps"] = str;
    deserializeState(root.as<JsonObject>(), CALL_MODE_BUTTON_PRESET);
/*
    String apireq = F("win&PL=~");
    if (!increase) apireq += '-';
    apireq += F("&P1=");
    apireq += presetLow;
    apireq += F("&P2=");
    apireq += presetHigh;
    handleSet(nullptr, apireq, false);
*/
    lampUpdated();
  }
}

/*
  * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
  * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
  * Below it is shown how this could be used for e.g. a light sensor
  */
/*
void addToJsonInfo(JsonObject& root)
{
  int reading = 20;
  //this code adds "u":{"Light":[20," lux"]} to the info object
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");
  JsonArray lightArr = user.createNestedArray("Light"); //name
  lightArr.add(reading); //value
  lightArr.add(" lux"); //unit
}
*/

/*
  * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
  * Values in the state object may be modified by connected clients
  */
/*
void addToJsonState(JsonObject &root)
{
  //root["user0"] = userVar0;
}
*/

/*
  * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
  * Values in the state object may be modified by connected clients
  */
/*
void readFromJsonState(JsonObject &root)
{
  //userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
  //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
}
*/

/**
 * addToConfig() (called from set.cpp) stores persistent properties to cfg.json
 */
void GuitarLedController::addToConfig(JsonObject &root) {
  // we add JSON object: {"Guitar-Led":{"DT-pin":12,"CLK-pin":14,"SW-pin":13}}
  JsonObject top = root.createNestedObject(FPSTR(_name)); // usermodname
  top[FPSTR(_enabled)] = enabled;
  top[FPSTR(_DT_pin)]  = pinA;
  top[FPSTR(_CLK_pin)] = pinB;
  top[FPSTR(_SW_pin)]  = pinC;
  top[FPSTR(_presetLow)]  = presetLow;
  top[FPSTR(_presetHigh)] = presetHigh;
  DEBUG_PRINTLN(F("Guitar LED config saved."));
}

//WLEDMM: add appendConfigData
void GuitarLedController::appendConfigData()
{
  oappend(SET_F("addHB('Guitar-Led');"));

  #ifdef ENCODER_DT_PIN
    oappend(SET_F("xOpt('Guitar-Led:DT-pin',1,' ⎌',")); oappendi(ENCODER_DT_PIN); oappend(");"); 
  #endif
  #ifdef ENCODER_CLK_PIN
    oappend(SET_F("xOpt('Guitar-Led:CLK-pin',1,' ⎌',")); oappendi(ENCODER_CLK_PIN); oappend(");"); 
  #endif
  #ifdef ENCODER_SW_PIN
    oappend(SET_F("xOpt('Guitar-Led:SW-pin',1,' ⎌',")); oappendi(ENCODER_SW_PIN); oappend(");"); 
  #endif
}

/**
 * readFromConfig() is called before setup() to populate properties from values stored in cfg.json
 *
 * The function should return true if configuration was successfully loaded or false if there was no configuration.
 */
bool GuitarLedController::readFromConfig(JsonObject &root) {
  // we look for JSON object: {"Guitar-Led":{"DT-pin":12,"CLK-pin":14,"SW-pin":13}}
  JsonObject top = root[FPSTR(_name)];
  if (top.isNull()) {
    DEBUG_PRINT(FPSTR(_name));
    DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
    return false;
  }
  int8_t newDTpin  = top[FPSTR(_DT_pin)]  | pinA;
  int8_t newCLKpin = top[FPSTR(_CLK_pin)] | pinB;
  int8_t newSWpin  = top[FPSTR(_SW_pin)]  | pinC;

  presetHigh = top[FPSTR(_presetHigh)] | presetHigh;
  presetLow  = top[FPSTR(_presetLow)]  | presetLow;
  presetHigh = MIN(250,MAX(0,presetHigh));
  presetLow  = MIN(250,MAX(0,presetLow));

  enabled    = top[FPSTR(_enabled)] | enabled;

  DEBUG_PRINT(FPSTR(_name));
  if (!initDone) {
    // first run: reading from cfg.json
    pinA = newDTpin;
    pinB = newCLKpin;
    pinC = newSWpin;
    DEBUG_PRINTLN(F(" config loaded."));
  } else {
    DEBUG_PRINTLN(F(" config (re)loaded."));
    // changing parameters from settings page
    if (pinA!=newDTpin || pinB!=newCLKpin || pinC!=newSWpin) {
      pinManager.deallocatePin(pinA, PinOwner::UM_GuitarLed);
      pinManager.deallocatePin(pinB, PinOwner::UM_GuitarLed);
      pinManager.deallocatePin(pinC, PinOwner::UM_GuitarLed);
      pinA = newDTpin;
      pinB = newCLKpin;
      pinC = newSWpin;
      if (pinA<0 || pinB<0) { // WLEDMM support for rotary without pushbutton
        enabled = false;
        return true;
      }
      if (enabled) setup();   // WLEDMM no pin stealing!
    }
  }
  // use "return !top["newestParameter"].isNull();" when updating Usermod with new features
  return !top[FPSTR(_presetHigh)].isNull();
}
