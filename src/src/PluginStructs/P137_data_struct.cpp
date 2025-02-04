#include "../PluginStructs/P137_data_struct.h"

#ifdef USES_P137

# ifdef ESP32

int16_t P137_settingToValue(uint16_t data,
                            uint16_t threshold) {
  if (data > threshold) {
    return threshold - data;
  }
  return data;
}

uint16_t P137_valueToSetting(int      data,
                             uint16_t threshold) {
  if (data <= -1) {
    return threshold - data;
  }
  return static_cast<uint16_t>(data);
}

/**
 * Apply default parameters based on P137_CONFIG_PREDEFINED setting.
 * This will be 0 by default, so nothing is changed.
 * P137_CONFIG_DISABLEBITS: (set to 1 to disable that setting)
 * Bit = Feature
 * 0   = LDO2
 * 1   = LDO3
 * 2   = LDOIO
 * 3   = GPIO0
 * 4   = GPIO1
 * 5   = GPIO2
 * 6   = GPIO3
 * 7   = GPIO4
 * 8   = DCDC2
 * 9   = DCDC3
 */
void P137_CheckPredefinedParameters(struct EventStruct *event) {
  if (P137_CONFIG_PREDEFINED > 0) {
    P137_CURRENT_PREDEFINED = P137_CONFIG_PREDEFINED;

    // Set defaults
    for (int i = 0; i < 5; ++i) { // GPI0..4
      P137_SET_GPIO_FLAGS(i, static_cast<uint8_t>(P137_GPIOBootState_e::Default));
    }

    switch (static_cast<P137_PredefinedDevices_e>(P137_CONFIG_PREDEFINED)) {
      case P137_PredefinedDevices_e::M5Stack_StickC: // M5Stack StickC
      {
        P137_REG_DCDC2_LDO2     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC2) << 16) | P137_valueToSetting(3000, P137_CONST_MAX_LDO);
        P137_REG_DCDC3_LDO3     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC) << 16) | P137_valueToSetting(3000, P137_CONST_MAX_LDO);
        P137_REG_LDOIO          =  P137_valueToSetting(2800, P137_CONST_MAX_LDOIO);
        P137_CONFIG_DISABLEBITS = 0b1111110000;     // NC pins disabled
        break;
      }
      case P137_PredefinedDevices_e::M5Stack_Core2: // M5Stack Core2
      {
        P137_REG_DCDC2_LDO2     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC2) << 16) | P137_valueToSetting(3300, P137_CONST_MAX_LDO);
        P137_REG_DCDC3_LDO3     = (P137_valueToSetting(0, P137_CONST_MAX_DCDC) << 16) | P137_valueToSetting(0, P137_CONST_MAX_LDO);
        P137_REG_LDOIO          =  P137_valueToSetting(3300, P137_CONST_MAX_LDOIO);
        P137_CONFIG_DISABLEBITS = 0b0101000000; // NC pins disabled
        // Specials: GPIO1 High = LED off, GPIO4 High = Enable TFT
        P137_SET_GPIO_FLAGS(1, static_cast<uint8_t>(P137_GPIOBootState_e::Output_high));
        P137_SET_GPIO_FLAGS(4, static_cast<uint8_t>(P137_GPIOBootState_e::Output_high));
        break;
      }
      case P137_PredefinedDevices_e::LilyGO_TBeam: // LilyGO T-Beam
      {
        P137_REG_DCDC2_LDO2     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC2) << 16) | P137_valueToSetting(3300, P137_CONST_MAX_LDO);
        P137_REG_DCDC3_LDO3     = (P137_valueToSetting(3300, P137_CONST_MAX_DCDC) << 16) | P137_valueToSetting(3300, P137_CONST_MAX_LDO);
        P137_REG_LDOIO          =  P137_valueToSetting(3300, P137_CONST_MAX_LDOIO);
        P137_CONFIG_DISABLEBITS = 0b1111111000;   // NC pins disabled
        break;
      }
      case P137_PredefinedDevices_e::UserDefined: // User defined
      {
        P137_REG_DCDC2_LDO2     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC2) << 16) | P137_valueToSetting(3300, P137_CONST_MAX_LDO);
        P137_REG_DCDC3_LDO3     = (P137_valueToSetting(-1, P137_CONST_MAX_DCDC) << 16) | P137_valueToSetting(-1, P137_CONST_MAX_LDO);
        P137_REG_LDOIO          = P137_valueToSetting(-1, P137_CONST_MAX_LDOIO);
        P137_CONFIG_DISABLEBITS = 0b0000000000; // All pins enabled
        break;
      }
      case P137_PredefinedDevices_e::Unselected:
        // Fall through
        break;
    }
  }
}

