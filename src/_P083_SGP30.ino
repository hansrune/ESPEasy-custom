#include "_Plugin_Helper.h"
#ifdef USES_P083

/*********************************************************************************************\
* SGP30 - Gas (TVOC - Total Volatile Organic Compounds) and Air Quality (eCO2)
*
*
* I2C Address: 0x58
\*********************************************************************************************/


# include "src/PluginStructs/P083_data_struct.h"

# define PLUGIN_083
# define PLUGIN_ID_083        83
# define PLUGIN_NAME_083       "Gases - SGP30 TVOC/eCO2"
# define PLUGIN_VALUENAME1_083 "TVOC"
# define PLUGIN_VALUENAME2_083 "eCO2"


# define P083_TVOC event->TaskIndex, 0
# define P083_ECO2 event->TaskIndex, 1
# define P083_TVOC_BASELINE event->TaskIndex, 2
# define P083_ECO2_BASELINE event->TaskIndex, 3


boolean Plugin_083(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      auto& dev = Device[++deviceCount];
      dev.Number         = PLUGIN_ID_083;
      dev.Type           = DEVICE_TYPE_I2C;
      dev.VType          = Sensor_VType::SENSOR_TYPE_DUAL;
      dev.FormulaOption  = true;
      dev.ValueCount     = 2;
      dev.SendDataOption = true;
      dev.TimerOption    = true;
      dev.PluginStats    = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_083);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_083));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_083));
      break;
    }

    case PLUGIN_I2C_HAS_ADDRESS:
    {
      success = (event->Par1 == 0x58);
      break;
    }

    # if FEATURE_I2C_GET_ADDRESS
    case PLUGIN_I2C_GET_ADDRESS:
    {
      event->Par1 = 0x58;
      success     = true;
      break;
    }
    # endif // if FEATURE_I2C_GET_ADDRESS

    case PLUGIN_WEBFORM_LOAD:
    {
      addFormSubHeader(F("Sensor"));
      addRowLabel(F("Sensor State"));
      P083_data_struct *P083_data =
        static_cast<P083_data_struct *>(getPluginTaskData(event->TaskIndex));

      bool isInitialized = false;

      if (nullptr != P083_data) {
        isInitialized = P083_data->initialized;
      }
      addHtml(isInitialized ? F("Initialized") : F("-"));
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P083_data_struct());
      P083_data_struct *P083_data =
        static_cast<P083_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P083_data) {
        if (!P083_data->initialized) {
          addLog(LOG_LEVEL_ERROR, F("SGP30: Sensor not found"));
        } else {
          // Look at the stored base line values to see if we can restore them.
          uint16_t eco2_base = UserVar.getFloat(P083_TVOC_BASELINE);
          uint16_t tvoc_base = UserVar.getFloat(P083_ECO2_BASELINE);

          if ((eco2_base != 0) && (tvoc_base != 0)) {
            addLog(LOG_LEVEL_INFO, F("SGP30: Restore last known baseline values"));
            P083_data->sgp.setIAQBaseline(eco2_base, tvoc_base);
          }
          success = true;
        }
      }
      break;
    }

    case PLUGIN_ONCE_A_SECOND:
    {
      P083_data_struct *P083_data =
        static_cast<P083_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P083_data) {
        if (P083_data->initialized)
        {
          if (P083_data->sgp.IAQmeasure())
          {
            UserVar.setFloat(P083_TVOC, P083_data->sgp.TVOC);
            UserVar.setFloat(P083_ECO2, P083_data->sgp.eCO2);
            success = true;

            // For the first 15s after the sgp30_iaq_init command the sensor is
            // in an initialization phase during which a sgp30_measure_iaq command
            // returns fixed values of 400 ppm CO2eq and 0 ppb TVOC.
            if ((timePassedSince(P083_data->init_time) > 15000) || ((P083_data->sgp.TVOC != 0) && (P083_data->sgp.eCO2 != 400))) {
              P083_data->newValues = true;
            }
          }
        }
      }
      break;
    }

    case PLUGIN_READ:
    {
      P083_data_struct *P083_data =
        static_cast<P083_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P083_data) {
        if (P083_data->initialized)
        {
          if (P083_data->newValues)
          {
            P083_data->newValues = false;

            // Keep track of the base line and store it in the unused uservar values.
            // When starting we will restore them so a crash or reboot will not result in strange values.
            uint16_t eco2_base, tvoc_base;

            if (P083_data->sgp.getIAQBaseline(&eco2_base, &tvoc_base)) {
              UserVar.setFloat(P083_TVOC_BASELINE, eco2_base);
              UserVar.setFloat(P083_ECO2_BASELINE, tvoc_base);
            }


            if (loglevelActiveFor(LOG_LEVEL_INFO)) {
              addLogMove(LOG_LEVEL_INFO, concat(F("SGP30: TVOC: "), formatUserVarNoCheck(P083_TVOC)));
              addLogMove(LOG_LEVEL_INFO, concat(F("SGP30: eCO2: "), formatUserVarNoCheck(P083_ECO2)));
            }
            success = true;
            break;
          } else {
            addLog(LOG_LEVEL_ERROR, F("SGP30: No new measured values"));
            break;
          }
        } else {
          addLog(LOG_LEVEL_ERROR, F("SGP30: Sensor not found"));
          break;
        }
      }
    }
  }
  return success;
}

#endif // USES_P083
