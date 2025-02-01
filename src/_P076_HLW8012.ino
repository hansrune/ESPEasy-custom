#include "_Plugin_Helper.h"
#ifdef USES_P076

// #######################################################################################################
// #################### Plugin 076 HLW8012 AC Current and Voltage measurement sensor #####################
// #######################################################################################################
//
// This plugin is interfacing with HLW8012 and BL0937 IC which is use with some
// commercial devices like:
// -- Sonoff POW
// -- ElectroDragon HLW8012 Breakout board
// --- and more
//
// The Sonoff POW uses the following PINs: SEL=GPIO05(D1), CF1=GPIO13(D8),
// CF=GPIO14(D5)
// The ED Module has pinheaders so any available PIN on the ESP8266 can be used.
//
// HLW8012 IC works with 5VDC (it seems at 3.3V is not stable in reading)
//

/** Changelog:
 * 2024-12-26 tonhuisman: Add preset for Shelly Plus PLUG-S (ESP32 device), only enabled for ESP32 builds
 *                        Preset data provided by 'dsiggy' in https://www.letscontrolit.com/forum/viewtopic.php?t=10503#p71477
 * 2024-12-01 dsiggi: - The old measurement values (voltage, current and power factor) are now again the default to support old setups.
 *                    - Removed spaces from value names
 *                    - Seperated Output selector string an value string
 *                    - Uncrustify
 * 2024-11-24 dsiggi: - Added the ability to read reactive power, apparent power and energy
 *                    - Added ability to reset the energy counter with console command hlwresetenergy
 * 2023-01-03 tonhuisman: Uncrustify source, apply some code improvements
 *                        Older changelog not registered.
 */

#include <HLW8012.h>

HLW8012 *Plugin_076_hlw = nullptr;

#if FEATURE_PLUGIN_STATS
#include "src/DataStructs/PluginTaskData_base.h"
#endif // if FEATURE_PLUGIN_STATS
#include "src/Helpers/OversamplingHelper.h"

#define PLUGIN_076
#define PLUGIN_ID_076 76
#define PLUGIN_076_DEBUG false // activate extra log info in the debug
#define PLUGIN_NAME_076 "Energy (AC) - HLW8012/BL0937"

#define HLW_DELAYREADING 2000

// These are the nominal values for the resistors in the circuit
#define HLW_CURRENT_RESISTOR 0.001
#define HLW_VOLTAGE_RESISTOR_UP (5 * 470000) // Real: 2280k
#define HLW_VOLTAGE_RESISTOR_DOWN (1000)     // Real 1.009k
// -----------------------------------------------------------------------------------------------

#define P076_INDEX_VOLT 0
#define P076_INDEX_CURR 1
#define P076_INDEX_POWR 2
#define P076_INDEX_VAR 3
#define P076_INDEX_VA 4
#define P076_INDEX_PF 5
#define P076_INDEX_ENER 6

#define P076_NR_OUTPUT_VALUES 4
#define P076_NR_OUTPUT_OPTIONS 7
#define P076_QUERY1_CONFIG_POS 0
#define P076_QUERY1 PCONFIG(0)
#define P076_QUERY2 PCONFIG(1)
#define P076_QUERY3 PCONFIG(2)
#define P076_QUERY4 PCONFIG(3)
#define P076_QUERY1_DFLT P076_INDEX_VOLT // Voltage (V)
#define P076_QUERY2_DFLT P076_INDEX_CURR // Current (A)
#define P076_QUERY3_DFLT P076_INDEX_POWR // Active Power (W)
#define P076_QUERY4_DFLT P076_INDEX_PF   // Power Factor (cosphi)

#define P076_SEL_CUR_READ PCONFIG(4)
#define P076_CF_TRIGGER PCONFIG(5)
#define P076_CF1_TRIGGER PCONFIG(6)
#define P076_PREDEFINED PCONFIG(7)

#define P076_FLAGS PCONFIG_ULONG(0)
#define P076_FLAG_TOZERO 0
#define P076_TOZERO bitRead(P076_FLAGS, P076_FLAG_TOZERO)

#define P076_Custom 0

// HLW8012 Devices
#define P076_Sonoff 1
#define P076_Huafan 2
#define P076_KMC 3
#define P076_Aplic 4
#define P076_SK03 5