// **************************************************************************/
// toString: convert P137_valueOptions_e enum to value-name or display-name
// **************************************************************************/
const __FlashStringHelper* toString(const P137_valueOptions_e value,
                                    bool                      displayString) {
  switch (value) {
    case P137_valueOptions_e::None: return displayString ? F("None") : F("none");
    case P137_valueOptions_e::BatteryVoltage: return displayString ? F("Battery voltage") : F("batteryvoltage");
    case P137_valueOptions_e::BatteryDischargeCurrent: return displayString ? F("Battery discharge current") : F("batterydischargecurrent");
    case P137_valueOptions_e::BatteryChargeCurrent: return displayString ? F("Battery charge current") : F("batterychargecurrent");
    case P137_valueOptions_e::BatteryPower: return displayString ? F("Battery power") : F("batterypower");
    case P137_valueOptions_e::AcinVoltage: return displayString ? F("Input voltage") : F("inputvoltage");
    case P137_valueOptions_e::AcinCurrent: return displayString ? F("Input current") : F("inputcurrent");
    case P137_valueOptions_e::VbusVoltage: return displayString ? F("VBus voltage") : F("vbusvoltage");
    case P137_valueOptions_e::VbusCurrent: return displayString ? F("VBus current") : F("vbuscurrent");
    case P137_valueOptions_e::InternalTemperature: return displayString ? F("Internal temperature") : F("internaltemperature");
    case P137_valueOptions_e::ApsVoltage: return displayString ? F("APS Voltage") : F("apsvoltage");
    case P137_valueOptions_e::LDO2: return displayString ? F("LDO2 Voltage") : F("ldo2voltage");
    case P137_valueOptions_e::LDO3: return displayString ? F("LDO3 Voltage") : F("ldo3voltage");
    case P137_valueOptions_e::LDOIO: return displayString ? F("LDOIO Voltage") : F("gpiovoltage");
    case P137_valueOptions_e::DCDC2: return displayString ? F("DCDC2 Voltage") : F("dcdc2voltage");
    case P137_valueOptions_e::DCDC3: return displayString ? F("DCDC3 Voltage") : F("dcdc3voltage");
  }
  return F("*Undefined*");
}

// **************************************************************************/
// toString: convert P137_GPIOBootState_e enum to string
// **************************************************************************/
const __FlashStringHelper* toString(const P137_GPIOBootState_e value) {
  switch (value) {
    case P137_GPIOBootState_e::Default: return F("Default");
    case P137_GPIOBootState_e::Output_low: return F("Output, low");
    case P137_GPIOBootState_e::Output_high: return F("Output, high");
    case P137_GPIOBootState_e::Input: return F("Input");
    case P137_GPIOBootState_e::PWM: return F("PWM");
  }
  return F("*Undefined*");
}

