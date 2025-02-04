#include "src/Helpers/_CPlugin_Helper.h"

#ifdef USES_C008

// #######################################################################################################
// ########################### Controller Plugin 008: Generic HTTP #######################################
// #######################################################################################################

# define CPLUGIN_008
# define CPLUGIN_ID_008         8
# define CPLUGIN_NAME_008       "Generic HTTP"

bool CPlugin_008(CPlugin::Function function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {
    case CPlugin::Function::CPLUGIN_PROTOCOL_ADD:
    {
      ProtocolStruct& proto = getProtocolStruct(event->idx); //      = CPLUGIN_ID_008;
      proto.usesMQTT     = false;
      proto.usesTemplate = true;
      proto.usesAccount  = true;
      proto.usesPassword = true;
      proto.usesExtCreds = true;
      proto.defaultPort  = 80;
      proto.usesID       = true;
      break;
    }

    case CPlugin::Function::CPLUGIN_GET_DEVICENAME:
    {
      string = F(CPLUGIN_NAME_008);
      break;
    }

    case CPlugin::Function::CPLUGIN_INIT:
    {
      success = init_c008_delay_queue(event->ControllerIndex);
      break;
    }

    case CPlugin::Function::CPLUGIN_EXIT:
    {
      exit_c008_delay_queue();
      break;
    }

    case CPlugin::Function::CPLUGIN_PROTOCOL_TEMPLATE:
    {
      free_string(event->String1);
      event->String2 = F("demo.php?name=%sysname%&task=%tskname%&valuename=%valname%&value=%value%");
      break;
    }

    case CPlugin::Function::CPLUGIN_PROTOCOL_SEND:
    {
      if (C008_DelayHandler == nullptr) {
        break;
      }
      if (C008_DelayHandler->queueFull(event->ControllerIndex)) {
        break;
      }


      String pubname;
      {
        // Place the ControllerSettings in a scope to free the memory as soon as we got all relevant information.
        MakeControllerSettings(ControllerSettings); //-V522

        if (!AllocatedControllerSettings()) {
          addLog(LOG_LEVEL_ERROR, F("C008 : Generic HTTP - Cannot send, out of RAM"));
          break;
        }
        LoadControllerSettings(event->ControllerIndex, *ControllerSettings);
        pubname = ControllerSettings->Publish;
      }
      const bool contains_valname = pubname.indexOf(F("%valname%")) != -1;

      uint8_t valueCount = getValueCountForTask(event->TaskIndex);
      std::unique_ptr<C008_queue_element> element(new (std::nothrow) C008_queue_element(event, valueCount));
      success = C008_DelayHandler->addToQueue(std::move(element));

      if (success) {
        // Element was added.
        // Now we try to append to the existing element
        // and thus preventing the need to create a long string only to copy it to a queue element.
        C008_queue_element& element = static_cast<C008_queue_element&>(*(C008_DelayHandler->sendQueue.back()));

        // Collect the values at the same run, to make sure all are from the same sample
        //LoadTaskSettings(event->TaskIndex); // FIXME TD-er: This can probably be removed

        for (uint8_t x = 0; x < valueCount; x++)
        {
          bool   isvalid;
          const String formattedValue = formatUserVar(event, x , isvalid);

          if (isvalid) {
            // First store in a temporary string, so we can use move_special to allocate on the best heap
            String txt;
            txt += '/';
            txt += pubname;
            if (contains_valname) {
              parseSingleControllerVariable(txt, event, x, true);
            }
            parseControllerVariables(txt, event, true);

# ifndef BUILD_NO_DEBUG
            if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
              addLog(LOG_LEVEL_DEBUG, strformat(
                F("C008 : pubname: %s value: %s"),
                pubname.c_str(),
                formattedValue.c_str()
              ));
            }
#endif
            txt.replace(F("%value%"), formattedValue);
            move_special(element.txt[x], std::move(txt));
# ifndef BUILD_NO_DEBUG
            if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
              addLog(LOG_LEVEL_DEBUG_MORE, concat(F("C008 : "), element.txt[x]));
            }
# endif // ifndef BUILD_NO_DEBUG
          }
        }
      }
      Scheduler.scheduleNextDelayQueue(SchedulerIntervalTimer_e::TIMER_C008_DELAY_QUEUE, C008_DelayHandler->getNextScheduleTime());
      break;
    }

    case CPlugin::Function::CPLUGIN_FLUSH:
    {
      process_c008_delay_queue();
      delay(0);
      break;
    }

    default:
      break;
  }
  return success;
}

// ********************************************************************************
// Generic HTTP get request
// ********************************************************************************

// Uncrustify may change this into multi line, which will result in failed builds
// *INDENT-OFF*
bool do_process_c008_delay_queue(cpluginID_t cpluginID, const Queue_element_base& element_base, ControllerSettingsStruct& ControllerSettings) {
  const C008_queue_element& element = static_cast<const C008_queue_element&>(element_base);
// *INDENT-ON*
  while (element.txt[element.valuesSent].isEmpty()) {
    // A non valid value, which we are not going to send.
    // Increase sent counter until a valid value is found.
    if (element.checkDone(true)) {
      return true;
    }
  }

  int httpCode = -1;
  send_via_http(
    cpluginID,
    ControllerSettings,
    element._controller_idx,
    element.txt[element.valuesSent],
    F("GET"),
    EMPTY_STRING,
    EMPTY_STRING,
    httpCode);
  return element.checkDone((httpCode >= 100) && (httpCode < 300));
}

#endif // ifdef USES_C008
