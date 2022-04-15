#include "../ESPEasyCore/ESPEasyRules.h"

#include "../Commands/InternalCommands.h"
#include "../DataStructs/TimingStats.h"
#include "../DataTypes/EventValueSource.h"
#include "../ESPEasyCore/ESPEasy_backgroundtasks.h"
#include "../ESPEasyCore/Serial.h"
#include "../Globals/Cache.h"
#include "../Globals/Device.h"
#include "../Globals/EventQueue.h"
#include "../Globals/ExtraTaskSettings.h"
#include "../Globals/Plugins.h"
#include "../Globals/Plugins_other.h"
#include "../Globals/Settings.h"
#include "../Helpers/ESPEasy_Storage.h"
#include "../Helpers/ESPEasy_time_calc.h"
#include "../Helpers/FS_Helper.h"
#include "../Helpers/Misc.h"
#include "../Helpers/Numerical.h"
#include "../Helpers/Rules_calculate.h"
#include "../Helpers/RulesHelper.h"
#include "../Helpers/RulesMatcher.h"
#include "../Helpers/StringConverter.h"
#include "../Helpers/StringParser.h"

#include "../../_Plugin_Helper.h"



#include <math.h>
#include <vector>


String EventToFileName(const String& eventName) {
  int size  = eventName.length();
  int index = eventName.indexOf('=');

  if (index > -1) {
    size = index;
  }
#if defined(ESP8266)
  String fileName = F("rules/");
#endif // if defined(ESP8266)
#if defined(ESP32)
  String fileName = F("/rules/");
#endif // if defined(ESP32)
  fileName += eventName.substring(0, size);
  fileName.replace('#', RULE_FILE_SEPARAROR);
  fileName.toLowerCase();
  return fileName;
}

String FileNameToEvent(const String& fileName) {
#if defined(ESP8266)
  String eventName = fileName.substring(6);
#endif // if defined(ESP8266)
#if defined(ESP32)
  String eventName = fileName.substring(7);
#endif // if defined(ESP32)
  eventName.replace(RULE_FILE_SEPARAROR, '#');
  return eventName;
}

void checkRuleSets() {
  Cache.rulesHelper.closeAllFiles();
}

/********************************************************************************************\
   Process next event from event queue
 \*********************************************************************************************/
bool processNextEvent() {
  if (Settings.UseRules)
  {
    String nextEvent;

    if (eventQueue.getNext(nextEvent)) {
      rulesProcessing(nextEvent);
      return true;
    }
  }

  // Just make sure any (accidentally) added or remaining events are not kept.
  eventQueue.clear();
  return false;
}

/********************************************************************************************\
   Rules processing
 \*********************************************************************************************/
void rulesProcessing(const String& event) {
  if (!Settings.UseRules) {
    return;
  }
  START_TIMER
  #ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("rulesProcessing"));
  #endif // ifndef BUILD_NO_RAM_TRACKER
#ifndef BUILD_NO_DEBUG
  unsigned long timer = millis();
#endif // ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_INFO)) {
    String log = F("EVENT: ");
    log += event;
    addLogMove(LOG_LEVEL_INFO, log);
  }

  if (Settings.OldRulesEngine()) {
    bool eventHandled = false;

    if (Settings.EnableRulesCaching()) {
      String filename;
      size_t pos = 0;
      if (Cache.rulesHelper.findMatchingRule(event, filename, pos)) {
        eventHandled = rulesProcessingFile(filename, event, pos);
      }
    } else {
      for (uint8_t x = 0; x < RULESETS_MAX && !eventHandled; x++) {
        eventHandled = rulesProcessingFile(getRulesFileName(x), event);
      }
    }
  } else {
    #ifdef WEBSERVER_NEW_RULES
    String fileName = EventToFileName(event);

    // if exists processed the rule file
    if (fileExists(fileName)) {
      rulesProcessingFile(fileName, event);
    }
# ifndef BUILD_NO_DEBUG
    else {
      addLog(LOG_LEVEL_DEBUG, String(F("EVENT: ")) + event +
             F(" is ingnored. File ") + fileName +
             F(" not found."));
    }
# endif    // ifndef BUILD_NO_DEBUG
    #endif // WEBSERVER_NEW_RULES
  }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("EVENT: ");
    log += event;
    log += F(" Processing time:");
    log += timePassedSince(timer);
    log += F(" milliSeconds");
    addLogMove(LOG_LEVEL_DEBUG, log);
  }
#endif // ifndef BUILD_NO_DEBUG
  STOP_TIMER(RULES_PROCESSING);
  backgroundtasks();
}

/********************************************************************************************\
   Rules processing
 \*********************************************************************************************/
