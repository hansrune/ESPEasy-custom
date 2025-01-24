#ifndef DATATYPES_FORMSELECTOROPTIONS_H
#define DATATYPES_FORMSELECTOROPTIONS_H

#include "../../ESPEasy_common.h"

class FormSelectorOptions {
public:

  FormSelectorOptions(int optionCount);

  FormSelectorOptions(int          optionCount,
                      const int    indices[],
                      const String attr[] = nullptr);

  FormSelectorOptions(int          optionCount,
                      const String options[],
                      const int    indices[] = nullptr,
                      const String attr[] = nullptr);
  FormSelectorOptions(int                        optionCount,
                      const __FlashStringHelper *options[],
                      const int                  indices[] = nullptr,
                      const String attr[] = nullptr);


  virtual ~FormSelectorOptions();

  // Return either a string representation of the current index,
  // or the override implementation in a class inheriting of this class
  virtual String getOptionString(int index) const;

  virtual int    getIndexValue(int index) const;

  void           addFormSelector(const __FlashStringHelper *label,
                                 const __FlashStringHelper *id,
                                 int                        selectedIndex);

  void           addFormSelector(const String&label,
                                 const __FlashStringHelper *id,
                                 int                        selectedIndex);

  void           addFormSelector(const __FlashStringHelper *label,
                                 const String& id,
                                 int                        selectedIndex);

  void addFormSelector(const String& label,
                       const String& id,
                       int           selectedIndex);


  bool reloadonchange = false;
  bool enabled = true;
  const __FlashStringHelper * classname;
#if FEATURE_TOOLTIPS
  String tooltip;
#endif // if FEATURE_TOOLTIPS
  String onChangeCall;

protected:

  int _optionCount{};
  const __FlashStringHelper **_names_f{ nullptr };
  String *_names_str{ nullptr };
  int *_indices{ nullptr };
  String *_attr_str{ nullptr };
};


#endif // ifndef DATATYPES_FORMSELECTOROPTIONS_H
