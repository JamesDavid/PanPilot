// recipe_files.h — saved recipe programs on LittleFS (/programs/*.json, the
// web Recipe Creator's save target). Closes the bench 2026-07-12 gap: saved
// programs were write-only — nothing on the device could list or RUN them.
// Device-only.
#pragma once
#if !defined(PANPILOT_SIM)
#include "core/recipe.h"

namespace hal {

// Basenames (no ".json") of saved programs. Returns the count (<= max).
int recipe_files_list(char names[][24], int max);

// Load + RE-VALIDATE a saved program into static storage (one loaded program
// at a time — it must outlive the whole cook). Returns nullptr on missing
// file, parse error, or validator rejection.
const RecipeProgram* recipe_file_load(const char* name);

}  // namespace hal
#endif