bool rulesProcessingFile(const String& fileName, const String& event, size_t pos) {
  if (!Settings.UseRules || !fileExists(fileName)) {
    return false;
  }
  #ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("rulesProcessingFile"));
  #endif // ifndef BUILD_NO_RAM_TRACKER
#ifndef BUILD_NO_DEBUG

  if (Settings.SerialLogLevel == LOG_LEVEL_DEBUG_DEV) {
    serialPrint(F("RuleDebug Processing:"));
    serialPrintln(fileName);
    serialPrintln(F("     flags CMI  parse output:"));
  }
#endif // ifndef BUILD_NO_DEBUG

  static uint8_t nestingLevel = 0;

  nestingLevel++;

  if (nestingLevel > RULES_MAX_NESTING_LEVEL) {
    addLog(LOG_LEVEL_ERROR, F("EVENT: Error: Nesting level exceeded!"));
    nestingLevel--;
    return false;
  }


  bool match     = false;
  bool codeBlock = false;
  bool isCommand = false;
  bool condition[RULES_IF_MAX_NESTING_LEVEL];
  bool ifBranche[RULES_IF_MAX_NESTING_LEVEL];
  uint8_t ifBlock     = 0;
  uint8_t fakeIfBlock = 0;


  bool moreAvailable = true;
  bool eventHandled = false;
  while (moreAvailable && !eventHandled) {
    const bool searchNextOnBlock = !codeBlock && !match;
    String line = Cache.rulesHelper.readLn(fileName, pos, moreAvailable, searchNextOnBlock);

    // Parse the line and extract the action (if there is any)
    String action;
    {
      START_TIMER
      const bool matched_before_parse = match;
      parseCompleteNonCommentLine(line, event, action, match, codeBlock,
                                  isCommand, condition, ifBranche, ifBlock,
                                  fakeIfBlock);
      if (matched_before_parse && !match) {
        // We were processing a matching event and now crossed the "endon"
        // So we're done processing
        eventHandled = true;
        backgroundtasks();
      }
      STOP_TIMER(RULES_PARSE_LINE);
    }

    if (match) // rule matched for one action or a block of actions
    {
      START_TIMER
      processMatchedRule(action, event, match, codeBlock,
                          isCommand, condition, ifBranche, ifBlock, fakeIfBlock);
      STOP_TIMER(RULES_PROCESS_MATCHED);
    }
  }

/*
  if (f) {
    f.close();
  }
*/

  nestingLevel--;
  #ifndef BUILD_NO_RAM_TRACKER
  checkRAM(F("rulesProcessingFile2"));
  #endif // ifndef BUILD_NO_RAM_TRACKER
  backgroundtasks();
  return eventHandled; // && nestingLevel == 0;
}


/********************************************************************************************\
   Parse string commands
 \*********************************************************************************************/
bool get_next_inner_bracket(const String& line, int& startIndex, int& closingIndex, char closingBracket)
{
  if (line.length() <= 1) {
    // Not possible to have opening and closing bracket on a line this short.
    return false;
  }
  char openingBracket = closingBracket;

  switch (closingBracket) {
    case ']': openingBracket = '['; break;
    case '}': openingBracket = '{'; break;
    case ')': openingBracket = '('; break;
    default:
      // unknown bracket type
      return false;
  }
  // Closing bracket should not be found on the first position.
  closingIndex = line.indexOf(closingBracket, startIndex + 1);

  if (closingIndex == -1) { 
    // not found
    return false; 
  }

  for (int i = (closingIndex - 1); i > startIndex; --i) {
    if (line[i] == openingBracket) {
      startIndex = i;
      return true;
    }
  }
  return false;
}

bool get_next_argument(const String& fullCommand, int& index, String& argument, char separator)
{
  if (index == -1) {
    return false;
  }
  int newIndex = fullCommand.indexOf(separator, index);

  if (newIndex == -1) {
    argument = fullCommand.substring(index);
  } else {
    argument = fullCommand.substring(index, newIndex);
  }

  if (argument.startsWith(String(separator))) {
    argument = argument.substring(1);
  }

  //  addLog(LOG_LEVEL_INFO, String("get_next_argument: ") + String(index) + " " + fullCommand + " " + argument);
  index = newIndex;

  if (index != -1) {
    ++index;
  }
  return argument.length() > 0;
}

