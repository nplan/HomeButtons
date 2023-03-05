#ifndef HOMEBUTTONS_TYPES_H
#define HOMEBUTTONS_TYPES_H

#include "static_string.h"
#include "config.h"

using SerialNumber = StaticString<8>;
using RandomID = StaticString<6>;
using ModelName = StaticString<20>;
using ModelID = StaticString<2>;
using HWVersion = StaticString<3>;
using UniqueID = StaticString<20>;

using DeviceName = StaticString<20>;
using ButtonLabel = StaticString<BTN_LABEL_MAXLEN>;

#endif  // HOMEBUTTONS_TYPES_H;