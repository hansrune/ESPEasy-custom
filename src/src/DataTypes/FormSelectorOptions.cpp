#include "../DataTypes/FormSelectorOptions.h"

#include "../WebServer/Markup.h"

FormSelectorOptions::FormSelectorOptions(int optionCount) 
: _optionCount(optionCount) 
{
  classname = F("wide");
}

FormSelectorOptions::FormSelectorOptions(
  int          optionCount,
  const int    indices[],
  const String attr[]) : _optionCount(optionCount)
{
  classname = F("wide");
  _indices = new int[optionCount];

  if (attr != nullptr) {
    _attr_str = new String[optionCount];
  }

  for (int i = 0; i < optionCount; ++i) {
    _indices[i] = indices[i];

    if (attr != nullptr) {
      _attr_str[i] = attr[i];
    }
  }
}

FormSelectorOptions::FormSelectorOptions(
  int          optionCount,
  const String options[],
  const int    indices[],
  const String attr[]) : _optionCount(optionCount)
{
  classname = F("wide");
  _names_str = new String[optionCount];

  if (indices != nullptr) {
    _indices = new int[optionCount];
  }

  if (attr != nullptr) {
    _attr_str = new String[optionCount];
  }


  for (int i = 0; i < optionCount; ++i) {
    _names_str[i] = options[i];

    if (indices != nullptr) {
      _indices[i] = indices[i];
    }

    if (attr != nullptr) {
      _attr_str[i] = attr[i];
    }
  }
}

FormSelectorOptions::FormSelectorOptions(
  int                        optionCount,
  const __FlashStringHelper *options[],
  const int                  indices[],
  const String               attr[]) : _optionCount(optionCount)
{
  classname = F("wide");
  _names_f = new const __FlashStringHelper *[optionCount];

  if (indices != nullptr) {
    _indices = new int[optionCount];
  }

  if (attr != nullptr) {
    _attr_str = new String[optionCount];
  }

  for (int i = 0; i < optionCount; ++i) {
    _names_f[i] = options[i];

    if (indices != nullptr) {
      _indices[i] = indices[i];
    }

    if (attr != nullptr) {
      _attr_str[i] = attr[i];
    }
  }
}

FormSelectorOptions::~FormSelectorOptions() {
    #define DELETE_ARR(A) \
            if (A != nullptr) { delete[] A; A = nullptr; }
  DELETE_ARR(_names_f)
  DELETE_ARR(_names_str)
  DELETE_ARR(_indices)
  DELETE_ARR(_attr_str)
    #undef DELETE_ARR
}

String FormSelectorOptions::getOptionString(int index) const
{
  if (index >= _optionCount) {
    return EMPTY_STRING;
  }

  if (_names_f != nullptr) {
    return String(_names_f[index]);
  }

  if (_names_str != nullptr) {
    return String(_names_str[index]);
  }

  if (_indices != nullptr) {
    return String(_indices[index]);
  }
  return String(index);
}

int FormSelectorOptions::getIndexValue(int index) const
{
  if (index >= _optionCount) {
    return -1;
  }

  if (_indices != nullptr) {
    return _indices[index];
  }
  return index;
}

void FormSelectorOptions::addFormSelector(
  const __FlashStringHelper *label,
  const __FlashStringHelper *id,
  int                        selectedIndex)
{
  addFormSelector(String(label), String(id), selectedIndex);
}

void FormSelectorOptions::addFormSelector(
  const String             & label,
  const __FlashStringHelper *id,
  int                        selectedIndex)
{
  addFormSelector(label, String(id), selectedIndex);
}

void FormSelectorOptions::addFormSelector(
  const __FlashStringHelper *label,
  const String             & id,
  int                        selectedIndex)
{
  addFormSelector(String(label), id, selectedIndex);
}

void FormSelectorOptions::addFormSelector(
  const String& label,
  const String& id,
  int           selectedIndex)
{
  addRowLabel_tr_id(label, id);

  // FIXME TD-er Change bool 'enabled' to disabled
  if (reloadonchange)
  {
    addSelector_Head_reloadOnChange(id, classname, !enabled
                                    #if FEATURE_TOOLTIPS
                                    , tooltip
                                    #endif // if FEATURE_TOOLTIPS
                                    );
  } else {
    do_addSelector_Head(id, classname, EMPTY_STRING, !enabled
                        #if FEATURE_TOOLTIPS
                        , tooltip
                        #endif // if FEATURE_TOOLTIPS
                        );
  }

  for (int i = 0; i < _optionCount; ++i)
  {
    const int index = getIndexValue(i);
    addSelector_Item(
      getOptionString(i),
      index,
      selectedIndex == index,
      false,
      _attr_str ? _attr_str[i] : EMPTY_STRING);

    if ((i & 0x07) == 0) { delay(0); }
  }
  addSelector_Foot();
}