bool parse_bitwise_functions(const String& cmd_s_lower, const String& arg1, const String& arg2, const String& arg3, int64_t& result) {
  #ifndef BUILD_NO_DEBUG
  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("Bitwise: {");
    log += wrapIfContains(cmd_s_lower, ':', '\"');
    log += ':';
    log += wrapIfContains(arg1, ':', '\"');

    if (arg2.length() > 0) {
      log += ':';
      log += wrapIfContains(arg2, ':', '\"');

      if (arg3.length() > 0) {
        log += ':';
        log += wrapIfContains(arg3, ':', '\"');
      }
    }
    log += '}';
    addLogMove(LOG_LEVEL_DEBUG, log);
  }
  #endif

  if (cmd_s_lower.length() < 2) {
    return false;
  }

  if (cmd_s_lower.startsWith(F("bit"))) {
    #define bitSetULL(value, bit) ((value) |= (1ULL << (bit)))
    #define bitClearULL(value, bit) ((value) &= ~(1ULL << (bit)))
    #define bitWriteULL(value, bit, bitvalue) (bitvalue ? bitSetULL(value, bit) : bitClearULL(value, bit))
    uint32_t bitnr = 0;
    uint64_t iarg2 = 0;

    if (!validUIntFromString(arg1, bitnr) || !validUInt64FromString(arg2, iarg2)) {
      return false;
    }

    if (cmd_s_lower.equals(F("bitread"))) {
      // Syntax like {bitread:0:123} to get a single decimal '1'
      result = bitRead(iarg2, bitnr);
    } else if (cmd_s_lower.equals(F("bitset"))) {
      // Syntax like {bitset:0:122} to set least significant bit of the given nr '122' to '1' => '123'
      result = iarg2;
      bitSetULL(result, bitnr);
    } else if (cmd_s_lower.equals(F("bitclear"))) {
      // Syntax like {bitclear:0:123} to set least significant bit of the given nr '123' to '0' => '122'
      result = iarg2;
      bitClearULL(result, bitnr);
    } else if (cmd_s_lower.equals(F("bitwrite"))) {
      uint32_t iarg3 = 0;

      // Syntax like {bitwrite:0:122:1} to set least significant bit of the given nr '122' to '1' => '123'
      if (validUIntFromString(arg3, iarg3)) {
        const int bitvalue = (iarg3 & 1); // Only use the last bit of the given parameter
        result = iarg2;
        bitWriteULL(result, bitnr, bitvalue);
      } else {
        // Need 3 parameters, but 3rd one is not a valid uint
        return false;
      }
    } else {
      // Starts with "bit", but no matching function found
      return false;
    }

    // all functions starting with "bit" are checked
    return true;
  }

  uint64_t iarg1, iarg2 = 0;

  if (!validUInt64FromString(arg1, iarg1) || !validUInt64FromString(arg2, iarg2)) {
    return false;
  }

  if (cmd_s_lower.equals(F("xor"))) {
    // Syntax like {xor:127:15} to XOR the binary values 1111111 and 1111 => 1110000
    result = iarg1 ^ iarg2;
  } else if (cmd_s_lower.equals(F("and"))) {
    // Syntax like {and:254:15} to AND the binary values 11111110 and 1111 => 1110
    result = iarg1 & iarg2;
  } else if (cmd_s_lower.equals(F("or"))) {
    // Syntax like {or:254:15} to OR the binary values 11111110 and 1111 => 11111111
    result = iarg1 | iarg2;
  } else {
    // No matching function found
    return false;
  }
  return true;
}

bool parse_math_functions(const String& cmd_s_lower, const String& arg1, const String& arg2, const String& arg3, double& result) {
  double farg1;
  float  farg2, farg3 = 0.0f;

  if (!validDoubleFromString(arg1, farg1)) {
    return false;
  }

  if (cmd_s_lower.equals(F("constrain"))) {
    // Contrain a value X to be within range of A to B
    // Syntax like {constrain:x:a:b} to constrain x in range a...b
    if (validFloatFromString(arg2, farg2) && validFloatFromString(arg3, farg3)) {
      if (farg2 > farg3) {
        const float tmp = farg2;
        farg2 = farg3;
        farg3 = tmp;
      }
      result = constrain(farg1, farg2, farg3);
    } else {
      return false;
    }
  } else {
    // No matching function found
    return false;
  }
  return true;
}