// **************************************************************************/
// toString: convert P137_PredefinedDevices_e enum to string
// **************************************************************************/
const __FlashStringHelper* toString(const P137_PredefinedDevices_e device) {
  switch (device) {
    case P137_PredefinedDevices_e::Unselected: return F("Select an option to set default values");
    case P137_PredefinedDevices_e::M5Stack_StickC: return F("M5Stack StickC");
    case P137_PredefinedDevices_e::M5Stack_Core2: return F("M5Stack Core2 v1.0 (Default)");
    case P137_PredefinedDevices_e::LilyGO_TBeam: return F("LilyGO T-Beam v1.1");
    case P137_PredefinedDevices_e::UserDefined: return F("User defined");
  }
  return F("*Undefined*");
}

// **************************************************************************/
// Constructor
// **************************************************************************/
P137_data_struct::P137_data_struct(struct EventStruct *event) {
  axp192 = new (std::nothrow) I2C_AXP192();                // Default address and I2C Wire object

  if (isInitialized()) {                                   // Functions based on:
    I2C_AXP192_InitDef initDef = {                         // M5Stack StickC / M5Stack Core2 / LilyGO T-Beam
      .EXTEN  = true,                                      // Enable ESP Power
      .BACKUP = true,                                      // Enable RTC power
      .DCDC1  = 3300,                                      // ESP Power      / ESP Power (Fixed)  / OLed
      .DCDC2  = P137_GET_CONFIG_DCDC2,                     // Unused         / Unused             / Unused
      .DCDC3  = P137_GET_CONFIG_DCDC3,                     // Unused         / LCD Backlight      / ESP Power
      .LDO2   = P137_GET_CONFIG_LDO2,                      // Backlight power (3000 mV) / Periferal VDD   / LoRa
      .LDO3   = P137_GET_CONFIG_LDO3,                      // TFT Power (3000 mV)       / Vibration motor / GPS
      .LDOIO  = P137_GET_CONFIG_LDOIO,                     // LDOIO voltage (2800 mV)
      .GPIO0  = static_cast<int>(P137_GET_FLAG_GPIO0 - 1), // Microphone power / Bus pwr enable / Unused
      .GPIO1  = static_cast<int>(P137_GET_FLAG_GPIO1 - 1), // Unused / Sys Led (green)          / Unused
      .GPIO2  = static_cast<int>(P137_GET_FLAG_GPIO2 - 1), // Unused / Speaker enable           / Unused
      .GPIO3  = static_cast<int>(P137_GET_FLAG_GPIO3 - 1), // Unused / Unused                   / Unused
      .GPIO4  = static_cast<int>(P137_GET_FLAG_GPIO4 - 1), // Unused / TFT Reset                / Unused
    };
    ldo2_value  = P137_GET_CONFIG_LDO2;
    ldo3_value  = P137_GET_CONFIG_LDO3;
    ldoio_value = P137_GET_CONFIG_LDOIO;
    dcdc2_value = P137_GET_CONFIG_DCDC2;
    dcdc3_value = P137_GET_CONFIG_DCDC3;

    axp192->begin(initDef);
  }
}

// **************************************************************************/
// Destructor
// **************************************************************************/
P137_data_struct::~P137_data_struct() {
  delete axp192;
}

// **************************************************************************/
// plugin_read: Read the values and send to controller(s)
// **************************************************************************/
bool P137_data_struct::plugin_read(struct EventStruct *event) {
  bool success = true;

  const uint8_t valueCount = P137_NR_OUTPUT_VALUES;

  for (uint8_t i = 0; i < valueCount; ++i) {
    UserVar.setFloat(event->TaskIndex, i, read_value(static_cast<P137_valueOptions_e>(PCONFIG(P137_CONFIG_BASE + i))));
  }

  return success;
}

