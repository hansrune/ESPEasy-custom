#include "_Plugin_Helper.h"
#ifdef USES_P022

# include "src/DataStructs/PinMode.h"
# include "src/Helpers/PortStatus.h"
# include "src/PluginStructs/P022_data_struct.h"

# include "ESPEasy-Globals.h" // For dummystring


// #######################################################################################################
// #################################### Plugin 022: PCA9685 ##############################################
// #######################################################################################################


# define PLUGIN_022
# define PLUGIN_ID_022         22
# define PLUGIN_NAME_022       "Extra IO - PCA9685"
# define PLUGIN_VALUENAME1_022 "PWM"

constexpr pluginID_t P022_PLUGIN_ID{ PLUGIN_ID_022 };

// FIXME TD-er: This plugin uses a lot of calls to the P022_data_struct, which could be combined in single functions.

boolean Plugin_022(uint8_t function, struct EventStruct *event, String& string)
{
  boolean  success = false;
  int      address = 0;
  int      mode2   = 0x10;
  uint16_t freq    = PCA9685_MAX_FREQUENCY;
  uint16_t range   = PCA9685_MAX_PWM;

  if ((event != nullptr) && (event->TaskIndex >= 0))
  {
    address = CONFIG_PORT;
    mode2   = PCONFIG(0);
    freq    = PCONFIG(1);
    range   = PCONFIG(2);
  }

  if ((address < PCA9685_ADDRESS) || (address > PCA9685_MAX_ADDRESS)) {
    address = PCA9685_ADDRESS;
  }

  if (freq == 0) {
    freq = PCA9685_MAX_FREQUENCY;
  }

  if (range == 0) {
    range = PCA9685_MAX_PWM;
  }

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      auto& dev = Device[++deviceCount];
      dev.Number             = PLUGIN_ID_022;
      dev.Type               = DEVICE_TYPE_I2C;
      dev.VType              = Sensor_VType::SENSOR_TYPE_NONE;
      dev.Ports              = 1;
      dev.Custom             = true;
      dev.ExitTaskBeforeSave = false;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_022);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_022));
      break;
    }

    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      uint8_t optionValues[PCA9685_NUMS_ADDRESS];

      for (uint8_t i = 0; i < PCA9685_NUMS_ADDRESS; ++i)
      {
        optionValues[i] = PCA9685_ADDRESS + i;
      }
      addFormSelectorI2C(F("i2c_addr"), PCA9685_NUMS_ADDRESS, optionValues, address);
      break;
    }

    # if FEATURE_I2C_GET_ADDRESS
    case PLUGIN_I2C_GET_ADDRESS:
    {
      event->Par1 = address;
      success     = true;
      break;
    }
    # endif // if FEATURE_I2C_GET_ADDRESS

    case PLUGIN_SET_DEFAULTS:
    {
      PCONFIG(0) = 0x10; // Default
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      // The options lists are quite long.
      // To prevent stack overflow issues, each selection has its own scope.
      {
        String m2Options[PCA9685_MODE2_VALUES];
        int    m2Values[PCA9685_MODE2_VALUES];

        for (int i = 0; i < PCA9685_MODE2_VALUES; ++i)
        {
          m2Values[i]  = i;
          m2Options[i] = formatToHex_decimal(i);

          if (i == 0x10) {
            m2Options[i] += F(" - (default)");
          }
        }
        addFormSelector(F("MODE2"), F("pmode2"), PCA9685_MODE2_VALUES, m2Options, m2Values, mode2);
      }
      addFormNumericBox(
        strformat(F("Frequency (%d-%d)"), PCA9685_MIN_FREQUENCY, PCA9685_MAX_FREQUENCY),
        F("pfreq"),
        freq,
        PCA9685_MIN_FREQUENCY,
        PCA9685_MAX_FREQUENCY);
      addFormNote(concat(F("default "), PCA9685_MAX_FREQUENCY));
      addFormNumericBox(F("Range (1-10000)"), F("prange"), range, 1, 10000);
      addFormNote(concat(F("default "), PCA9685_MAX_PWM));
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      const uint8_t oldAddress = CONFIG_PORT;

      CONFIG_PORT = getFormItemInt(F("i2c_addr"));
      PCONFIG(0)  = getFormItemInt(F("pmode2"));
      PCONFIG(1)  = getFormItemInt(F("pfreq"));
      PCONFIG(2)  = getFormItemInt(F("prange"));

      P022_data_struct *P022_data =
        static_cast<P022_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P022_data) {
        P022_data->p022_clear_init(oldAddress);

        if (!P022_data->p022_is_init(CONFIG_PORT))
        {
          P022_data->Plugin_022_initialize(address);

          if (PCONFIG(0) != mode2) {
            P022_data->Plugin_022_writeRegister(address, PCA9685_MODE2, PCONFIG(0));
          }

          if (PCONFIG(1) != freq) {
            P022_data->Plugin_022_Frequency(address, PCONFIG(1));
          }
        }
      }
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      success = initPluginTaskData(event->TaskIndex, new (std::nothrow) P022_data_struct());
      break;
    }

    case PLUGIN_WRITE:
    {
      # if FEATURE_I2C_DEVICE_CHECK

      if (!I2C_deviceCheck(address, event->TaskIndex, 10, PLUGIN_I2C_GET_ADDRESS)) {
        break; // Will return the default false for success
      }
      # endif // if FEATURE_I2C_DEVICE_CHECK
      P022_data_struct *P022_data =
        static_cast<P022_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr == P022_data) {
        break;
      }
      String log;
      bool   instanceCommand = false;
      String command         = parseString(string, 1);
      int8_t dotPos          = command.indexOf('.');

      if (dotPos > -1)
      {
        String name = command.substring(0, dotPos);
        removeChar(name, '[');
        removeChar(name, ']');

        if (name.equalsIgnoreCase(getTaskDeviceName(event->TaskIndex))) {
          command         = command.substring(dotPos + 1);
          instanceCommand = true;
        } else {
          break;
        }
      }

      if ((equals(command, F("pcapwm"))) || (instanceCommand && (equals(command, F("pwm")))))
      {
        success = true;

        // "log" is also sent along with the SendStatusOnlyIfNeeded
        log  = P022_data_struct::P022_logPrefix(address, F("PWM "));
        log += event->Par1;
        const uint32_t dutyCycle       = event->Par2;
        const uint32_t fadeDuration_ms = event->Par3;

        if ((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS))
        {
          if ((dutyCycle >= 0) && (dutyCycle <= range))
          {
            P022_data->initModeFreq(address, mode2, freq);

            // setPinState(P022_PLUGIN_ID, event->Par1, PIN_MODE_PWM, event->Par2);
            portStatusStruct newStatus;
            const uint32_t   key = createKey(P022_PLUGIN_ID, event->Par1);

            // WARNING: operator [] creates an entry in the map if key does not exist
            newStatus = globalMapPortStatus[key];

            // Code 'borrowed' from set_Gpio_PWM() in Hardware.cpp, keeping variable names
            if (fadeDuration_ms != 0)
            {
              const int32_t resolution_factor = (1 << 12);
              const uint8_t prev_mode         = newStatus.mode;
              int32_t prev_value              = newStatus.getDutyCycle();

              // getPinState(pluginID, gpio, &prev_mode, &prev_value);
              if (prev_mode != PIN_MODE_PWM) {
                prev_value = 0;
              }

              const int32_t step_value = ((static_cast<int32_t>(dutyCycle) - prev_value) * resolution_factor) /
                                         static_cast<int32_t>(fadeDuration_ms);
              int32_t curr_value = prev_value * resolution_factor;

              log += strformat(F(", fade: %d ms"), fadeDuration_ms);

              int i = fadeDuration_ms;

              while (i--) {
                curr_value += step_value;
                const int16_t new_value = curr_value / resolution_factor;

                P022_data->Plugin_022_Write(address, event->Par1, map(new_value, 0, range, 0, PCA9685_MAX_PWM));
                delay(1);
              }
            }
            P022_data->Plugin_022_Write(address, event->Par1, map(dutyCycle, 0, range, 0, PCA9685_MAX_PWM));

            newStatus.command   = 1;
            newStatus.mode      = PIN_MODE_PWM;
            newStatus.dutyCycle = dutyCycle; // state was the wrong field...
            savePortStatus(key, newStatus);

            if (loglevelActiveFor(LOG_LEVEL_INFO)) {
              addLog(LOG_LEVEL_INFO, log);
            }

            // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
            SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
          }
          else {
            if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
              addLog(LOG_LEVEL_ERROR, concat(log, strformat(F(" the pwm value %d  is invalid value."), event->Par2)));
            }
          }
        }
        else {
          if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
            addLog(LOG_LEVEL_ERROR, concat(log, F(" is invalid value.")));
          }
        }
      }

      if ((equals(command, F("pcafrq"))) || (instanceCommand && (equals(command, F("frq")))))
      {
        success = true;

        if ((event->Par1 >= PCA9685_MIN_FREQUENCY) && (event->Par1 <= PCA9685_MAX_FREQUENCY))
        {
          if (!P022_data->p022_is_init(address))
          {
            P022_data->Plugin_022_initialize(address);
            P022_data->Plugin_022_writeRegister(address, PCA9685_MODE2, mode2);
          }
          P022_data->Plugin_022_Frequency(address, event->Par1);

          // setPinState(PLUGIN_ID_022, 99, PIN_MODE_UNDEFINED, event->Par1);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(P022_PLUGIN_ID, 99);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_UNDEFINED;
          newStatus.state   = event->Par1;
          savePortStatus(key, newStatus);

          log  = P022_data_struct::P022_logPrefix(address, F("FREQ "));
          log += event->Par1;
          addLog(LOG_LEVEL_INFO, log);

          // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, 99, log, 0));
          SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
            addLog(LOG_LEVEL_ERROR,
                   strformat(F("%sfrequency %d out of range."),
                             P022_data_struct::P022_logPrefix(address).c_str(),
                             event->Par1));
          }
        }
      }

      if (instanceCommand && (equals(command, F("mode2"))))
      {
        success = true;

        if ((event->Par1 >= 0) && (event->Par1 < PCA9685_MODE2_VALUES))
        {
          if (!P022_data->p022_is_init(address))
          {
            P022_data->Plugin_022_initialize(address);
            P022_data->Plugin_022_Frequency(address, freq);
          }
          P022_data->Plugin_022_writeRegister(address, PCA9685_MODE2, event->Par1);

          addLog(LOG_LEVEL_INFO, strformat(F("%s%s"),
                                           P022_data_struct::P022_logPrefix(address, F("MODE2 0x")).c_str(),
                                           formatToHex(event->Par1, 2).c_str()));
        }
        else {
          addLog(LOG_LEVEL_ERROR,
                 strformat(F("%s%s is out of range"),
                           P022_data_struct::P022_logPrefix(address, F("MODE2 0x")).c_str(),
                           formatToHex(event->Par1, 2).c_str()));
        }
      }

      if (equals(command, F("status")))
      {
        if (equals(parseString(string, 2), F("pca")))
        {
          P022_data->initModeFreq(address, mode2, freq);
          success = true;
          String dummyString;

          // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par2, dummyString, 0));
          SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, createKey(P022_PLUGIN_ID, event->Par2), dummyString, 0);
        }
      }

      if (instanceCommand && (equals(command, F("gpio"))))
      {
        success = true;
        log     = P022_data_struct::P022_logPrefix(address, F("GPIO "));
        const bool allPins = equals(parseString(string, 2), F("all"));

        if (((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS)) ||
            allPins)
        {
          P022_data->initModeFreq(address, mode2, freq);
          int pin = event->Par1;

          if (allPins)
          {
            pin  = -1;
            log += F("all");
          }
          else
          {
            log += pin;
          }

          if (event->Par2 == 0)
          {
            log += F(" off");
            P022_data->Plugin_022_Off(address, pin);
          }
          else
          {
            log += F(" on");
            P022_data->Plugin_022_On(address, pin);
          }
          addLog(LOG_LEVEL_INFO, log);

          // setPinState(PLUGIN_ID_022, pin, PIN_MODE_OUTPUT, event->Par2);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(P022_PLUGIN_ID, pin);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_OUTPUT;
          newStatus.state   = event->Par2;
          savePortStatus(key, newStatus);

          // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, pin, log, 0));
          SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          addLog(LOG_LEVEL_ERROR, concat(log, F(" is invalid value.")));
        }
      }

      if (instanceCommand && (equals(command, F("pulse"))))
      {
        success = true;
        log     = concat(P022_data_struct::P022_logPrefix(address, F("GPIO ")), event->Par1);

        if ((event->Par1 >= 0) && (event->Par1 <= PCA9685_MAX_PINS))
        {
          P022_data->initModeFreq(address, mode2, freq);

          if (event->Par2 == 0)
          {
            log += F(" off");
            P022_data->Plugin_022_Off(address, event->Par1);
          }
          else
          {
            log += F(" on");
            P022_data->Plugin_022_On(address, event->Par1);
          }
          log += strformat(F(" Pulse set for %dms"), event->Par3);
          int autoreset = 0;

          if (event->Par3 > 0)
          {
            if (equals(parseString(string, 5), F("auto")))
            {
              autoreset = -1;
              log      += F(" with autoreset infinity");
            }
            else
            {
              autoreset = event->Par4;

              if (autoreset > 0)
              {
                log += concat(F(" for "), autoreset);
              }
            }
          }
          Scheduler.setPluginTaskTimer(event->Par3
                                       , event->TaskIndex
                                       , event->Par1
                                       , !event->Par2
                                       , event->Par3
                                       , autoreset);

          // setPinState(PLUGIN_ID_022, event->Par1, PIN_MODE_OUTPUT, event->Par2);
          portStatusStruct newStatus;
          const uint32_t   key = createKey(P022_PLUGIN_ID, event->Par1);

          // WARNING: operator [] creates an entry in the map if key does not exist
          newStatus         = globalMapPortStatus[key];
          newStatus.command = 1;
          newStatus.mode    = PIN_MODE_OUTPUT;
          newStatus.state   = event->Par2;
          savePortStatus(key, newStatus);

          addLog(LOG_LEVEL_INFO, log);

          // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
          SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
        }
        else {
          addLog(LOG_LEVEL_ERROR, concat(log, F(" is invalid value.")));
        }
      }

      break;
    }
    case PLUGIN_TASKTIMER_IN:
    {
      P022_data_struct *P022_data =
        static_cast<P022_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P022_data) {
        String log = P022_data_struct::P022_logPrefix(address, F("GPIO "));
        log += event->Par1;
        int autoreset = event->Par4;

        if (event->Par2 == 0)
        {
          log += F(" off");
          P022_data->Plugin_022_Off(address, event->Par1);
        }
        else
        {
          log += F(" on");
          P022_data->Plugin_022_On(address, event->Par1);
        }

        if ((autoreset > 0) || (autoreset == -1))
        {
          if (autoreset > -1)
          {
            log += concat(F(" Pulse auto restart for "), autoreset);
            autoreset--;
          }
          Scheduler.setPluginTaskTimer(event->Par3
                                       , event->TaskIndex
                                       , event->Par1
                                       , !event->Par2
                                       , event->Par3
                                       , autoreset);
        }

        // setPinState(PLUGIN_ID_022, event->Par1, PIN_MODE_OUTPUT, event->Par2);
        portStatusStruct newStatus;
        const uint32_t   key = createKey(P022_PLUGIN_ID, event->Par1);

        // WARNING: operator [] creates an entry in the map if key does not exist
        newStatus         = globalMapPortStatus[key];
        newStatus.command = 1;
        newStatus.mode    = PIN_MODE_OUTPUT;
        newStatus.state   = event->Par2;
        savePortStatus(key, newStatus);

        // SendStatus(event, getPinStateJSON(SEARCH_PIN_STATE, PLUGIN_ID_022, event->Par1, log, 0));
        SendStatusOnlyIfNeeded(event, SEARCH_PIN_STATE, key, log, 0);
      }
      break;
    }
  }
  return success;
}

#endif // USES_P022
