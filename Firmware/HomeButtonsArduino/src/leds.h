#ifndef C77B45E9_4260_40BC_A071_406A2B1A3B59
#define C77B45E9_4260_40BC_A071_406A2B1A3B59

#include "Arduino.h"
#include "config.h"

class LEDs {
 public:
  enum class State { IDLE, ACTIVE };

  void begin();
  void end();
  void blink(uint8_t led_num, uint8_t num_blinks, bool hold = false,
             uint8_t brightness = LED_DFLT_BRIGHT);
  State get_state();

  void update();

 private:
  enum class CMDState { NONE, CMD_END };

  struct Blink {
    uint8_t led_num = 0;
    uint8_t num_blinks = 0;
    uint8_t brightness = 0;
    bool hold = false;
  };

  State state = State::IDLE;
  CMDState cmd_state = CMDState::NONE;
  Blink cmd_blink = {};
  bool new_cmd_blink = false;
  Blink current_blink = {};
};

extern LEDs leds;

#endif /* C77B45E9_4260_40BC_A071_406A2B1A3B59 */