// **************************************************************************/
// read_value: Read the requested value
// **************************************************************************/
float P137_data_struct::read_value(P137_valueOptions_e value) {
  if (isInitialized()) {
    switch (value) {
      case P137_valueOptions_e::None: return 0.0f;
      case P137_valueOptions_e::BatteryVoltage: return axp192->getBatteryVoltage();
      case P137_valueOptions_e::BatteryDischargeCurrent: return axp192->getBatteryDischargeCurrent();
      case P137_valueOptions_e::BatteryChargeCurrent: return axp192->getBatteryChargeCurrent();
      case P137_valueOptions_e::BatteryPower: return axp192->getBatteryPower();
      case P137_valueOptions_e::AcinVoltage: return axp192->getAcinVolatge();
      case P137_valueOptions_e::AcinCurrent: return axp192->getAcinCurrent();
      case P137_valueOptions_e::VbusVoltage: return axp192->getVbusVoltage();
      case P137_valueOptions_e::VbusCurrent: return axp192->getVbusCurrent();
      case P137_valueOptions_e::InternalTemperature: return axp192->getInternalTemperature();
      case P137_valueOptions_e::ApsVoltage: return axp192->getApsVoltage();
      case P137_valueOptions_e::LDO2: return static_cast<float>(ldo2_value);
      case P137_valueOptions_e::LDO3: return static_cast<float>(ldo3_value);
      case P137_valueOptions_e::LDOIO: return static_cast<float>(ldoio_value);
      case P137_valueOptions_e::DCDC2: return static_cast<float>(dcdc2_value);
      case P137_valueOptions_e::DCDC3: return static_cast<float>(dcdc3_value);
    }
  }
  return 0.0f;
}

// **************************************************************************/
// plugin_write: Process commands
// **************************************************************************/
const char P137_subcommands[] PROGMEM = "ldo2|ldo3|ldoio|gpio0|gpio1|gpio2|gpio3|gpio4|dcdc2|dcdc3|ldo2perc|lco3perc|ldoioperc|"
                                        "dcdc2perc|dcdc3pers|ldo2map|lco3map|ldoiomap|dcdc2map|dcdc3map|";
enum class P137_subcommands_e : int8_t {
  invalid = -1,
  ldo2    = 0,
  ldo3,
  ldoio,
  gpio0,
  gpio1,
  gpio2,
  gpio3,
  gpio4,
  dcdc2,
  dcdc3,
  ldo2perc,
  ldo3perc,
  ldoioperc,
  dcdc2perc,
  dcdc3perc,
  ldo2map,
  ldo3map,
  ldoiomap,
  dcdc2map,
  dcdc3map,
};