// BL093 Devices
#define P076_BlitzWolf 6
#define P076_Teckin 7
#define P076_TeckinUS 8
#define P076_Gosund 9
#define P076_Shelly_PLUG_S 10
#define P076_Shelly_Plus_PLUG_S 11

OversamplingHelper<float> p076_values[P076_NR_OUTPUT_OPTIONS]{};
int StoredTaskIndex = -1;
uint8_t p076_read_stage{};
unsigned long p076_timer{};

// Forward declaration helper function
const __FlashStringHelper *p076_getQueryString(uint8_t value_nr,
                                               bool displayString);

#if ESP_IDF_VERSION_MAJOR >= 5

// FIXME TD-er: Must check if older (and ESP8266) envs need IRAM_ATTR in the function declaration.
void p076_hlw8012_cf1_interrupt();
void p076_hlw8012_cf_interrupt();
#else  // if ESP_IDF_VERSION_MAJOR >= 5
void IRAM_ATTR p076_hlw8012_cf1_interrupt();
void IRAM_ATTR p076_hlw8012_cf_interrupt();
#endif // if ESP_IDF_VERSION_MAJOR >= 5

bool p076_getDeviceParameters(int device,
                              uint8_t &SEL_Pin,
                              uint8_t &CF_Pin,
                              uint8_t &CF1_Pin,
                              uint8_t &Cur_read,
                              uint8_t &CF_Trigger,
                              uint8_t &CF1_Trigger)
{
  switch (device)
  {
  case P076_Custom:
    SEL_Pin = 0;
    CF_Pin = 0;
    CF1_Pin = 0;
    Cur_read = LOW;
    CF_Trigger = LOW;
    CF1_Trigger = LOW;
    break;
  case P076_Sonoff:
    SEL_Pin = 5;
    CF_Pin = 14;
    CF1_Pin = 13;
    Cur_read = HIGH;
    CF_Trigger = CHANGE;
    CF1_Trigger = CHANGE;
    break;
  case P076_Huafan:
    SEL_Pin = 13;
    CF_Pin = 14;
    CF1_Pin = 12;
    Cur_read = HIGH;
    CF_Trigger = CHANGE;
    CF1_Trigger = CHANGE;
    break;
  case P076_KMC:
    SEL_Pin = 12;
    CF_Pin = 4;
    CF1_Pin = 5;
    Cur_read = HIGH;
    CF_Trigger = CHANGE;
    CF1_Trigger = CHANGE;
    break;
  case P076_Aplic: // SEL_Pin = 12; CF_Pin =  4; CF1_Pin =  5; Cur_read =  LOW; CF_Trigger =  CHANGE; CF1_Trigger = CHANGE; break;
  case P076_SK03:
    SEL_Pin = 12;
    CF_Pin = 4;
    CF1_Pin = 5;
    Cur_read = LOW;
    CF_Trigger = CHANGE;
    CF1_Trigger = CHANGE;
    break;
  case P076_BlitzWolf: // SEL_Pin   = 12; CF_Pin =  5; CF1_Pin = 14; Cur_read =  LOW; CF_Trigger = FALLING; CF1_Trigger = CHANGE; break;
  case P076_TeckinUS:  // SEL_Pin    = 12; CF_Pin =  5; CF1_Pin = 14; Cur_read =  LOW; CF_Trigger = FALLING; CF1_Trigger = CHANGE; break;
  case P076_Shelly_PLUG_S:
    SEL_Pin = 12;
    CF_Pin = 5;
    CF1_Pin = 14;
    Cur_read = LOW;
    CF_Trigger = FALLING;
    CF1_Trigger = CHANGE;
    break;
  case P076_Teckin: // SEL_Pin = 12; CF_Pin =  4; CF1_Pin =  5; Cur_read =  LOW; CF_Trigger = FALLING; CF1_Trigger = CHANGE; break;
  case P076_Gosund:
    SEL_Pin = 12;
    CF_Pin = 4;
    CF1_Pin = 5;
    Cur_read = LOW;
    CF_Trigger = FALLING;
    CF1_Trigger = CHANGE;
    break;
#ifdef ESP32
  case P076_Shelly_Plus_PLUG_S:
    SEL_Pin = 19;
    CF_Pin = 10;
    CF1_Pin = 22;
    Cur_read = LOW;
    CF_Trigger = CHANGE;
    CF1_Trigger = CHANGE;
    break;
#endif // ifdef ESP32
  default:
    return false;
  }
  return true;
}

