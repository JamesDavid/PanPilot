// recipe_files.cpp — see recipe_files.h.
#if !defined(PANPILOT_SIM)
#include "hal/recipe_files.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "core/recipe_validate.h"

namespace hal {
namespace {
// One loaded program at a time, in static storage: the sequencer holds the
// pointers for the entire cook.
RecipeStep s_steps[40];
char s_say[40][40];
char s_name[24];
RecipeProgram s_prog;

StepType stepType(const char* s) {
  if (!strcmp(s, "HOLD")) return StepType::HOLD;
  if (!strcmp(s, "CUE")) return StepType::CUE;
  if (!strcmp(s, "TIMER")) return StepType::TIMER;
  if (!strcmp(s, "PREP")) return StepType::PREP;
  if (!strcmp(s, "LOOP")) return StepType::LOOP;
  return StepType::END;
}
}  // namespace

int recipe_files_list(char names[][24], int max) {
  if (!LittleFS.begin(false)) return 0;
  File dir = LittleFS.open("/programs");
  if (!dir || !dir.isDirectory()) return 0;
  int n = 0;
  for (File f = dir.openNextFile(); f && n < max; f = dir.openNextFile()) {
    String nm = f.name();                    // basename on LittleFS
    if (!nm.endsWith(".json")) continue;
    nm.remove(nm.length() - 5);
    strncpy(names[n], nm.c_str(), 23);
    names[n][23] = '\0';
    ++n;
  }
  return n;
}

const RecipeProgram* recipe_file_load(const char* name) {
  if (!name || !name[0] || !LittleFS.begin(false)) return nullptr;
  File f = LittleFS.open(String("/programs/") + name + ".json", "r");
  if (!f) return nullptr;
  JsonDocument doc;
  const bool bad = deserializeJson(doc, f) != DeserializationError::Ok;
  f.close();
  if (bad) {
    Serial.printf("[recipe] %s.json: parse error\n", name);
    return nullptr;
  }
  int n = 0;
  for (JsonObject st : doc["steps"].as<JsonArray>()) {
    if (n >= 40) break;
    s_steps[n].type = stepType(st["type"] | "END");
    s_steps[n].a = st["a"] | 0;
    s_steps[n].b = st["b"] | 0;
    s_steps[n].expect = (uint8_t)(st["expect"] | 0);
    strncpy(s_say[n], st["say"] | "", sizeof(s_say[n]) - 1);
    s_say[n][sizeof(s_say[n]) - 1] = '\0';
    s_steps[n].say = s_say[n];
    ++n;
  }
  // Re-validate on load: the file may predate a validator rule, or have been
  // hand-edited. The validator is the source of truth (M20).
  RecipeVerdict v = recipe_validate(s_steps, n);
  if (!v.ok) {
    Serial.printf("[recipe] %s.json REJECTED: %s (step %d)\n", name, v.reason,
                  v.badStep);
    return nullptr;
  }
  strncpy(s_name, doc["name"] | name, sizeof(s_name) - 1);
  s_name[sizeof(s_name) - 1] = '\0';
  s_prog = {s_name, s_steps, n, doc["brown"] | false};
  return &s_prog;
}

}  // namespace hal
#endif
