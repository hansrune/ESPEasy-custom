#pragma once

#include "../../_Plugin_Helper.h"

#ifdef USES_P139

#include <AXP2101_settings.h>

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




#endif