void parse_string_commands(String& line) {
  int startIndex = 0;
  int closingIndex;

  bool mustReplaceMaskedChars = false;

  while (get_next_inner_bracket(line, startIndex, closingIndex, '}')) {
    // Command without opening and closing brackets.
    const String fullCommand = line.substring(startIndex + 1, closingIndex);
    const String cmd_s_lower = parseString(fullCommand, 1, ':');
    const String arg1        = parseStringKeepCase(fullCommand, 2, ':');
    const String arg2        = parseStringKeepCase(fullCommand, 3, ':');
    const String arg3        = parseStringKeepCase(fullCommand, 4, ':');

    if (cmd_s_lower.length() > 0) {
      String replacement; // maybe just replace with empty to avoid looping?
      //      addLog(LOG_LEVEL_INFO, String(F("parse_string_commands cmd: ")) + cmd_s_lower + " " + arg1 + " " + arg2 + " " + arg3);

      uint64_t iarg1, iarg2 = 0;
      double   fresult = 0.0;
      int64_t  iresult = 0;

      if (parse_math_functions(cmd_s_lower, arg1, arg2, arg3, fresult)) {
        const bool trimTrailingZeros = true;
        replacement = doubleToString(fresult, maxNrDecimals_double(fresult), trimTrailingZeros);
      } else if (parse_bitwise_functions(cmd_s_lower, arg1, arg2, arg3, iresult)) {
        replacement = ull2String(iresult);
      } else if (cmd_s_lower.equals(F("substring"))) {
        // substring arduino style (first char included, last char excluded)
        // Syntax like 12345{substring:8:12:ANOTHER HELLO WORLD}67890
        int startpos, endpos = -1;

        if (validIntFromString(arg1, startpos)
            && validIntFromString(arg2, endpos)) {
          replacement = arg3.substring(startpos, endpos);
        }
      } else if (cmd_s_lower.equals(F("strtol"))) {
        // string to long integer (from cstdlib)
        // Syntax like 1234{strtol:16:38}7890
        if (validUInt64FromString(arg1, iarg1)
            && validUInt64FromString(arg2, iarg2)) {
          replacement = String(strtoul(arg2.c_str(), nullptr, iarg1));
        }

        // FIXME TD-er: removed for now as it is too specific.
        // Maybe introduce one using 2 or 3 parameters ({div:100:255:3} for *100/255 3 decimals)

        /*
           } else if (cmd_s_lower.equals(F("div100ths"))) {
           // division and giving the 100ths as integer
           // 5 / 100 would yield 5
           // useful for fractions that use a full uint8_t gaining a
           // precision/granularity of 1/256 instead of only 1/100
           // Syntax like XXX{div100ths:24:256}XXX
           if (validUInt64FromString(arg1, iarg1)
            && validUInt64FromString(arg2, iarg2)) {
           float val = (100.0 * iarg1) / (1.0 * iarg2);
           char sval[10];
           sprintf_P(sval, PSTR("%02d"), (int)val);
           replacement = String(sval);
           }
         */
      } else if (cmd_s_lower.equals(F("tobin"))) {
        // Convert to binary string
        // Syntax like 1234{tobin:15}7890
        if (validUInt64FromString(arg1, iarg1)) {
          replacement = ull2String(iarg1, BIN);
        }
      } else if (cmd_s_lower.equals(F("tohex"))) {
        // Convert to HEX string
        // Syntax like 1234{tohex:15}7890
        if (validUInt64FromString(arg1, iarg1)) {
          replacement = ull2String(iarg1, HEX);
        }
      } else if (cmd_s_lower.equals(F("ord"))) {
        // Give the ordinal/integer value of the first character of a string
        // Syntax like let 1,{ord:B}
        uint8_t uval = arg1.c_str()[0];
        replacement = String(uval);
      } else if (cmd_s_lower.equals(F("urlencode"))) {
        // Convert to url-encoded string
        // Syntax like {urlencode:"string to/encode"}
        if (!arg1.isEmpty()) {
          replacement = URLEncode(arg1);
        }
      }

      if (replacement.isEmpty()) {
        // part in braces is not a supported command.
        // replace the {} with other characters to mask the braces so we can continue parsing.
        // We have to unmask then after we're finished.
        // See: https://github.com/letscontrolit/ESPEasy/issues/2932#issuecomment-596139096
        replacement = line.substring(startIndex, closingIndex + 1);
        replacement.replace('{', static_cast<char>(0x02));
        replacement.replace('}', static_cast<char>(0x03));
        mustReplaceMaskedChars = true;
      }

      // Replace the full command including opening and closing brackets.
      line.replace(line.substring(startIndex, closingIndex + 1), replacement);

      /*
         if (replacement.length() > 0) {
         addLog(LOG_LEVEL_INFO, String(F("parse_string_commands cmd: ")) + fullCommand + String(F(" -> ")) + replacement);
         }
       */
    }
  }

  if (mustReplaceMaskedChars) {
    // We now have to check if we did mask some parts and unmask them.
    // Let's hope we don't mess up any Unicode here.
    line.replace(static_cast<char>(0x02), '{');
    line.replace(static_cast<char>(0x03), '}');
  }
}

void replace_EventValueN_Argv(String& line, const String& argString, unsigned int argc)
{
  String eventvalue;

  eventvalue.reserve(16);
  eventvalue = F("%eventvalue");

  if (argc == 0) {
    // Used for compatibility reasons
    // it still needs to call the "1st" argument
    argc = 1;
  } else {
    eventvalue += argc;
  }
  eventvalue += '%';
  String tmpParam;

  if (GetArgv(argString.c_str(), tmpParam, argc)) {
    line.replace(eventvalue, tmpParam);
  }
}

