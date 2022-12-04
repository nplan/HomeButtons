#include "utils.h"

#include "config.h"

String check_button_label(String label) {
  if (label.length() < 1) {
    return String("-");
  } else {
    return label.substring(0, BTN_LABEL_MAXLEN);
  }
}