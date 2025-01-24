#pragma once

#include "../../_Plugin_Helper.h"

#ifdef USES_P139

#include <AXP2101_settings.h>

// **********************************************************************
// Charge LED settings
// **********************************************************************
class AXP2101_ChargeLED_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_ChargeLED_FormSelector(AXP2101_chargeled_d selected);
  virtual ~AXP2101_ChargeLED_FormSelector() {}

  virtual String getOptionString(int index) const override;
  virtual int    getIndexValue(int index) const override;

};


// **********************************************************************
// Reg 61: Iprechg Charger Settings
// 0 .. 200 mA in 25 mA steps
// **********************************************************************
class AXP2101_PreChargeCurrentLimit_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_PreChargeCurrentLimit_FormSelector(int selected);
  virtual ~AXP2101_PreChargeCurrentLimit_FormSelector() {}

  virtual String getOptionString(int index) const override;
  virtual int    getIndexValue(int index) const override;

};


// **********************************************************************
// Reg 62: ICC Charger Settings
// 0 .. 200 mA in 25 mA steps
// 200 ... 1000 mA in 100 mA steps
// **********************************************************************
class AXP2101_ConstChargeCurrentLimit_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_ConstChargeCurrentLimit_FormSelector(int selected);
  virtual ~AXP2101_ConstChargeCurrentLimit_FormSelector() {}

  virtual String getOptionString(int index) const override;
  virtual int    getIndexValue(int index) const override;

};



// **********************************************************************
// Reg 63: Iterm Charger Settings and Control
// 0 .. 200 mA in 25 mA steps  + enable checkbox
// **********************************************************************
class AXP2101_TerminationChargeCurrentLimit_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_TerminationChargeCurrentLimit_FormSelector(int selected);
  virtual ~AXP2101_TerminationChargeCurrentLimit_FormSelector() {}

  virtual String getOptionString(int index) const override;
  virtual int    getIndexValue(int index) const override;

};


// **********************************************************************
// Reg 64: CV Charger Voltage Settings
// **********************************************************************
class AXP2101_CV_charger_voltage_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_CV_charger_voltage_FormSelector(AXP2101_CV_charger_voltage_e selected);
  virtual ~AXP2101_CV_charger_voltage_FormSelector() {}

  virtual String getOptionString(int index) const override;
  virtual int    getIndexValue(int index) const override;

};



// **********************************************************************
// Reg 14: Minimum System Voltage Control
// **********************************************************************
class AXP2101_Linear_Charger_Vsys_dpm_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_Linear_Charger_Vsys_dpm_FormSelector(AXP2101_Linear_Charger_Vsys_dpm_e selected);
  virtual ~AXP2101_Linear_Charger_Vsys_dpm_FormSelector() {}

  virtual String getOptionString(int index) const override;

};



// **********************************************************************
// Reg 15: Input Voltage Limit
// **********************************************************************
class AXP2101_Vin_DPM_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_Vin_DPM_FormSelector(AXP2101_VINDPM_e selected);
  virtual ~AXP2101_Vin_DPM_FormSelector() {}

  virtual String getOptionString(int index) const override;

};



// **********************************************************************
// Reg 16: Input Current Limit
// **********************************************************************
class AXP2101_InputCurrentLimit_FormSelector : public FormSelectorOptions
{
public:
  AXP2101_InputCurrentLimit_FormSelector(AXP2101_InputCurrentLimit_e selected);
  virtual ~AXP2101_InputCurrentLimit_FormSelector() {}

  virtual String getOptionString(int index) const override;

};



#endif