boolean Plugin_076(uint8_t function, struct EventStruct *event, String &string)
{
  boolean success = false;

  switch (function)
  {
  case PLUGIN_DEVICE_ADD:
  {
    auto &dev = Device[++deviceCount];
    dev.Number = PLUGIN_ID_076;
    dev.Type = DEVICE_TYPE_TRIPLE;
    dev.VType = Sensor_VType::SENSOR_TYPE_QUAD;
    dev.FormulaOption = true;
    dev.ValueCount = 4;
    dev.SendDataOption = true;
    dev.TimerOption = true;
    dev.PluginStats = true;
    dev.setPin1Direction(gpio_direction::gpio_output);
    dev.OutputDataType = Output_Data_type_t::Simple;

    //      dev.TaskLogsOwnPeaks = true;
    break;
  }

  case PLUGIN_GET_DEVICENAME:
  {
    string = F(PLUGIN_NAME_076);
    break;
  }

  case PLUGIN_GET_DEVICEVALUENAMES:
  {
    for (uint8_t i = 0; i < VARS_PER_TASK; ++i)
    {
      if (i < P076_NR_OUTPUT_VALUES)
      {
        uint8_t choice = PCONFIG(i + P076_QUERY1_CONFIG_POS);
        ExtraTaskSettings.setTaskDeviceValueName(i, p076_getQueryString(choice, false));
      }
      else
      {
        ExtraTaskSettings.clearTaskDeviceValueName(i);
      }
    }
    break;
  }

  case PLUGIN_SET_DEFAULTS:
  {
    // Load some defaults
    P076_QUERY1 = P076_QUERY1_DFLT;
    P076_QUERY2 = P076_QUERY2_DFLT;
    P076_QUERY3 = P076_QUERY3_DFLT;
    P076_QUERY4 = P076_QUERY4_DFLT;

    success = true;
    break;
  }

  case PLUGIN_GET_DEVICEGPIONAMES:
  {
    event->String1 = formatGpioName_output(F("SEL"));
    event->String2 = formatGpioName_input(F("CF1"));
    event->String3 = formatGpioName_input(F("CF"));
    break;
  }

  case PLUGIN_WEBFORM_LOAD_OUTPUT_SELECTOR:
  {
    // To select the data in the 4 fields.
    const __FlashStringHelper *options[P076_NR_OUTPUT_OPTIONS];

    for (uint8_t i = 0; i < P076_NR_OUTPUT_OPTIONS; ++i)
    {
      options[i] = p076_getQueryString(i, true);
    }

    for (uint8_t i = 0; i < P076_NR_OUTPUT_VALUES; ++i)
    {
      const uint8_t pconfigIndex = i + P076_QUERY1_CONFIG_POS;
      sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P076_NR_OUTPUT_OPTIONS, options);
    }

    success = true;

    break;
  }

  case PLUGIN_WEBFORM_LOAD:
  {
    uint8_t devicePinSettings = P076_PREDEFINED;
    p076_checkdefault_queries(event);

    addFormSubHeader(F("Predefined Pin settings"));
    {
      // Place this in a scope, to keep memory usage low.
      const __FlashStringHelper *predefinedNames[] = {
          F("Custom"),
          F("Sonoff Pow (r1)"),
          F("Huafan SS"),
          F("KMC 70011"),
          F("Aplic WDP303075"),
          F("SK03 Outdoor"),
          F("BlitzWolf SHP"),
          F("Teckin"),
          F("Teckin US"),
          F("Gosund SP1 v23"),
          F("Shelly PLUG-S"),
#ifdef ESP32
          F("Shelly Plus PLUG-S"),
#endif // ifdef ESP32
      };
      const int predefinedId[] = {
          P076_Custom,
          P076_Sonoff,
          P076_Huafan,
          P076_KMC,
          P076_Aplic,
          P076_SK03,
          P076_BlitzWolf,
          P076_Teckin,
          P076_TeckinUS,
          P076_Gosund,
          P076_Shelly_PLUG_S,
#ifdef ESP32
          P076_Shelly_Plus_PLUG_S,
#endif // ifdef ESP32
      };
      constexpr int nrElements = NR_ELEMENTS(predefinedId);
      const FormSelectorOptions selector(nrElements, predefinedNames, predefinedId);
      selector.addFormSelector(F("Device"), F("preDefDevSel"), devicePinSettings);
      addFormNote(F("Enable device and select device type first"));
    }

    {
      // Place this in a scope, to keep memory usage low.
      uint8_t currentRead = P076_SEL_CUR_READ;

      if ((currentRead != LOW) && (currentRead != HIGH))
      {
        currentRead = LOW;
      }
      addFormSubHeader(F("Custom Pin settings (choose Custom above)"));
      {
        const __FlashStringHelper *modeCurr[] = {
            F("LOW"),
            F("HIGH"),
        };

        const int modeCurrValues[] = {
            LOW,
            HIGH,
        };

        const FormSelectorOptions selector(NR_ELEMENTS(modeCurr), modeCurr, modeCurrValues);
        selector.addFormSelector(F("SEL Current (A) Reading"), F("curr_read"), currentRead);
      }
      {
        const __FlashStringHelper *modeRaise[] = {
            F("LOW"),
            F("CHANGE"),
            F("RISING"),
            F("FALLING"),
        };

        const int modeValues[] = {
            LOW,
            CHANGE,
            RISING,
            FALLING,
        };

        const FormSelectorOptions selector(NR_ELEMENTS(modeRaise), modeRaise, modeValues);
        selector.addFormSelector(F("CF1  Interrupt Edge"), F("cf1_edge"), P076_CF1_TRIGGER);
        selector.addFormSelector(F("CF Interrupt Edge"), F("cf_edge"), P076_CF_TRIGGER);
      }
    }

    ESPEASY_RULES_FLOAT_TYPE current, voltage, power;

    if (Plugin076_LoadMultipliers(event->TaskIndex, current, voltage, power))
    {
      addFormSubHeader(F("Calibration Values"));
      addFormTextBox(F("Current Multiplier"), F("currmult"),
#if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     doubleToString(current, 2)
#else  // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     floatToString(current, 2)
#endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                         ,
                     25);
      addFormTextBox(F("Voltage Multiplier"), F("voltmult"),
#if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     doubleToString(voltage, 2)
#else  // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     floatToString(voltage, 2)
#endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                         ,
                     25);
      addFormTextBox(F("Power Multiplier"), F("powmult"),
#if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     doubleToString(power, 2)
#else  // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                     floatToString(power, 2)
#endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
                         ,
                     25);
    }

    addFormSubHeader(F("Measuring options"));
    addFormCheckBox(F("Set values to zero if nothing is measured"), F("tozero"), P076_TOZERO);

    success = true;
    break;
  }

  case PLUGIN_WEBFORM_SAVE:
  {
    // Set Pin settings
    uint8_t selectedDevice = getFormItemInt(F("preDefDevSel"));

    P076_PREDEFINED = selectedDevice;
    {
      uint8_t SEL_Pin, CF_Pin, CF1_Pin, Cur_read, CF_Trigger, CF1_Trigger;

      if ((selectedDevice != 0) && p076_getDeviceParameters(selectedDevice, SEL_Pin, CF_Pin, CF1_Pin, Cur_read, CF_Trigger, CF1_Trigger))
      {
        P076_SEL_CUR_READ = Cur_read;
        P076_CF_TRIGGER = CF_Trigger;
        P076_CF1_TRIGGER = CF1_Trigger;

        CONFIG_PIN1 = SEL_Pin;
        CONFIG_PIN2 = CF1_Pin;
        CONFIG_PIN3 = CF_Pin;
      }
      else
      {
        P076_SEL_CUR_READ = getFormItemInt(F("curr_read"));
        P076_CF_TRIGGER = getFormItemInt(F("cf_edge"));
        P076_CF1_TRIGGER = getFormItemInt(F("cf1_edge"));
      }
    }

    // Set Multipliers
    ESPEASY_RULES_FLOAT_TYPE hlwMultipliers[3];
    hlwMultipliers[0] = getFormItemFloat(F("currmult"));
    hlwMultipliers[1] = getFormItemFloat(F("voltmult"));
    hlwMultipliers[2] = getFormItemFloat(F("powmult"));

    if ((hlwMultipliers[0] > 1.0) && (hlwMultipliers[1] > 1.0) && (hlwMultipliers[2] > 1.0))
    {
      SaveCustomTaskSettings(event->TaskIndex, reinterpret_cast<const uint8_t *>(&hlwMultipliers),
                             sizeof(hlwMultipliers));
#if PLUGIN_076_DEBUG
      addLog(LOG_LEVEL_INFO, F("P076: Saved Calibration from Config Page"));
#endif // if PLUGIN_076_DEBUG

      if (Plugin_076_hlw)
      {
        Plugin_076_hlw->setCurrentMultiplier(hlwMultipliers[0]);
        Plugin_076_hlw->setVoltageMultiplier(hlwMultipliers[1]);
        Plugin_076_hlw->setPowerMultiplier(hlwMultipliers[2]);
      }

#if PLUGIN_076_DEBUG
      addLog(LOG_LEVEL_INFO, F("P076: Multipliers Reassigned"));
#endif // if PLUGIN_076_DEBUG
    }

#if PLUGIN_076_DEBUG
    addLogMove(LOG_LEVEL_INFO, strformat(F("P076: PIN Settings  curr_read: %d cf_edge: %d cf1_edge: %d"),
                                         P076_SEL_CUR_READ, P076_CF_TRIGGER, P076_CF1_TRIGGER));
#endif // if PLUGIN_076_DEBUG

    // Save output selector parameters.
    for (uint8_t i = 0; i < P076_NR_OUTPUT_VALUES; ++i)
    {
      const uint8_t pconfigIndex = i + P076_QUERY1_CONFIG_POS;
      const uint8_t choice = PCONFIG(pconfigIndex);
      sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, p076_getQueryString(choice, false));
    }

    // Save ToZero-Flag
    bitWrite(P076_FLAGS, P076_FLAG_TOZERO, isFormItemChecked(F("tozero")));

    success = true;
    break;
  }

  case PLUGIN_TEN_PER_SECOND:

    if (Plugin_076_hlw)
    {
      /*
      switch (p076_read_stage) {
        case 0: // Set mode to read current
          Plugin_076_hlw->setMode(MODE_CURRENT);
          p076_timer = millis() + HLW_DELAYREADING;
          ++p076_read_stage;
          break;
        case 1: // Read current + set mode to read voltage

          if (timeOutReached(p076_timer)) {
            bool valid        = false;
            const float value = Plugin_076_hlw->getCurrent(valid);

            if (valid || P076_TOZERO) {
              p076_addValue(event, P076_INDEX_CURR, value);
            }
            Plugin_076_hlw->setMode(MODE_VOLTAGE);
            p076_timer = millis() + HLW_DELAYREADING;
            ++p076_read_stage;
          }
          break;
        case 2: // Read voltage + active power + power factor

          if (timeOutReached(p076_timer)) {
          */
      for (uint8_t i = 0; i < P076_NR_OUTPUT_OPTIONS; ++i)
      {
        bool valid = false;
        float value{};

        switch (i)
        {
        case P076_INDEX_VOLT:
          value = Plugin_076_hlw->getVoltage(valid);
          break;
        case P076_INDEX_CURR:
          value = Plugin_076_hlw->getCurrent(valid);
          break;
        case P076_INDEX_POWR:
          value = Plugin_076_hlw->getActivePower(valid);
          break;
        case P076_INDEX_VAR:
          value = Plugin_076_hlw->getReactivePower(valid);
          break;
        case P076_INDEX_VA:
          value = Plugin_076_hlw->getApparentPower(valid);
          break;
        case P076_INDEX_PF:
          value = static_cast<int>(100 * Plugin_076_hlw->getPowerFactor(valid));
          break;
        case P076_INDEX_ENER:
          value = Plugin_076_hlw->getEnergy();
          valid = true;
          break;
        }

        if (valid || P076_TOZERO)
        {
          p076_addValue(event, i, value);
        }
      }
      /*
      ++p076_read_stage;
    }
    break;
  default:
    p076_read_stage = 0;
    break;
}
*/
    }
    success = true;
    break;

  case PLUGIN_READ:

    if (Plugin_076_hlw)
    {
      for (taskVarIndex_t i = 0; i < 4; ++i)
      {
        const uint8_t index = PCONFIG(i);
        if (index < NR_ELEMENTS(p076_values)) {
          float value{};
          if (p076_values[index].get(value))
          {
            success = true;
            UserVar.setFloat(event->TaskIndex, i, value);
          }
        }
      }

#if PLUGIN_076_DEBUG
      addLogMove(LOG_LEVEL_INFO,
                 strformat(F("P076: Read values - V=%.2f - A=%.2f - W=%.2f - VAR=%.2f - VA=%.2f - Pf%%=%.2f - Ws=%.2f"),
                           p076_values[P076_INDEX_VOLT],
                           p076_values[P076_INDEX_CURR],
                           p076_values[P076_INDEX_POWR],
                           p076_values[P076_INDEX_VAR],
                           p076_values[P076_INDEX_VA],
                           p076_values[P076_INDEX_PF],
                           p076_values[P076_INDEX_ENER]));
#endif // if PLUGIN_076_DEBUG

      // Plugin_076_hlw->toggleMode();
    }
    break;

  case PLUGIN_EXIT:
  {
    Plugin076_Reset(event->TaskIndex);
    success = true;
    break;
  }

  case PLUGIN_INIT:
  {
    Plugin076_Reset(event->TaskIndex);

    // This initializes the HWL8012 library.
    const uint8_t CF_PIN = CONFIG_PIN3;
    const uint8_t CF1_PIN = CONFIG_PIN2;
    const uint8_t SEL_PIN = CONFIG_PIN1;

    if (validGpio(CF_PIN) && validGpio(CF1_PIN) && validGpio(SEL_PIN))
    {
      Plugin_076_hlw = new (std::nothrow) HLW8012;

      if (Plugin_076_hlw)
      {
        p076_checkdefault_queries(event);

        const uint8_t currentRead = P076_SEL_CUR_READ;
        const uint8_t cf_trigger = P076_CF_TRIGGER;
        const uint8_t cf1_trigger = P076_CF1_TRIGGER;

        Plugin_076_hlw->begin(CF_PIN, CF1_PIN, SEL_PIN, currentRead,
                              HLW_DELAYREADING * 1000); // in usec
#if PLUGIN_076_DEBUG
        addLog(LOG_LEVEL_INFO, F("P076: Init object done"));
#endif // if PLUGIN_076_DEBUG
        Plugin_076_hlw->setResistors(HLW_CURRENT_RESISTOR,
                                     HLW_VOLTAGE_RESISTOR_UP,
                                     HLW_VOLTAGE_RESISTOR_DOWN);
#if PLUGIN_076_DEBUG
        addLog(LOG_LEVEL_INFO, F("P076: Init Basic Resistor Values done"));
#endif // if PLUGIN_076_DEBUG

        ESPEASY_RULES_FLOAT_TYPE current, voltage, power;

        if (Plugin076_LoadMultipliers(event->TaskIndex, current, voltage, power))
        {
#if PLUGIN_076_DEBUG
          addLog(LOG_LEVEL_INFO, F("P076: Saved Calibration after INIT"));
#endif // if PLUGIN_076_DEBUG

          Plugin_076_hlw->setCurrentMultiplier(current);
          Plugin_076_hlw->setVoltageMultiplier(voltage);
          Plugin_076_hlw->setPowerMultiplier(power);
        }
        else
        {
          Plugin076_ResetMultipliers();
        }

#if PLUGIN_076_DEBUG
        addLog(LOG_LEVEL_INFO, F("P076: Applied Calibration after INIT"));
#endif // if PLUGIN_076_DEBUG

        StoredTaskIndex = event->TaskIndex; // store task index value in order to
                                            // use it in the PLUGIN_WRITE routine

        // Library expects an interrupt on both edges
        attachInterrupt(CF1_PIN, p076_hlw8012_cf1_interrupt, cf1_trigger);
        attachInterrupt(CF_PIN, p076_hlw8012_cf_interrupt, cf_trigger);

        // Need a few seconds to read the first sample, so trigger a new read a few seconds after init.
        Scheduler.schedule_task_device_timer(event->TaskIndex, millis() + 5000);

        success = true;
      }
    }
    break;
  }

  case PLUGIN_WRITE:

    if (Plugin_076_hlw)
    {
      const String command = parseString(string, 1);

      if (equals(command, F("hlwreset")))
      {
        Plugin076_ResetMultipliers();
        success = true;
      }

      if (equals(command, F("hlwcalibrate")))
      {
        float CalibVolt = 0.0f;
        float CalibCurr = 0.0f;
        float CalibAcPwr = 0.0f;

        if (validFloatFromString(parseString(string, 2), CalibVolt))
        {
          if (validFloatFromString(parseString(string, 3), CalibCurr))
          {
            validFloatFromString(parseString(string, 4), CalibAcPwr);
          }
        }
#if PLUGIN_076_DEBUG
        addLogMove(LOG_LEVEL_INFO,
                   strformat(F("P076: Calibration to values - Expected-V=%.2f - Expected-A=%.2f - Expected-W=%.2f"),
                             CalibVolt, CalibCurr, CalibAcPwr));
#endif // if PLUGIN_076_DEBUG
        bool changed = false;

        if (CalibVolt != 0)
        {
          Plugin_076_hlw->expectedVoltage(CalibVolt);
          changed = true;
        }

        if (definitelyGreaterThan(CalibCurr, 0.0f))
        {
          Plugin_076_hlw->expectedCurrent(CalibCurr);
          changed = true;
        }

        if (!essentiallyZero(CalibAcPwr))
        {
          Plugin_076_hlw->expectedActivePower(CalibAcPwr);
          changed = true;
        }

        // if at least one calibration value has been provided then save the new
        // multipliers //
        if (changed)
        {
          Plugin076_SaveMultipliers();
        }
        success = true;
      }

      if (equals(command, F("hlwresetenergy")))
      {
        Plugin076_ResetEnergy();
        success = true;
      }
    }
    break;
  }
  return success;
}