void substitute_eventvalue(String& line, const String& event) {
  if (substitute_eventvalue_CallBack_ptr != nullptr) {
    substitute_eventvalue_CallBack_ptr(line, event);
  }

  if (line.indexOf(F("%eventvalue")) != -1) {
    if (event.charAt(0) == '!') {
      line.replace(F("%eventvalue%"), event); // substitute %eventvalue% with
                                              // literal event string if
                                              // starting with '!'
    } else {
      int equalsPos = event.indexOf('=');

      if (equalsPos > 0) {
        // Replace %eventvalueX% with the actual value of the event.
        // For compatibility reasons also replace %eventvalue%  (argc = 0)
        String argString = event.substring(equalsPos + 1);

        for (unsigned int argc = 0; argc <= 4; ++argc) {
          replace_EventValueN_Argv(line, argString, argc);
        }
      }
    }
  }
}

void parseCompleteNonCommentLine(String& line, const String& event,
                                 String& action, bool& match,
                                 bool& codeBlock, bool& isCommand,
                                 bool condition[], bool ifBranche[],
                                 uint8_t& ifBlock, uint8_t& fakeIfBlock) {
  if (line.length() == 0) {
    return;
  }
  const bool lineStartsWith_on = line.substring(0, 3).equalsIgnoreCase(F("on "));

  if (!codeBlock && !match) {
    // We're looking for a new code block.
    // Continue with next line if none was found on current line.
    if (!lineStartsWith_on) {
      return;
    }
  }

  if (line.equalsIgnoreCase(F("endon"))) // Check if action block has ended, then we will
                                           // wait for a new "on" rule
  {
    isCommand   = false;
    codeBlock   = false;
    match       = false;
    ifBlock     = 0;
    fakeIfBlock = 0;
    return;
  }


  isCommand = true;

  if (match || !codeBlock) {
    // only parse [xxx#yyy] if we have a matching ruleblock or need to eval the
    // "on" (no codeBlock)
    // This to avoid wasting CPU time...
    if (match && !fakeIfBlock) {
      // substitution of %eventvalue% is made here so it can be used on if
      // statement too
      substitute_eventvalue(line, event);
    }

    if (match || lineStartsWith_on) {
      // Only parseTemplate when we are actually doing something with the line.
      // When still looking for the "on ... do" part, do not change it before we found the block.
      line = parseTemplate(line);
    }
  }


  if (!codeBlock) // do not check "on" rules if a block of actions is to be
                  // processed
  {
    action.clear();
    if (lineStartsWith_on) {
      String lineOrg = line; // store original line for future use
      line.toLowerCase();    // convert all to lower case to make checks easier

      ifBlock     = 0;
      fakeIfBlock = 0;
      const int split = line.indexOf(F(" do"), 3);

      String eventTrigger;

      if (split != -1) {
        eventTrigger = line.substring(3, split); // Skipping "on "
        action       = lineOrg.substring(split + 4); // " do" + " " = 4 chars

        // Remove trailing and leadin spaces on the eventTrigger and action.
        eventTrigger.trim();
        action.trim();

        START_TIMER
        match = ruleMatch(event, eventTrigger);
        STOP_TIMER(RULES_MATCH);
      } else {
        match = false;
      }

      if (action.length() > 0) // single on/do/action line, no block
      {
        isCommand = true;
        codeBlock = false;
      } else {
        isCommand = false;
        codeBlock = true;
      }
    }
  } else {
    #ifndef BUILD_NO_DEBUG
    if (loglevelActiveFor(LOG_LEVEL_DEBUG_DEV)) {
      // keep the line for the log
      action = line;
    } else {
      action = std::move(line);  
    }
    #else
    action = std::move(line);
    #endif
  }

#ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG_DEV)) {
    String log = F("RuleDebug: ");
    log += codeBlock ? 0 : 1;
    log += match ? 0 : 1;
    log += isCommand ? 0 : 1;
    log += F(": ");
    log += line;
    addLogMove(LOG_LEVEL_DEBUG_DEV, log);
  }
#endif // ifndef BUILD_NO_DEBUG
}

