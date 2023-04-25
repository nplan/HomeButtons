#ifndef HOMEBUTTONS_TYPES_H
#define HOMEBUTTONS_TYPES_H

#include "static_string.h"
#include "config.h"

using SerialNumber = StaticString<8>;
using RandomID = StaticString<6>;
using ModelName = StaticString<20>;
using ModelID = StaticString<2>;
using HWVersion = StaticString<3>;
using UniqueID = StaticString<21>;

using DeviceName = StaticString<20>;
using ButtonLabel = StaticString<BTN_LABEL_MAXLEN>;
using MDIName = StaticString<48>;

#endif  // HOMEBUTTONS_TYPES_H;