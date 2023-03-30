#include "app.h"

static App *app;

void setup() {
  app = new App();
  app->setup();
}

void loop() {}