void processMatchedRule(String& action, const String& event,
                        bool& match, bool& codeBlock,
                        bool& isCommand, bool condition[], bool ifBranche[],
                        uint8_t& ifBlock, uint8_t& fakeIfBlock) {
  String lcAction = action;

  lcAction.toLowerCase();

  if (fakeIfBlock) {
    isCommand = false;
  }
  else if (ifBlock) {
    if (condition[ifBlock - 1] != ifBranche[ifBlock - 1]) {
      isCommand = false;
    }
  }
  int split =
    lcAction.indexOf(F("elseif ")); // check for optional "elseif" condition

  if (split != -1) {
    // Found "elseif" condition
    isCommand = false;

    if (ifBlock && !fakeIfBlock) {
      if (ifBranche[ifBlock - 1]) {
        if (condition[ifBlock - 1]) {
          ifBranche[ifBlock - 1] = false;
        }
        else {
          String check = lcAction.substring(split + 7);
          check.trim();
          condition[ifBlock - 1] = conditionMatchExtended(check);
#ifndef BUILD_NO_DEBUG

          if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
            String log  = F("Lev.");
            log += String(ifBlock);
            log += F(": [elseif ");
            log += check;
            log += F("]=");
            log += boolToString(condition[ifBlock - 1]);
            addLogMove(LOG_LEVEL_DEBUG, log);
          }
#endif // ifndef BUILD_NO_DEBUG
        }
      }
    }
  } else {
    // check for optional "if" condition
    split = lcAction.indexOf(F("if "));

    if (split != -1) {
      if (ifBlock < RULES_IF_MAX_NESTING_LEVEL) {
        if (isCommand) {
          ifBlock++;
          String check = lcAction.substring(split + 3);
          check.trim();
          condition[ifBlock - 1] = conditionMatchExtended(check);
          ifBranche[ifBlock - 1] = true;
#ifndef BUILD_NO_DEBUG

          if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
            String log  = F("Lev.");
            log += String(ifBlock);
            log += F(": [if ");
            log += check;
            log += F("]=");
            log += boolToString(condition[ifBlock - 1]);
            addLogMove(LOG_LEVEL_DEBUG, log);
          }
#endif // ifndef BUILD_NO_DEBUG
        } else {
          fakeIfBlock++;
        }
      } else {
        fakeIfBlock++;

        if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
          String log  = F("Lev.");
          log += String(ifBlock);
          log += F(": Error: IF Nesting level exceeded!");
          addLogMove(LOG_LEVEL_ERROR, log);
        }
      }
      isCommand = false;
    }
  }

  if ((lcAction == F("else")) && !fakeIfBlock) // in case of an "else" block of
                                               // actions, set ifBranche to
                                               // false
  {
    ifBranche[ifBlock - 1] = false;
    isCommand              = false;
#ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      String log  = F("Lev.");
      log += String(ifBlock);
      log += F(": [else]=");
      log += boolToString(condition[ifBlock - 1] == ifBranche[ifBlock - 1]);
      addLogMove(LOG_LEVEL_DEBUG, log);
    }
#endif // ifndef BUILD_NO_DEBUG
  }

  if (lcAction == F("endif")) // conditional block ends here
  {
    if (fakeIfBlock) {
      fakeIfBlock--;
    }
    else if (ifBlock) {
      ifBlock--;
    }
    isCommand = false;
  }

  // process the action if it's a command and unconditional, or conditional and
  // the condition matches the if or else block.
  if (isCommand) {
    substitute_eventvalue(action, event);

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      String actionlog = F("ACT  : ");
      actionlog += action;
      addLogMove(LOG_LEVEL_INFO, actionlog);
    }

    ExecuteCommand_all(EventValueSource::Enum::VALUE_SOURCE_RULES, action.c_str());
    delay(0);
  }
}

/********************************************************************************************\
   Check if an event matches to a given rule
 \*********************************************************************************************/


/********************************************************************************************\
   Check expression
 \*********************************************************************************************/
bool conditionMatchExtended(String& check) {
  int  condAnd   = -1;
  int  condOr    = -1;
  bool rightcond = false;
  bool leftcond  = conditionMatch(check); // initial check

  #ifndef BUILD_NO_DEBUG
  String debugstr;

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    debugstr += boolToString(leftcond);
  }
  #endif // ifndef BUILD_NO_DEBUG

  do {
    #ifndef BUILD_NO_DEBUG

    if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
      String log = F("conditionMatchExtended: ");
      log += debugstr;
      log += ' ';
      log += wrap_String(check, '"');
      addLogMove(LOG_LEVEL_DEBUG, log);
    }
    #endif // ifndef BUILD_NO_DEBUG
    condAnd = check.indexOf(F(" and "));
    condOr  = check.indexOf(F(" or "));

    if ((condAnd > 0) || (condOr > 0)) {                             // we got AND/OR
      if ((condAnd > 0) && (((condOr < 0) /*&& (condOr < condAnd)*/) ||
                            ((condOr > 0) && (condOr > condAnd)))) { // AND is first
        check     = check.substring(condAnd + 5);
        rightcond = conditionMatch(check);
        leftcond  = (leftcond && rightcond);

        #ifndef BUILD_NO_DEBUG

        if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
          debugstr += F(" && ");
        }
        #endif // ifndef BUILD_NO_DEBUG
      } else { // OR is first
        check     = check.substring(condOr + 4);
        rightcond = conditionMatch(check);
        leftcond  = (leftcond || rightcond);

        #ifndef BUILD_NO_DEBUG

        if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
          debugstr += F(" || ");
        }
        #endif // ifndef BUILD_NO_DEBUG
      }

      #ifndef BUILD_NO_DEBUG

      if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
        debugstr += boolToString(rightcond);
      }
      #endif // ifndef BUILD_NO_DEBUG
    }
  } while (condAnd > 0 || condOr > 0);

  #ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    check = debugstr;
  }
  #endif // ifndef BUILD_NO_DEBUG
  return leftcond;
}