void Plugin076_ResetEnergy()
{
  if (Plugin_076_hlw)
  {
    Plugin_076_hlw->resetEnergy();
#if PLUGIN_076_DEBUG
    addLog(LOG_LEVEL_INFO, F("P076: Reset Energy counter to zero"));
#endif // if PLUGIN_076_DEBUG
  }
}

void Plugin076_ResetMultipliers()
{
  if (Plugin_076_hlw)
  {
    Plugin_076_hlw->resetMultipliers();
    Plugin076_SaveMultipliers();
#if PLUGIN_076_DEBUG
    addLog(LOG_LEVEL_INFO, F("P076: Reset Multipliers to DEFAULT"));
#endif // if PLUGIN_076_DEBUG
  }
}

void Plugin076_SaveMultipliers()
{
  if (StoredTaskIndex < 0)
  {
    return; // Not yet initialized.
  }
  ESPEASY_RULES_FLOAT_TYPE hlwMultipliers[3]{};

  if (Plugin076_ReadMultipliers(hlwMultipliers[0], hlwMultipliers[1], hlwMultipliers[2]))
  {
#if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
    SaveCustomTaskSettings(StoredTaskIndex, reinterpret_cast<const uint8_t *>(&hlwMultipliers),
                           sizeof(hlwMultipliers));
#else  // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
    double hlwMultipliers_d[3]{};
    hlwMultipliers_d[0] = hlwMultipliers[0];
    hlwMultipliers_d[1] = hlwMultipliers[1];
    hlwMultipliers_d[2] = hlwMultipliers[2];

    SaveCustomTaskSettings(StoredTaskIndex, reinterpret_cast<const uint8_t *>(&hlwMultipliers_d),
                           sizeof(hlwMultipliers_d));
#endif // if FEATURE_USE_DOUBLE_AS_ESPEASY_RULES_FLOAT_TYPE
  }
}