bool P137_data_struct::plugin_write(struct EventStruct *event,
                                    const String      & string) {
  bool   success = false;
  String cmd     = parseString(string, 1);

  if (isInitialized() && equals(cmd, F("axp"))) { // Command trigger
    cmd = parseString(string, 2);                 // sub command
    const int subcommand_i          = GetCommandCode(cmd.c_str(), P137_subcommands);
    const P137_subcommands_e subcmd = static_cast<P137_subcommands_e>(subcommand_i);

    const String var3   = parseString(string, 3);
    const bool   empty3 = var3.isEmpty();
    const bool   empty4 = parseString(string, 4).isEmpty();
    const bool   state3 = !empty3 && (event->Par2 == 0 || event->Par2 == 1);
    success = true;

    if ((event->Par2 >= 0) && (event->Par2 <= P137_CONST_100_PERCENT) && !empty3 && empty4) {
      // percentage 0..100, 0 turns off

      switch (subcmd) {
        case P137_subcommands_e::ldo2perc:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 0)) { // axp,ldo2perc,<percentage>
            ldo2_value = event->Par2 > 0 ? map(event->Par2,
                                               P137_CONST_1_PERCENT, P137_CONST_100_PERCENT,
                                               ldo2_range[0], ldo2_range[1]) : 0;
            axp192->setLDO2(ldo2_value);
          }
          break;
        case P137_subcommands_e::ldo3perc:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 1)) { // axp,ldo3perc,<percentage>
            ldo3_value = event->Par2 > 0 ? map(event->Par2,
                                               P137_CONST_1_PERCENT, P137_CONST_100_PERCENT,
                                               ldo3_range[0], ldo3_range[1]) : 0;
            axp192->setLDO3(ldo3_value);
          }
          break;
        case P137_subcommands_e::ldoioperc:

          if ((event->Par2 > 0) &&
              !bitRead(P137_CONFIG_DISABLEBITS, 2)) { // axp,ldoioperc,<percentage>
            ldoio_value = map(event->Par2,
                              P137_CONST_1_PERCENT, P137_CONST_100_PERCENT,
                              ldoio_range[0], ldoio_range[1]);
            axp192->setLDOIO(ldoio_value);
          }
          break;
        case P137_subcommands_e::dcdc2perc:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 8)) { // axp,dcdc2perc,<percentage>
            dcdc2_value = event->Par2 > 0 ? map(event->Par2,
                                                P137_CONST_1_PERCENT, P137_CONST_100_PERCENT,
                                                dcdc2_range[0], dcdc2_range[1]) : 0;
            axp192->setDCDC2(dcdc2_value);
          }
          break;
        case P137_subcommands_e::dcdc3perc:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 9)) { // axp,dcdc3perc,<percentage>
            dcdc3_value = event->Par2 > 0 ? map(event->Par2,
                                                P137_CONST_1_PERCENT, P137_CONST_100_PERCENT,
                                                dcdc3_range[0], dcdc3_range[1]) : 0;
            axp192->setDCDC3(dcdc3_value);
          }
          break;
        default:
          success = false;
          break;
      }
    } else if ((event->Par2 >= 0) && (event->Par3 <= P137_CONST_MAX_LDO) && (event->Par2 < event->Par3) && !empty3 && !empty4) {
      // map range <low>,<high>

      switch (subcmd) {
        case P137_subcommands_e::ldo2map:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 0)) { // axp,ldo2map,<low>,<high>
            ldo2_range[0] = event->Par2;
            ldo2_range[1] = event->Par3;
          }
          break;
        case P137_subcommands_e::ldo3map:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 1)) { // axp,ldo3map,<low>,<high>
            ldo3_range[0] = event->Par2;
            ldo3_range[1] = event->Par3;
          }
          break;
        case P137_subcommands_e::ldoiomap:

          if ((event->Par2 >= P137_CONST_MIN_LDOIO) &&
              !bitRead(P137_CONFIG_DISABLEBITS, 2)) { // axp,ldoiomap,<low>,<high>
            ldoio_range[0] = event->Par2;
            ldoio_range[1] = event->Par3;
          }
          break;
        case P137_subcommands_e::dcdc2map:

          if ((event->Par3 <= P137_CONST_MAX_DCDC2) &&
              !bitRead(P137_CONFIG_DISABLEBITS, 8)) { // axp,dcdc2map,<low>,<high>
            dcdc2_range[0] = event->Par2;
            dcdc2_range[1] = event->Par3;
          }
          break;
        case P137_subcommands_e::dcdc3map:

          if (!bitRead(P137_CONFIG_DISABLEBITS, 9)) { // axp,dcdc3map,<low>,<high>
            dcdc3_range[0] = event->Par2;
            dcdc3_range[1] = event->Par3;
          }
          break;
        default:
          success = false;
          break;
      }
    } else {
      if (bitRead(P137_CONFIG_DISABLEBITS, static_cast<uint8_t>(subcmd))) {
        success = false;
      } else {
        switch (subcmd) {
          case P137_subcommands_e::ldo2:

            if (!empty3) {                  // axp,ldo2,<voltage>
              axp192->setLDO2(event->Par2); // Range checking done by function
              ldo2_value = event->Par2;
            }
            break;
          case P137_subcommands_e::ldo3:

            if (!empty3) {                  // axp,ldo3,<voltage>
              axp192->setLDO3(event->Par2); // Range checking done by function
              ldo3_value = event->Par2;
            }
            break;
          case P137_subcommands_e::ldoio:

            if (!empty3) {                   // axp,ldoio,<voltage>
              axp192->setLDOIO(event->Par2); // Range checking done by function
              ldoio_value = event->Par2;
            }
            break;
          case P137_subcommands_e::gpio0:

            if (state3) {                    // axp,gpio0,state
              axp192->setGPIO0(event->Par2); // Range checking done before
            }
            break;
          case P137_subcommands_e::gpio1:

            if (state3) {                    // axp,gpio1,state
              axp192->setGPIO1(event->Par2); // Range checking done before
            }
            break;
          case P137_subcommands_e::gpio2:

            if (state3) {                    // axp,gpio2,state
              axp192->setGPIO2(event->Par2); // Range checking done before
            }
            break;
          case P137_subcommands_e::gpio3:

            if (state3) {                    // axp,gpio3,state
              axp192->setGPIO3(event->Par2); // Range checking done before
            }
            break;
          case P137_subcommands_e::gpio4:

            if (state3) {                    // axp,gpio4,state
              axp192->setGPIO4(event->Par2); // Range checking done before
            }
            break;
          case P137_subcommands_e::dcdc2:

            if (!empty3) {                   // axp,dcdc2,<voltage>
              axp192->setDCDC2(event->Par2); // Range checking done by function
              dcdc2_value = event->Par2;
            }
            break;
          case P137_subcommands_e::dcdc3:

            if (!empty3) {                   // axp,dcdc3,<voltage>
              axp192->setDCDC3(event->Par2); // Range checking done by function
              dcdc3_value = event->Par2;
            }
            break;
          default:
            success = false;
            break;
        }
      }
    }
  }

  return success;
}

