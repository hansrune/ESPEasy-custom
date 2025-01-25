#include "../DataTypes/FormSelectorOptions.h"

#include "../WebServer/Markup.h"
#include "../WebServer/Markup_Forms.h"
#include "../WebServer/HTML_wrappers.h"

FormSelectorOptions::FormSelectorOptions(int optionCount)
  : _optionCount(optionCount)
{
  classname = F("wide");
}

FormSelectorOptions::FormSelectorOptions(
  int          optionCount,
  const int    indices[],
  const String attr[]) :
  _optionCount(optionCount),
  _indices(indices),
  _attr_str(attr)
{
  classname = F("wide");
}

FormSelectorOptions::FormSelectorOptions(
  int          optionCount,
  const String options[],
  const int    indices[],
  const String attr[]) :
  _optionCount(optionCount),
  _names_str(options),
  _indices(indices),
  _attr_str(attr)
{
  classname = F("wide");
}

FormSelectorOptions::FormSelectorOptions(
  int                        optionCount,
  const __FlashStringHelper *options[],
  const int                  indices[],
  const String               attr[]) :
  _optionCount(optionCount),
  _names_f(options),
  _indices(indices),
  _attr_str(attr)
{
  classname = F("wide");
}

FormSelectorOptions::~FormSelectorOptions() {}

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
    do_addSelector_Head(id, classname, onChangeCall, !enabled
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
  if (reloadonchange) {
    addHtml(F("&#128260;"));
//    addFormNote(F("Page will reload when selection is changed."));
  }
}
