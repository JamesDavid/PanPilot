// food_json.cpp — see food_json.h.
#if !defined(PANPILOT_SIM)
#include "food_json.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

namespace hal {

int load_custom_foods(FoodStore& store) {
  if (!LittleFS.begin(false) || !LittleFS.exists("/foods.json")) return -1;
  File f = LittleFS.open("/foods.json", "r");
  if (!f) return -1;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[foods] /foods.json parse error: %s\n", err.c_str());
    return -1;
  }

  int n = 0;
  for (JsonObject o : doc["foods"].as<JsonArray>()) {
    uint16_t sides[FOODLIB_MAX_SIDES] = {0};
    int ns = 0;
    for (JsonVariant v : o["sideSec"].as<JsonArray>())
      if (ns < FOODLIB_MAX_SIDES) sides[ns++] = (uint16_t)v.as<int>();

    if (store.add(o["name"] | "Custom", o["variant"] | "",
                  o["panLoF"] | 0, o["panHiF"] | 0,
                  sides, o["sides"] | ns,
                  o["refF"] | 0, o["compPct"] | 0,
                  o["restSec"] | 0, o["safeInternalF"] | 0,
                  o["flip"] | ""))
      ++n;
  }
  Serial.printf("[foods] loaded %d custom food(s) from /foods.json\n", n);
  return n;
}

}  // namespace hal
#endif