/****************************************************************************
 * plugin_get_config_value: Retrieve values like [<taskname>#<valuename>]
 ***************************************************************************/
const char P137_getvalues[] PROGMEM =
  "batvoltage"    //  BatteryVoltage          = 0x01,
  "|batdischarge" //  BatteryDischargeCurrent = 0x02,
  "|batcharge"    //  BatteryChargeCurrent    = 0x03,
  "|batpower"     //  BatteryPower            = 0x04,
  "|inpvoltage"   //  AcinVoltage             = 0x05,
  "|inpcurrent"   //  AcinCurrent             = 0x06,
  "|vbusvolt"     //  VbusVoltage             = 0x07,
  "|vbuscurr"     //  VbusCurrent             = 0x08,
  "|inttemp"      //  InternalTemperature     = 0x09,
  "|apsvolt"      //  ApsVoltage              = 0x0A,
  "|ldo2volt"     //  LDO2                    = 0x0B,
  "|ldo3volt"     //  LDO3                    = 0x0C,
  "|ldoiovolt"    //  LDOIO                   = 0x0D,
  "|dcdc2volt"    //  DCDC2                   = 0x12,
  "|dcdc3volt"    //  DCDC3                   = 0x13,
;


bool P137_data_struct::plugin_get_config_value(struct EventStruct *event,
                                               String            & string)
{
  const String command = parseString(string, 1);
  int getvalue_i       = GetCommandCode(command.c_str(), P137_getvalues);

  if (getvalue_i == -1) { return false; }

  // P137_valueOptions_e do not enumerate without a gap, so we need to 'patch' the index
  if (getvalue_i == 13) {
    getvalue_i = static_cast<int>(P137_valueOptions_e::DCDC2);
  }  else if (getvalue_i == 14) {
    getvalue_i = static_cast<int>(P137_valueOptions_e::DCDC3);
  }  else {
    ++getvalue_i; // Don't match with "None"
  }
  const P137_valueOptions_e getval = static_cast<P137_valueOptions_e>(getvalue_i);

  string = toString(read_value(getval), static_cast<unsigned int>(P137_CONFIG_DECIMALS));
  return true;
}

# endif // ifdef ESP32
#endif  // ifdef USES_P137