bool Plugin076_ReadMultipliers(ESPEASY_RULES_FLOAT_TYPE &current, ESPEASY_RULES_FLOAT_TYPE &voltage, ESPEASY_RULES_FLOAT_TYPE &power)
{
  current = 0.0f;
  voltage = 0.0f;
  power = 0.0f;

  if (Plugin_076_hlw)
  {
    current = Plugin_076_hlw->getCurrentMultiplier();
    voltage = Plugin_076_hlw->getVoltageMultiplier();
    power = Plugin_076_hlw->getPowerMultiplier();
    return true;
  }
  return false;
}

bool Plugin076_LoadMultipliers(taskIndex_t TaskIndex,
                               ESPEASY_RULES_FLOAT_TYPE &current,
                               ESPEASY_RULES_FLOAT_TYPE &voltage,
                               ESPEASY_RULES_FLOAT_TYPE &power)
{
  // If multipliers are empty load default ones and save all of them as
  // "CustomTaskSettings"
  if (!Plugin076_ReadMultipliers(current, voltage, power))
  {
    return false;
  }
  double hlwMultipliers[3];

  LoadCustomTaskSettings(TaskIndex, reinterpret_cast<uint8_t *>(&hlwMultipliers),
                         sizeof(hlwMultipliers));

  if (hlwMultipliers[0] > 1.0)
  {
    current = hlwMultipliers[0];
  }

  if (hlwMultipliers[1] > 1.0)
  {
    voltage = hlwMultipliers[1];
  }

  if (hlwMultipliers[2] > 1.0)
  {
    power = hlwMultipliers[2];
  }
  return (current > 1.0) && (voltage > 1.0) && (power > 1.0);
}

