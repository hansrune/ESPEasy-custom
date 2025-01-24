#include "../PluginStructs/P139_data_struct_formselectors.h"

#ifdef USES_P139


// **********************************************************************
// Charge LED settings
// **********************************************************************
AXP2101_ChargeLED_FormSelector::AXP2101_ChargeLED_FormSelector(
  AXP2101_chargeled_d selected) :
  FormSelectorOptions(5)
{
  addFormSelector(
    F("Charge LED"), F("led"),
    static_cast<int>(selected));
}

AXP2101_chargeled_d get_AXP2101_chargeled_d(int index) {
  if (index < 0) { return AXP2101_chargeled_d::Off; }

  if (index <= static_cast<int>(AXP2101_chargeled_d::Steady_On)) {
    return static_cast<AXP2101_chargeled_d>(index);
  }
  return AXP2101_chargeled_d::Protected;
}

String AXP2101_ChargeLED_FormSelector::getOptionString(int index) const
{
  return toString(get_AXP2101_chargeled_d(index));
}

int AXP2101_ChargeLED_FormSelector::getIndexValue(int index) const
{
  return static_cast<int>(get_AXP2101_chargeled_d(index));
}

// **********************************************************************
// Reg 61: Iprechg Charger Settings
// 0 .. 200 mA in 25 mA steps
// **********************************************************************
AXP2101_PreChargeCurrentLimit_FormSelector::AXP2101_PreChargeCurrentLimit_FormSelector(
  int selected) : FormSelectorOptions(9)
{
  addFormSelector(
    F("Term Charge Current"), F("iterm"),
    static_cast<int>(selected));
  addUnit(F("mA"));
}

int get_AXP2101_0_to_200mA_ChargeCurrentLimit(int index) {
  if (index < 0) { return 0; }
  const int res = 25 * index;

  if (res > 200) { return 200; }
  return res;
}

String AXP2101_PreChargeCurrentLimit_FormSelector::getOptionString(int index) const
{
  return String(get_AXP2101_0_to_200mA_ChargeCurrentLimit(index));
}

int AXP2101_PreChargeCurrentLimit_FormSelector::getIndexValue(int index) const
{
  return get_AXP2101_0_to_200mA_ChargeCurrentLimit(index);
}

// **********************************************************************
// Reg 62: ICC Charger Settings
// 0 .. 200 mA in 25 mA steps
// 200 ... 1000 mA in 100 mA steps
// **********************************************************************
AXP2101_ConstChargeCurrentLimit_FormSelector::AXP2101_ConstChargeCurrentLimit_FormSelector(
  int selected) : FormSelectorOptions(17)
{
  addFormSelector(
    F("Constant Current Charge Limit"), F("iccchg"),
    static_cast<int>(selected));
  addUnit(F("mA"));
}

int get_AXP2101_ConstChargeCurrentLimit(int index) {
  if (index < 0) { return 0; }
  const int res = index <= 8
    ? 25 * index
    : (index - 8) * 100 + 200;

  if (res > 1000) { return 1000; }
  return res;
}

String AXP2101_ConstChargeCurrentLimit_FormSelector::getOptionString(int index) const
{
  return String(get_AXP2101_ConstChargeCurrentLimit(index));
}

int AXP2101_ConstChargeCurrentLimit_FormSelector::getIndexValue(int index) const
{
  return get_AXP2101_ConstChargeCurrentLimit(index);
}

// **********************************************************************
// Reg 63: Iterm Charger Settings and Control
// 0 .. 200 mA in 25 mA steps  + enable checkbox
// **********************************************************************
AXP2101_TerminationChargeCurrentLimit_FormSelector::AXP2101_TerminationChargeCurrentLimit_FormSelector(
  int selected) : FormSelectorOptions(9)
{
  addFormSelector(
    F("Pre Charge Current"), F("iprechg"),
    static_cast<int>(selected));
  addUnit(F("mA"));
}

String AXP2101_TerminationChargeCurrentLimit_FormSelector::getOptionString(int index) const
{
  return String(get_AXP2101_0_to_200mA_ChargeCurrentLimit(index));
}

int AXP2101_TerminationChargeCurrentLimit_FormSelector::getIndexValue(int index) const
{
  return get_AXP2101_0_to_200mA_ChargeCurrentLimit(index);
}