void logtimeStringToSeconds(const String& tBuf, int hours, int minutes, int seconds, bool valid)
{
  #ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log;
    log  = F("timeStringToSeconds: ");
    log += wrap_String(tBuf, '"');
    log += F(" --> ");
    if (valid) {
      if (hours < 10) log += '0';
      log += hours;
      log += ':';
      if (minutes < 10) log += '0';
      log += minutes;
      log += ':';
      if (seconds < 10) log += '0';
      log += seconds;
    } else {
      log += F("invalid");
    }
    addLogMove(LOG_LEVEL_DEBUG, log);
  }

  #endif // ifndef BUILD_NO_DEBUG
}

// convert old and new time string to nr of seconds
// return whether it should be considered a time string.
bool timeStringToSeconds(const String& tBuf, int& time_seconds, String& timeString) {
  {
    // Make sure we only check for expected characters
    // e.g. if 10:11:12 > 7:07 and 10:11:12 < 20:09:04
    // Should only try to match "7:07", not "7:07 and 10:11:12" 
    // Or else it will find "7:07:11"
    bool done = false;
    for (uint8_t pos = 0; !done && timeString.length() < 8 && pos < tBuf.length(); ++pos) {
      char c = tBuf[pos];
      if (isdigit(c) || c == ':') {
        timeString += c;
      } else {
        done = true;
      }
    }
  }

  time_seconds = -1;
  int hours   = 0;
  int minutes = 0;
  int seconds = 0;

  int tmpIndex = 0;
  String hours_str, minutes_str, seconds_str;
  bool   validTime = false;

  if (get_next_argument(timeString, tmpIndex, hours_str, ':')) {
    if (validIntFromString(hours_str, hours)) {
      validTime = true;

      if ((hours < 0) || (hours > 24)) {
        validTime = false;
      } else {
        time_seconds = hours * 60 * 60;
      }

      if (validTime && get_next_argument(timeString, tmpIndex, minutes_str, ':')) {
        if (validIntFromString(minutes_str, minutes)) {
          if ((minutes < 0) || (minutes > 59)) {
            validTime = false;
          } else {
            time_seconds += minutes * 60;
          }

          if (validTime && get_next_argument(timeString, tmpIndex, seconds_str, ':')) {
            // New format, only HH:MM:SS
            if (validIntFromString(seconds_str, seconds)) {
              if ((seconds < 0) || (seconds > 59)) {
                validTime = false;
              } else {
                time_seconds += seconds;
              }
            }
          } else {
            // Old format, only HH:MM
          }
        }
      } else {
        // It is a valid time string, but could also be just a numerical.
        // We mark it here as invalid, meaning the 'other' time to compare it to must contain more than just the hour.
        validTime = false;
      }
    }
  }
  logtimeStringToSeconds(timeString, hours, minutes, seconds, validTime);
  return validTime;
}

// Balance the count of parentheses (aka round braces) by adding the missing left or right parentheses, if any
// Returns the number of added parentheses, < 0 is left parentheses added, > 0 is right parentheses added
int balanceParentheses(String& string) {
  int left = 0;
  int right = 0;
  for (unsigned int i = 0; i < string.length(); i++) {
    switch (string[i]) {
      case '(':
        left++;
        break;
      case ')':
        right++;
        break;
    }
  }
  if (left != right) {
    string.reserve(string.length() + abs(right - left)); // Re-allocate max. once
  }
  if (left > right) {
    for (int i = 0; i < left - right; i++) {
      string += ')';
    }
  } else if (right > left) {
    for (int i = 0; i < right - left; i++) {
      string = String(F("(")) + string; // This is quite 'expensive'
    }
  }
  return left - right;
}

