#include "_Plugin_Helper.h"
#ifdef USES_P002


# include "src/Helpers/Hardware.h"
# include "src/PluginStructs/P002_data_struct.h"

// #######################################################################################################
// #################################### Plugin 002: Analog ###############################################
// #######################################################################################################

# define PLUGIN_002
# define PLUGIN_ID_002         2
# define PLUGIN_NAME_002       "Analog input - internal"
# define PLUGIN_VALUENAME1_002 "Analog"


boolean Plugin_002(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      auto& dev = Device[++deviceCount];
      dev.Number           = PLUGIN_ID_002;
      dev.Type             = DEVICE_TYPE_ANALOG;
      dev.VType            = Sensor_VType::SENSOR_TYPE_SINGLE;
      dev.FormulaOption    = true;
      dev.ValueCount       = 1;
      dev.SendDataOption   = true;
      dev.TimerOption      = true;
      dev.PluginStats      = true;
      dev.TaskLogsOwnPeaks = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_002);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_002));
      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      P002_data_struct *P002_data =
        static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P002_data) {
        P002_data->webformLoad(event);
        success = true;
      } else {
        P002_data = new (std::nothrow) P002_data_struct();

        if (nullptr != P002_data) {
          P002_data->init(event);
          P002_data->webformLoad(event);
          success = true;
          delete P002_data;
        }
      }
      break;
    }

# if FEATURE_PLUGIN_STATS
    case PLUGIN_WEBFORM_LOAD_SHOW_STATS:
    {
      P002_data_struct *P002_data =
        static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P002_data) {
        success = P002_data->webformLoad_show_stats(event);
      }
      break;
    }
# endif // if FEATURE_PLUGIN_STATS

    case PLUGIN_WEBFORM_SAVE:
    {
      addHtmlError(P002_data_struct::webformSave(event));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      initPluginTaskData(event->TaskIndex, new (std::nothrow) P002_data_struct());
      P002_data_struct *P002_data =
        static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (nullptr != P002_data) {
        success = true;
        P002_data->init(event);
      }
      break;
    }
    case PLUGIN_TEN_PER_SECOND:
    {
      if (P002_OVERSAMPLING != P002_USE_CURENT_SAMPLE) // Use multiple samples
      {
        P002_data_struct *P002_data =
          static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

        if (nullptr != P002_data) {
          P002_data->takeSample();
        }
      }
      success = true;
      break;
    }

    case PLUGIN_READ:
    {
      int   raw_value = 0;
      float res_value = 0.0f;

      P002_data_struct *P002_data =
        static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

      if ((P002_data != nullptr) && P002_data->getValue(res_value, raw_value)) {
        UserVar.setFloat(event->TaskIndex, 0, res_value);

        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          String log = strformat(
            F("ADC  : Analog value: %d = %s"),
            raw_value,
            formatUserVarNoCheck(event, 0).c_str());

          if (P002_OVERSAMPLING == P002_USE_OVERSAMPLING) {
            log += strformat(F(" (%u samples)"), P002_data->getOversamplingCount());
          }
          addLogMove(LOG_LEVEL_INFO, log);
        }
        P002_data->reset();
        success = true;
      } else {
        addLog(LOG_LEVEL_ERROR, F("ADC  : No value received "));
        success = false;
      }

      break;
    }

    case PLUGIN_SET_CONFIG:
    {
      P002_data_struct *P002_data =
        static_cast<P002_data_struct *>(getPluginTaskData(event->TaskIndex));

      if (P002_data != nullptr) {
        success = P002_data->plugin_set_config(event, string);

        if (success) {
          P002_data->init(event);
        }
      }
      break;
    }
  }
  return success;
}

#endif // USES_P002