// **********************************************************************
// Reg 64: CV Charger Voltage Settings
// **********************************************************************
AXP2101_CV_charger_voltage_FormSelector::AXP2101_CV_charger_voltage_FormSelector(
  AXP2101_CV_charger_voltage_e selected) :
  FormSelectorOptions(static_cast<int>(AXP2101_CV_charger_voltage_e::MAX) - 1)
{
  addFormSelector(
    F("CV Charger Voltage"), F("cv_volt"),
    static_cast<int>(selected));
  addUnit(F("V"));
}

AXP2101_CV_charger_voltage_e get_AXP2101_CV_charger_voltage_e(int index) {
  if (index < 0) { return AXP2101_CV_charger_voltage_e::reserved; }
  constexpr int offset = static_cast<int>(AXP2101_CV_charger_voltage_e::limit_4_00V);
  constexpr int max    = static_cast<int>(AXP2101_CV_charger_voltage_e::MAX);
  index += offset;

  if (index >= max) { return AXP2101_CV_charger_voltage_e::reserved; }
  return static_cast<AXP2101_CV_charger_voltage_e>(index);
}

String AXP2101_CV_charger_voltage_FormSelector::getOptionString(int index) const
{
  return toString(get_AXP2101_CV_charger_voltage_e(index));
}

int AXP2101_CV_charger_voltage_FormSelector::getIndexValue(int index) const
{
  return static_cast<int>(get_AXP2101_CV_charger_voltage_e(index));
}

// **********************************************************************
// Reg 14: Minimum System Voltage Control
// **********************************************************************
AXP2101_Linear_Charger_Vsys_dpm_FormSelector::AXP2101_Linear_Charger_Vsys_dpm_FormSelector(
  AXP2101_Linear_Charger_Vsys_dpm_e selected) :
  FormSelectorOptions(static_cast<int>(AXP2101_Linear_Charger_Vsys_dpm_e::MAX))
{
  addFormSelector(
    F("Minimum System Voltage"), F("min_vsys"),
    static_cast<int>(selected));
  addUnit(F("V"));
}

String AXP2101_Linear_Charger_Vsys_dpm_FormSelector::getOptionString(int index) const
{
  constexpr int max = static_cast<int>(AXP2101_Linear_Charger_Vsys_dpm_e::MAX);

  if ((index >= max) || (index < 0)) {
    return EMPTY_STRING;
  }
  return concat(F("4."), index + 1);
}

// **********************************************************************
// Reg 15: Input Voltage Limit
// **********************************************************************
AXP2101_Vin_DPM_FormSelector::AXP2101_Vin_DPM_FormSelector(
  AXP2101_VINDPM_e selected) :
  FormSelectorOptions(static_cast<int>(AXP2101_VINDPM_e::MAX))
{
  addFormSelector(
    F("Min Vin DPM Voltage"), F("vin_dpm"),
    static_cast<int>(selected));
  addUnit(F("V"));
}

String AXP2101_Vin_DPM_FormSelector::getOptionString(int index) const
{
  constexpr int max = static_cast<int>(AXP2101_VINDPM_e::MAX);

  if ((index >= max) || (index < 0)) {
    return EMPTY_STRING;
  }
  return toString(3.88f + 0.08f * index, 2);
}

// **********************************************************************
// Reg 16: Input Current Limit
// **********************************************************************
AXP2101_InputCurrentLimit_FormSelector::AXP2101_InputCurrentLimit_FormSelector(
  AXP2101_InputCurrentLimit_e selected) :
  FormSelectorOptions(static_cast<int>(AXP2101_InputCurrentLimit_e::MAX))
{
  addFormSelector(
    F("Input Current Limit"), F("cur_lim_in"),
    static_cast<int>(selected));
  addUnit(F("mA"));
}

String AXP2101_InputCurrentLimit_FormSelector::getOptionString(int index) const
{
  constexpr int max = static_cast<int>(AXP2101_InputCurrentLimit_e::MAX);

  if ((index >= max) || (index < 0)) {
    return EMPTY_STRING;
  }
  const int value = (index < 3)
    ? 100 + (400 * index)
    : 1000 + ((index - 3)) * 500;
  return String(value);
}

#endif // ifdef USES_P139