void Plugin076_Reset(taskIndex_t TaskIndex)
{
  if (Plugin_076_hlw)
  {
    const uint8_t CF_PIN = Settings.TaskDevicePin3[TaskIndex];
    const uint8_t CF1_PIN = Settings.TaskDevicePin2[TaskIndex];
    detachInterrupt(CF_PIN);
    detachInterrupt(CF1_PIN);
    delete Plugin_076_hlw;
    Plugin_076_hlw = nullptr;
  }
  StoredTaskIndex = -1;
  p076_read_stage = 0;
  p076_timer = 0;

  for (uint8_t i = 0; i < NR_ELEMENTS(p076_values); ++i)
  {
    p076_values[i].reset();
  }
}

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void IRAM_ATTR p076_hlw8012_cf1_interrupt()
{
  if (Plugin_076_hlw)
  {
    Plugin_076_hlw->cf1_interrupt();
  }
}

void IRAM_ATTR p076_hlw8012_cf_interrupt()
{
  if (Plugin_076_hlw)
  {
    Plugin_076_hlw->cf_interrupt();
  }
}

const __FlashStringHelper *p076_getQueryString(uint8_t value_nr, bool displayString)
{
  switch (value_nr)
  {
  case P076_INDEX_VOLT:
    return displayString ? F("Voltage") : F("Voltage");
  case P076_INDEX_CURR:
    return displayString ? F("Current") : F("Current");
  case P076_INDEX_POWR:
    return displayString ? F("Active Power") : F("Power");
  case P076_INDEX_VAR:
    return displayString ? F("Reactive Power") : F("VAR");
  case P076_INDEX_VA:
    return displayString ? F("Apparent Power") : F("VA");
  case P076_INDEX_PF:
    return displayString ? F("Power Factor") : F("PowerFactor");
  case P076_INDEX_ENER:
    return displayString ? F("Energy") : F("Ws");
  }
  return F("");
}