bool conditionMatch(const String& check) {
  int  posStart, posEnd;
  char compare;

  if (!findCompareCondition(check, compare, posStart, posEnd)) {
    return false;
  }

  String tmpCheck1 = check.substring(0, posStart);
  String tmpCheck2 = check.substring(posEnd);

  tmpCheck1.trim();
  tmpCheck2.trim();
  double Value1 = 0;
  double Value2 = 0;

  int  timeInSec1 = 0;
  int  timeInSec2 = 0;
  String timeString1, timeString2;
  bool validTime1 = timeStringToSeconds(tmpCheck1, timeInSec1, timeString1);
  bool validTime2 = timeStringToSeconds(tmpCheck2, timeInSec2, timeString2);
  bool result     = false;

  bool compareTimes = false;

  if ((validTime1 || validTime2) && (timeInSec1 != -1) && (timeInSec2 != -1))
  {
    // At least one is a time containing ':' separator
    // AND both can be considered a time, so use it as a time and compare seconds.
    compareTimes = true;
    result       = compareIntValues(compare, timeInSec1, timeInSec2);
    tmpCheck1    = timeString1;
    tmpCheck2    = timeString2;
  } else {
    int condAnd = tmpCheck2.indexOf(F(" and "));
    int condOr  = tmpCheck2.indexOf(F(" or "));
    if (condAnd > -1 || condOr > -1) {            // Only parse first condition, rest will be parsed 'later'
      if (condAnd > -1 && (condOr == -1 || condAnd < condOr)) {
        tmpCheck2 = tmpCheck2.substring(0, condAnd);
      } else if (condOr > -1) {
        tmpCheck2 = tmpCheck2.substring(0, condOr);
      }
      tmpCheck2.trim();
    }
    balanceParentheses(tmpCheck1);
    balanceParentheses(tmpCheck2);
    if (isError(Calculate(tmpCheck1, Value1)) ||
        isError(Calculate(tmpCheck2, Value2)))
    {
      return false;
    }
    result = compareDoubleValues(compare, Value1, Value2);
  }

  #ifndef BUILD_NO_DEBUG

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("conditionMatch: ");
    log += wrap_String(check, '"');
    log += F(" --> ");

    log += wrap_String(tmpCheck1, '"');
    log += wrap_String(check.substring(posStart, posEnd), ' '); // Compare
    log += wrap_String(tmpCheck2, '"');

    log += F(" --> ");
    log += boolToString(result);
    log += ' ';

    log += '(';
    const bool trimTrailingZeros = true;
    log += compareTimes ? String(timeInSec1) : doubleToString(Value1, 6, trimTrailingZeros);
    log += wrap_String(check.substring(posStart, posEnd), ' '); // Compare
    log += compareTimes ? String(timeInSec2) : doubleToString(Value2, 6, trimTrailingZeros);
    log += ')';
    addLogMove(LOG_LEVEL_DEBUG, log);
  }
  #else // ifndef BUILD_NO_DEBUG
  (void)compareTimes; // To avoid compiler warning
  #endif // ifndef BUILD_NO_DEBUG
  return result;
}

/********************************************************************************************\
   Generate rule events based on task refresh
 \*********************************************************************************************/
void createRuleEvents(struct EventStruct *event) {
  if (!Settings.UseRules) {
    return;
  }
  const deviceIndex_t DeviceIndex = getDeviceIndex_from_TaskIndex(event->TaskIndex);

  if (!validDeviceIndex(DeviceIndex)) { return; }

  #ifdef USE_SECOND_HEAP
//  HeapSelectIram ephemeral;  
// TD-er: Disabled for now, suspect for causing crashes
  #endif


  LoadTaskSettings(event->TaskIndex);

  const uint8_t valueCount = getValueCountForTask(event->TaskIndex);

  // Small optimization as sensor type string may result in large strings
  // These also only yield a single value, so no need to check for combining task values.
  if (event->sensorType == Sensor_VType::SENSOR_TYPE_STRING) {
    size_t expectedSize = 2 + getTaskDeviceName(event->TaskIndex).length();
    expectedSize += strlen(ExtraTaskSettings.TaskDeviceValueNames[0]);
   
    bool appendCompleteStringvalue = false;
    String eventString;

    if (eventString.reserve(expectedSize + event->String2.length())) {
      appendCompleteStringvalue = true;
    } else if (!eventString.reserve(expectedSize + 24)) {
      // No need to continue as we can't even allocate the event, we probably also cannot process it
      addLog(LOG_LEVEL_ERROR, F("Not enough memory for event"));
      return;
    }
    eventString += getTaskDeviceName(event->TaskIndex);
    eventString += '#';
    eventString += ExtraTaskSettings.TaskDeviceValueNames[0];
    eventString += '=';
    eventString += '`';
    if (appendCompleteStringvalue) {
      eventString += event->String2;
    } else {
      eventString += event->String2.substring(0, 10);
      eventString += F("...");
      eventString += event->String2.substring(event->String2.length() - 10);
    }
    eventString += '`';
    eventQueue.addMove(std::move(eventString));    
  } else if (Settings.CombineTaskValues_SingleEvent(event->TaskIndex)) {
    String eventString;
    eventString.reserve(128); // Enough for most use cases, prevent lots of memory allocations.
    eventString += getTaskDeviceName(event->TaskIndex);
    eventString += F("#All=");

    for (uint8_t varNr = 0; varNr < valueCount; varNr++) {
      if (varNr != 0) {
        eventString += ',';
      }
      eventString += formatUserVarNoCheck(event, varNr);
    }
    eventQueue.addMove(std::move(eventString));
  } else {
    for (uint8_t varNr = 0; varNr < valueCount; varNr++) {
      String eventString;
      eventString.reserve(64); // Enough for most use cases, prevent lots of memory allocations.
      eventString += getTaskDeviceName(event->TaskIndex);
      eventString += '#';
      eventString += ExtraTaskSettings.TaskDeviceValueNames[varNr];
      eventString += '=';
      eventString += formatUserVarNoCheck(event, varNr);
      eventQueue.addMove(std::move(eventString));
    }
  }
}
