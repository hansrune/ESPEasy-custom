#include "_Plugin_Helper.h"
#ifdef USES_P084

// #######################################################################################################
// #################################### Plugin 084: VEML6070 UV ##########################################
// #######################################################################################################

// ESPEasy Plugin to for UV with chip VEML6070
// written by Remco van Essen (https://github.com/RemCom)
// Based on VEML6070 plugin from Sonoff-Tasmota (https://github.com/arendst/Sonoff-Tasmota)
// Datasheet: https://www.vishay.com/docs/84277/veml6070.pdf


# define PLUGIN_084
# define PLUGIN_ID_084         84
# define PLUGIN_NAME_084       "UV - VEML6070"
# define PLUGIN_VALUENAME1_084 "Raw"
# define PLUGIN_VALUENAME2_084 "Risk"
# define PLUGIN_VALUENAME3_084 "Power"

# define VEML6070_ADDR_H             0x39
# define VEML6070_ADDR_L             0x38
# define VEML6070_RSET_DEFAULT       270000       // 270K default resistor value 270000 ohm, range from 220K..1Meg
# define VEML6070_UV_MAX_INDEX       15           // normal 11, internal on weather laboratories and NASA it's 15 so far the sensor is
                                                  // linear
# define VEML6070_UV_MAX_DEFAULT     11           // 11 = public default table values
# define VEML6070_POWER_COEFFCIENT   0.025f       // based on calculations from Karel Vanicek and reorder by hand
# define VEML6070_TABLE_COEFFCIENT   32.86270591f // calculated by hand with help from a friend of mine, a professor which works in aero
                                                  // space
                                                  // things
                                                  // (resistor, differences, power coefficients and official UV index calculations (LAT &
                                                  // LONG
                                                  // will be added later)

# define VEML6070_base_value ((VEML6070_RSET_DEFAULT / VEML6070_TABLE_COEFFCIENT) / VEML6070_UV_MAX_DEFAULT) * (1)
# define VEML6070_max_value  ((VEML6070_RSET_DEFAULT / VEML6070_TABLE_COEFFCIENT) / VEML6070_UV_MAX_DEFAULT) * (VEML6070_UV_MAX_INDEX)

boolean Plugin_084(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      auto& dev = Device[++deviceCount];
      dev.Number         = PLUGIN_ID_084;
      dev.Type           = DEVICE_TYPE_I2C;
      dev.VType          = Sensor_VType::SENSOR_TYPE_SINGLE;
      dev.FormulaOption  = true;
      dev.ValueCount     = 3;
      dev.SendDataOption = true;
      dev.TimerOption    = true;
      dev.PluginStats    = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_084);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_084));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_084));
      strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_084));
      break;
    }

    case PLUGIN_I2C_HAS_ADDRESS:
    {
      success = (event->Par1 == 0x38);
      break;
    }

    # if FEATURE_I2C_GET_ADDRESS
    case PLUGIN_I2C_GET_ADDRESS:
    {
      event->Par1 = 0x38;
      success     = true;
      break;
    }
    # endif // if FEATURE_I2C_GET_ADDRESS

    case PLUGIN_WEBFORM_LOAD:
    {
      const __FlashStringHelper *optionsMode[] = { F("1/2T"), F("1T"), F("2T"), F("4T (Default)") };
      constexpr size_t optionCount             = NR_ELEMENTS(optionsMode);
      const FormSelectorOptions selector(optionCount, optionsMode);
      selector.addFormSelector(F("Refresh Time Determination"), F("itime"), PCONFIG(0));

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE:
    {
      PCONFIG(0) = getFormItemInt(F("itime"));

      success = true;
      break;
    }

    case PLUGIN_INIT:
    {
      success = VEML6070_Init(PCONFIG(0));

      if (!success) {
        addLog(LOG_LEVEL_ERROR, F("VEML6070: Not available!"));
      }

      break;
    }

    case PLUGIN_READ:
    {
      uint16_t uv_raw;
      ESPEASY_RULES_FLOAT_TYPE uv_risk, uv_power;
      bool read_status;

      uv_raw   = VEML6070_ReadUv(&read_status); // get UV raw values
      uv_risk  = VEML6070_UvRiskLevel(uv_raw);  // get UV risk level
      uv_power = VEML6070_UvPower(uv_risk);     // get UV power in W/m2

      if (isnan(uv_raw) || (uv_raw == 65535) || !read_status) {
        addLog(LOG_LEVEL_ERROR, F("VEML6070: no data read!"));
        UserVar.setFloat(event->TaskIndex, 0, NAN);
        UserVar.setFloat(event->TaskIndex, 1, NAN);
        UserVar.setFloat(event->TaskIndex, 2, NAN);
        success = false;
      } else {
        UserVar.setFloat(event->TaskIndex, 0, uv_raw);
        UserVar.setFloat(event->TaskIndex, 1, uv_risk);
        UserVar.setFloat(event->TaskIndex, 2, uv_power);

        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          addLogMove(LOG_LEVEL_INFO, concat(F("VEML6070: UV: "), formatUserVarNoCheck(event, 0)));
        }

        success = true;
      }

      break;
    }
  }
  return success;
}

//////////////
// VEML6070 //
//////////////

// get UV raw values
uint16_t VEML6070_ReadUv(bool *status)
{
  uint16_t uv_raw      = 0;
  bool     wire_status = false;

  uv_raw   = I2C_read8(VEML6070_ADDR_H, &wire_status) << 8;
  *status  = wire_status;
  uv_raw  |= I2C_read8(VEML6070_ADDR_L, &wire_status);
  *status &= wire_status;

  return uv_raw;
}

bool VEML6070_Init(uint8_t it)
{
  return I2C_write8(VEML6070_ADDR_L, ((it << 2) | 0x02));
}

// Definition of risk numbers
//  0.0 - 2.9  "Low"      = sun->fun
//  3.0 - 5.9  "Mid"      = sun->glases advised
//  6.0 - 7.9  "High"     = sun->glases a must
//  8.0 - 10.9 "Danger"   = sun->skin burns Level 1
// 11.0 - 12.9 "BurnL1/2" = sun->skin burns level 1..2
// 13.0 - 25.0 "BurnL3"   = sun->skin burns with level 3

ESPEASY_RULES_FLOAT_TYPE VEML6070_UvRiskLevel(uint16_t uv_level)
{
  ESPEASY_RULES_FLOAT_TYPE risk{};

  constexpr ESPEASY_RULES_FLOAT_TYPE max_value = VEML6070_max_value;

  if (uv_level < max_value) {
    constexpr ESPEASY_RULES_FLOAT_TYPE factor = VEML6070_base_value;
    return (ESPEASY_RULES_FLOAT_TYPE)uv_level / factor;
  } else {
    // out of range and much to high - it must be outerspace or sensor damaged
    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLogMove(LOG_LEVEL_INFO, concat(F("VEML6070 out of range: "), risk));
    }

    return (ESPEASY_RULES_FLOAT_TYPE)99;
  }
}

ESPEASY_RULES_FLOAT_TYPE VEML6070_UvPower(ESPEASY_RULES_FLOAT_TYPE uvrisk)
{
  // based on calculations for effective irradiation from Karel Vanicek
  return VEML6070_POWER_COEFFCIENT * uvrisk;
}

#endif // USES_P084