void p076_checkdefault_queries(struct EventStruct *event)
{
  // Set default querys if not set
  // Required to perform settings transition to init taskvalue output selection
  // which was added later in PR #5169
  if ((P076_QUERY1 == 0) && (P076_QUERY2 == 0) && (P076_QUERY3 == 0) && (P076_QUERY4 == 0))
  {
    P076_QUERY1 = P076_QUERY1_DFLT;
    P076_QUERY2 = P076_QUERY2_DFLT;
    P076_QUERY3 = P076_QUERY3_DFLT;
    P076_QUERY4 = P076_QUERY4_DFLT;
  }
}

void p076_addValue(struct EventStruct *event, uint8_t index, float value)
{
  if (index >= NR_ELEMENTS(p076_values))
  {
    return;
  }
  p076_values[index].add(value);
#if FEATURE_PLUGIN_STATS

  /*
     // Disabled for now as it may cause fluke measurements to be recorded.
     PluginTaskData_base *taskData = getPluginTaskData(event->TaskIndex);

     if (taskData == nullptr) {
      return;
     }

     for (taskVarIndex_t i = 0; i < 4; ++i) {
      if (index == PCONFIG(i)) {
        PluginStats *stats = taskData->getPluginStats(i);

        if (stats != nullptr) {
          stats->trackPeak(value);
        }
        return;
      }
     }
   */
#endif // if FEATURE_PLUGIN_STATS
}

#endif // USES_P076
