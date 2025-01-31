#include "src/Helpers/_CPlugin_Helper.h"
#ifdef USES_C016

// #######################################################################################################
// ########################### Controller Plugin 016: Controller - Cache #################################
// #######################################################################################################

/*
   This is a cache layer to collect data while not connected to a network.
   The data will first be stored in RTC memory, which will survive a crash/reboot and even an OTA update.
   If this RTC buffer is full, it will be flushed to whatever is set here as storage.

   Typical sample sets contain:
   - UNIX timestamp
   - task index delivering the data
   - 4 float values

   These are the result of any plugin sending data to this controller.

   The controller can save the samples from RTC memory to several places on the flash:
   - Files on FS
   - Part reserved for OTA update (TODO)
   - Unused flash after the partitioned space (TODO)

   The controller can deliver the data to:
   <TODO>
 */

# include "src/Globals/C016_ControllerCache.h"
# include "src/Globals/ESPEasy_time.h"

# define CPLUGIN_016
# define CPLUGIN_ID_016         16
# define CPLUGIN_NAME_016       "Cache Controller [Experimental]"

// #include <ArduinoJson.h>

bool C016_allowLocalSystemTime = false;

bool CPlugin_016(CPlugin::Function function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case CPlugin::Function::CPLUGIN_PROTOCOL_ADD:
    {
      ProtocolStruct& proto = getProtocolStruct(event->idx); //              = CPLUGIN_ID_016;
      proto.usesMQTT             = false;
      proto.usesTemplate         = false;
      proto.usesAccount          = false;
      proto.usesPassword         = false;
      proto.usesExtCreds         = false;
      proto.defaultPort          = 80;
      proto.usesID               = false;
      proto.usesHost             = false;
      proto.usesPort             = false;
      proto.usesQueue            = false;
      proto.usesCheckReply       = false;
      proto.usesTimeout          = false;
      proto.usesSampleSets       = false;
      proto.needsNetwork         = false;
      proto.allowsExpire         = false;
      proto.allowLocalSystemTime = true;
      break;
    }

    case CPlugin::Function::CPLUGIN_GET_DEVICENAME:
    {
      string = F(CPLUGIN_NAME_016);
      break;
    }

    case CPlugin::Function::CPLUGIN_INIT:
    {
      {
        MakeControllerSettings(ControllerSettings); // -V522

        if (AllocatedControllerSettings()) {
          LoadControllerSettings(event->ControllerIndex, *ControllerSettings);
          C016_allowLocalSystemTime = ControllerSettings->useLocalSystemTime();
        }
      }
      success = init_c016_delay_queue(event->ControllerIndex);
      ControllerCache.init();
      break;
    }

    case CPlugin::Function::CPLUGIN_EXIT:
    {
      exit_c016_delay_queue();
      break;
    }

    case CPlugin::Function::CPLUGIN_WEBFORM_LOAD:
    {
      break;
    }

    case CPlugin::Function::CPLUGIN_WEBFORM_SAVE:
    {
      break;
    }

    case CPlugin::Function::CPLUGIN_PROTOCOL_TEMPLATE:
    {
      free_string(event->String1);
      free_string(event->String2);
      break;
    }

    case CPlugin::Function::CPLUGIN_PROTOCOL_SEND:
    {
      // Collect the values at the same run, to make sure all are from the same sample
      uint8_t valueCount = getValueCountForTask(event->TaskIndex);

      if (event->timestamp_sec == 0) {
        if (C016_allowLocalSystemTime)
          event->setLocalTimeTimestamp();
        else 
          event->setUnixTimeTimestamp();
      }
      const C016_queue_element element(
        event,
        valueCount);

      const C016_binary_element binary_element = element.getBinary();
      success = ControllerCache.write(reinterpret_cast<const uint8_t *>(&binary_element), sizeof(C016_binary_element));
      break;
    }

    case CPlugin::Function::CPLUGIN_WRITE:
    {
      if (C016_CacheInitialized()) {
        const String command = parseString(string, 1);

        if (equals(command, F("cachecontroller"))) {
          const String subcommand = parseString(string, 2);

          if (equals(subcommand, F("flush"))) {
            C016_flush();
            success = true;
          }
        }
      }
      break;
    }

    case CPlugin::Function::CPLUGIN_FLUSH:
    {
      C016_flush();
      delay(0);
      break;
    }

    case CPlugin::Function::CPLUGIN_WEBFORM_SHOW_HOST_CONFIG:
    {
      string = F("-");
      break;
    }

    default:
      break;
  }
  return success;
}

// ********************************************************************************
// Process the data from the cache
// ********************************************************************************
// Uncrustify may change this into multi line, which will result in failed builds
// *INDENT-OFF*
bool do_process_c016_delay_queue(cpluginID_t cpluginID, const Queue_element_base& element_base, ControllerSettingsStruct& ControllerSettings) {
// *INDENT-ON*
return true;

// FIXME TD-er: Hand over data to wherever it needs to be.
// Ideas:
// - Upload bin files to some server (HTTP post?)
// - Provide a sample to any connected controller
// - Do nothing and let some extern host pull the data from the node.
// - JavaScript to process the data inside the browser.
// - Feed it to some plugin (e.g. a display to show a chart)
}

#endif // ifdef USES_C016
