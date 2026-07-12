// net.cpp — see net.h.
#if !defined(PANPILOT_SIM) && defined(ENABLE_WIFI)
#include "net.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <ArduinoJson.h>
#include <LittleFS.h>

#include <esp_system.h>

#include "web_assets.h"
#include "web_creator.h"
#include "hal/weblog.h"
#include "app_config.h"
#include "core/thermal_model.h"
#include "core/presets.h"
#include "core/foodlib.h"
#include "core/recipe.h"
#include "core/recipe_validate.h"
#include "core/preplib.h"
#include "core/timezones.h"
#include "hal/storage.h"
#include "hal/session_store.h"

namespace net {
namespace {
WiFiManager s_wm;
AsyncWebServer s_server(80);
AsyncWebSocket s_ws("/ws");
bool s_portal = false;
char s_ap[20] = "PanPilot";   // portal SSID, shown on the Settings Wi-Fi row

// Settings mirror. GET reads this cache (refreshed by publishState); POST stages
// an edit that net::loop() applies on the loop core via these callbacks.
UnitCb s_unitCb = nullptr;
SetMuteCb s_muteCb = nullptr;
SetBrightCb s_brightCb = nullptr;
SetTzCb s_tzCb = nullptr;
volatile bool s_cUseF = true, s_cMuted = false, s_cTimeValid = false;
volatile uint8_t s_cBright = 2, s_cTz = 0, s_cH = 0, s_cM = 0;
volatile bool s_pUnit = false, s_pMute = false, s_pBright = false, s_pTz = false;
volatile bool s_pUnitVal = true, s_pMuteVal = false;
volatile uint8_t s_pBrightVal = 2, s_pTzVal = 0;
volatile bool s_pFoods = false;      // /foods.json changed -> reload on loop core
FoodsChangedCb s_foodsCb = nullptr;

bool pinOk(const String& given) {
  const String pin = hal::storage_get_web_pin();
  return pin.isEmpty() || given == pin;   // empty PIN = open
}

StepType stepType(const char* s) {
  if (!strcmp(s, "HOLD")) return StepType::HOLD;
  if (!strcmp(s, "CUE")) return StepType::CUE;
  if (!strcmp(s, "TIMER")) return StepType::TIMER;
  if (!strcmp(s, "PREP")) return StepType::PREP;
  if (!strcmp(s, "LOOP")) return StepType::LOOP;
  return StepType::END;
}

// Parse a program JSON body into steps; validate; optionally save to LittleFS.
// Replies {ok, badStep, reason}. Recipe strings are copied into a persistent
// arena so validate() (which stores const char*) stays valid for the reply.
void handleProgram(AsyncWebServerRequest* r, const String& body, bool save) {
  static RecipeStep steps[40];
  static char arena[40][40];
  JsonDocument doc;
  if (deserializeJson(doc, body)) { r->send(400, "application/json",
      "{\"ok\":false,\"reason\":\"bad json\"}"); return; }
  JsonArray arr = doc["steps"].as<JsonArray>();
  int n = 0;
  for (JsonObject st : arr) {
    if (n >= 40) break;
    steps[n].type = stepType(st["type"] | "END");
    steps[n].a = st["a"] | 0;
    steps[n].b = st["b"] | 0;
    steps[n].expect = (uint8_t)(st["expect"] | 0);
    strncpy(arena[n], st["say"] | "", sizeof(arena[n]) - 1);
    arena[n][sizeof(arena[n]) - 1] = 0;
    steps[n].say = arena[n];
    ++n;
  }
  RecipeVerdict v = recipe_validate(steps, n);
  if (v.ok && save) {
    if (!LittleFS.exists("/programs")) LittleFS.mkdir("/programs");
    File f = LittleFS.open("/programs/" + (doc["name"].as<String>().length()
                               ? doc["name"].as<String>() : String("recipe")) + ".json", "w");
    if (f) { serializeJson(doc, f); f.close(); }
  }
  char out[128];
  snprintf(out, sizeof(out), "{\"ok\":%s,\"badStep\":%d,\"reason\":\"%s\"}",
           v.ok ? "true" : "false", v.badStep, v.reason);
  r->send(200, "application/json", out);
}

const char* gname(GuidanceState g) {
  switch (g) {
    case GuidanceState::HEAT_MORE: return "HEAT_MORE";
    case GuidanceState::HOLD: return "HOLD";
    case GuidanceState::TURN_DOWN_SOON: return "TURN_DOWN_SOON";
    case GuidanceState::TURN_DOWN_NOW: return "TURN_DOWN_NOW";
    case GuidanceState::READY: return "READY";
    case GuidanceState::TOO_HOT: return "TOO_HOT";
    case GuidanceState::COOLING: return "COOLING";
    case GuidanceState::RECOVERING: return "RECOVERING";
    case GuidanceState::CHECK_AIM: return "CHECK_AIM";
    case GuidanceState::NO_PAN: return "NO_PAN";
    default: return "IDLE";
  }
}
const char* bartext(GuidanceState g) {
  switch (g) {
    case GuidanceState::HEAT_MORE: return "Heat more";
    case GuidanceState::HOLD: return "Hold";
    case GuidanceState::TURN_DOWN_SOON: return "Turn down soon";
    case GuidanceState::TURN_DOWN_NOW: return "TURN DOWN NOW";
    case GuidanceState::READY: return "READY";
    case GuidanceState::TOO_HOT: return "TOO HOT";
    case GuidanceState::COOLING: return "Cooling";
    case GuidanceState::RECOVERING: return "Recovering";
    case GuidanceState::CHECK_AIM: return "Check aim";
    case GuidanceState::NO_PAN: return "No pan";
    default: return "--";
  }
}
}  // namespace

void begin() {
  WiFi.mode(WIFI_STA);
  snprintf(s_ap, sizeof(s_ap), "PanPilot-%04X",
           (uint16_t)(ESP.getEfuseMac() & 0xFFFF));
  static WiFiManagerParameter mqttParam("mqtt", "MQTT broker (optional)",
                                        hal::storage_get_mqtt_broker().c_str(), 40);
  static WiFiManagerParameter pinParam("pin", "Web settings PIN (optional)",
                                       hal::storage_get_web_pin().c_str(), 8);
  s_wm.addParameter(&mqttParam);
  s_wm.addParameter(&pinParam);
  s_wm.setConfigPortalBlocking(false);
  s_wm.setConfigPortalTimeout(180);
  // Persist the extra params whenever the portal saves — including a portal
  // reopened later from Settings, not just this first-boot autoConnect.
  s_wm.setSaveParamsCallback([]() {
    if (strlen(mqttParam.getValue()) > 0)
      hal::storage_set_mqtt_broker(mqttParam.getValue());
    hal::storage_set_web_pin(pinParam.getValue());
  });
  s_portal = !s_wm.autoConnect(s_ap);    // opens captive portal if unprovisioned
  if (strlen(mqttParam.getValue()) > 0)
    hal::storage_set_mqtt_broker(mqttParam.getValue());
  hal::storage_set_web_pin(pinParam.getValue());

  // mDNS is (re)registered from net::loop() on every connect transition — a
  // begin() here at boot ran before the STA had a network whenever the user
  // provisioned via the portal minutes later, and panpilot.local never
  // resolved until a reboot (bench 2026-07-11).

  s_server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", PANPILOT_INDEX_HTML);
  });

  // Session history (roadmap §2.3): JSON list + CSV download.
  s_server.on("/api/sessions", HTTP_GET, [](AsyncWebServerRequest* r) {
    uint32_t ids[20];
    int n = hal::sessions_list(ids, 20);
    String j = "[";
    for (int i = 0; i < n; ++i) {
      SessionSummary s;
      if (!hal::session_summary(ids[i], s)) continue;
      if (i) j += ',';
      j += "{\"id\":"; j += ids[i];
      j += ",\"preset\":\""; j += preset(s.presetId).name;
      j += "\",\"maxC\":"; j += (int)s.maxTempC;
      j += ",\"inRange\":"; j += s.timeInRangeSec;
      j += ",\"overheat\":"; j += s.overheatSec;
      j += ",\"food\":"; j += s.foodAddedCount; j += '}';
    }
    j += ']';
    r->send(200, "application/json", j);
  });
  s_server.on("/api/session", HTTP_GET, [](AsyncWebServerRequest* r) {
    if (!r->hasParam("id")) { r->send(400); return; }
    uint32_t id = r->getParam("id")->value().toInt();
    String csv = hal::session_csv(id);
    if (csv.isEmpty()) { r->send(404); return; }
    AsyncWebServerResponse* resp = r->beginResponse(200, "text/csv", csv);
    resp->addHeader("Content-Disposition",
                    "attachment; filename=panpilot_cook.csv");
    r->send(resp);
  });
  // Recipe Creator (roadmap §4.1.1)
  s_server.on("/creator", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", PANPILOT_CREATOR_HTML);
  });
  s_server.on("/api/foodlib", HTTP_GET, [](AsyncWebServerRequest* r) {
    String j = "[";
    for (int i = 0; i < foodlib_count(); ++i) {
      const FoodEntry& f = foodlib_entry(i);
      if (i) j += ',';
      j += "{\"name\":\""; j += f.name; j += "\",\"variant\":\""; j += f.variant;
      j += "\",\"lo\":"; j += f.panTargetF_lo; j += ",\"hi\":"; j += f.panTargetF_hi;
      j += ",\"side1\":"; j += f.sideSec[0]; j += ",\"side2\":"; j += f.sideSec[1];
      j += ",\"safe\":"; j += f.safeInternalF; j += '}';
    }
    j += ']';
    r->send(200, "application/json", j);
  });
  // New-food form on the Creator page (bench 2026-07-12 "can i make a
  // food?"): POST one food (foods.example.json field names); it is merged
  // into /foods.json (same name+variant replaces) and the store hot-reloads
  // on the loop core via the foods-changed callback. The safety floor is
  // enforced at load (safeInternalFloor) as always.
  s_server.on(
      "/api/foods", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr,
      [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index,
         size_t total) {
        static String body;
        if (index == 0) { body = ""; body.reserve(total + 1); }
        body.concat((const char*)data, len);
        if (index + len != total) return;
        JsonDocument nu;
        if (deserializeJson(nu, body) || !(nu["name"].is<const char*>())) {
          r->send(400, "application/json",
                  "{\"ok\":false,\"reason\":\"bad json\"}");
          return;
        }
        JsonDocument all;
        if (LittleFS.exists("/foods.json")) {
          File f = LittleFS.open("/foods.json", "r");
          if (f) { deserializeJson(all, f); f.close(); }
        }
        if (!all["foods"].is<JsonArray>())
          all["foods"].to<JsonArray>();
        JsonArray arr = all["foods"].as<JsonArray>();
        // Same (name, variant) replaces in place.
        for (size_t i = 0; i < arr.size(); ++i) {
          if (strcmp(arr[i]["name"] | "", nu["name"] | "") == 0 &&
              strcmp(arr[i]["variant"] | "", nu["variant"] | "") == 0) {
            arr.remove(i);
            break;
          }
        }
        arr.add(nu);
        File f = LittleFS.open("/foods.json", "w");
        if (!f) { r->send(500); return; }
        serializeJson(all, f);
        f.close();
        s_pFoods = true;   // loop core reloads the store + food library
        r->send(200, "application/json", "{\"ok\":true}");
      });

  // Fats for the Creator's PREP dropdown (name is all the page needs; the
  // firmware owns the temps/smoke points).
  s_server.on("/api/preplib", HTTP_GET, [](AsyncWebServerRequest* r) {
    String j = "[";
    for (int i = 0; i < preplib_count(); ++i) {
      if (i) j += ',';
      j += "{\"name\":\""; j += preplib_entry(i).name; j += "\"}";
    }
    j += ']';
    r->send(200, "application/json", j);
  });
  auto bodyHandler = [](bool save) {
    return [save](AsyncWebServerRequest* r, uint8_t* data, size_t len,
                  size_t index, size_t total) {
      static String body;
      if (index == 0) { body = ""; body.reserve(total + 1); }
      body.concat((const char*)data, len);
      if (index + len == total) handleProgram(r, body, save);
    };
  };
  s_server.on("/api/programs/validate", HTTP_POST,
              [](AsyncWebServerRequest*) {}, nullptr, bodyHandler(false));
  s_server.on("/api/programs/new", HTTP_PUT,
              [](AsyncWebServerRequest*) {}, nullptr, bodyHandler(true));

  // Wi-Fi debugging (bench 2026-07-12): the serial tee's ring at /log means a
  // stove-mounted device needs no cable — everything Serial.* printed since
  // (roughly) the last few minutes is one curl away. /api/health carries the
  // vitals a remote debugger wants first.
  s_server.on("/log", HTTP_GET, [](AsyncWebServerRequest* r) {
    static char buf[6400];             // static: the async task's stack is small
    weblog_snapshot(buf, sizeof(buf));
    r->send(200, "text/plain; charset=utf-8", buf);
  });
  s_server.on("/api/health", HTTP_GET, [](AsyncWebServerRequest* r) {
    char j[320];
    snprintf(j, sizeof(j),
             "{\"fw\":\"%s\",\"uptimeS\":%lu,\"heapFree\":%u,\"heapMin\":%u,"
             "\"resetReason\":%d,\"rssi\":%d,\"ip\":\"%s\"}",
             PANPILOT_FW_VERSION, (unsigned long)(millis() / 1000),
             (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
             (int)esp_reset_reason(), (int)WiFi.RSSI(),
             WiFi.localIP().toString().c_str());
    r->send(200, "application/json", j);
  });

  // Web settings mirror (Phase 2).
  s_server.on("/settings", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", PANPILOT_SETTINGS_HTML);
  });
  s_server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest* r) {
    String z = "[";
    for (int i = 0; i < tz_count(); ++i) {
      if (i) z += ',';
      z += '"'; z += tz_name(i); z += '"';
    }
    z += ']';
    char clk[8];
    snprintf(clk, sizeof(clk), "%02u:%02u", s_cH, s_cM);
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"unit\":\"%s\",\"muted\":%s,\"brightness\":%u,\"tz\":%u,"
             "\"timeValid\":%s,\"clock\":\"%s\",\"pinReq\":%s,\"zones\":%s}",
             s_cUseF ? "F" : "C", s_cMuted ? "true" : "false", s_cBright, s_cTz,
             s_cTimeValid ? "true" : "false", clk,
             hal::storage_get_web_pin().isEmpty() ? "false" : "true", z.c_str());
    r->send(200, "application/json", buf);
  });
  s_server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest*) {}, nullptr,
      [](AsyncWebServerRequest* r, uint8_t* data, size_t len, size_t index,
         size_t total) {
        static String body;
        if (index == 0) { body = ""; body.reserve(total + 1); }
        body.concat((const char*)data, len);
        if (index + len != total) return;
        JsonDocument doc;
        if (deserializeJson(doc, body)) {
          r->send(400, "application/json", "{\"ok\":false,\"reason\":\"bad json\"}");
          return;
        }
        if (!pinOk(doc["pin"] | "")) {
          r->send(403, "application/json", "{\"ok\":false,\"reason\":\"wrong PIN\"}");
          return;
        }
        // Stage each provided field; net::loop() applies it on the loop core.
        if (doc["unit"].is<const char*>()) {
          s_pUnitVal = (String((const char*)doc["unit"]) == "F"); s_pUnit = true;
        }
        if (doc["muted"].is<bool>()) { s_pMuteVal = doc["muted"]; s_pMute = true; }
        if (doc["brightness"].is<int>()) {
          int b = doc["brightness"]; s_pBrightVal = (uint8_t)(b < 0 ? 0 : b > 2 ? 2 : b);
          s_pBright = true;
        }
        if (doc["tz"].is<int>()) {
          s_pTzVal = (uint8_t)tz_clamp(doc["tz"]); s_pTz = true;
        }
        r->send(200, "application/json", "{\"ok\":true}");
      });

  s_ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType,
                  void*, uint8_t*, size_t) {});
  s_server.addHandler(&s_ws);
  ElegantOTA.begin(&s_server);        // browser OTA upload at /update (M10)
  s_server.begin();
  Serial.printf("[net] AP=%s  http://panpilot.local/  (OTA: /update)\n", s_ap);
}

void set_settings_cbs(UnitCb u, SetMuteCb m, SetBrightCb b, SetTzCb t) {
  s_unitCb = u; s_muteCb = m; s_brightCb = b; s_tzCb = t;
}

void loop() {
  if (s_portal) {
    s_wm.process();
    // Portal ends by provisioning success or by its own timeout; track it so
    // the Settings row stops advertising a dead AP.
    if (WiFi.status() == WL_CONNECTED || !s_wm.getConfigPortalActive())
      s_portal = false;
  }
  // (Re)start mDNS on every connect transition — it must bind AFTER the STA
  // has an address (late portal provisioning, router reboots, DHCP renews).
  static bool wasUp = false;
  const bool up = WiFi.status() == WL_CONNECTED;
  if (up && !wasUp) {
    MDNS.end();
    if (MDNS.begin("panpilot")) MDNS.addService("http", "tcp", 80);
    Serial.printf("[net] connected  SSID=%s  IP=%s  http://panpilot.local/\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  }
  wasUp = up;
  s_ws.cleanupClients();
  ElegantOTA.loop();
  // Apply any web settings edits here, on the loop core (safe for NVS/UI).
  if (s_pUnit)   { s_pUnit = false;   if (s_unitCb)   s_unitCb(s_pUnitVal); }
  if (s_pMute)   { s_pMute = false;   if (s_muteCb)   s_muteCb(s_pMuteVal); }
  if (s_pBright) { s_pBright = false; if (s_brightCb) s_brightCb(s_pBrightVal); }
  if (s_pTz)     { s_pTz = false;     if (s_tzCb)     s_tzCb(s_pTzVal); }
  if (s_pFoods)  { s_pFoods = false;  if (s_foodsCb)  s_foodsCb(); }
}

void set_foods_cb(FoodsChangedCb cb) { s_foodsCb = cb; }

bool connected() { return WiFi.status() == WL_CONNECTED; }
bool portal_active() { return s_portal; }
const char* ap_name() { return s_ap; }
String ssid() { return connected() ? WiFi.SSID() : String(); }
String ip() { return connected() ? WiFi.localIP().toString() : String(); }
void start_portal() {
  if (s_portal || WiFi.status() == WL_CONNECTED) return;
  s_wm.setConfigPortalTimeout(180);
  s_wm.startConfigPortal(s_ap);   // non-blocking (configured in begin)
  s_portal = true;
  Serial.printf("[net] config portal reopened - join AP %s\n", s_ap);
}
String mqtt_broker() { return hal::storage_get_mqtt_broker(); }

void publishState(const UiState& s, bool useF) {
  // Cache for the settings API's GET (runs even with no WebSocket clients).
  s_cUseF = useF; s_cMuted = s.muted; s_cBright = s.brightnessLevel;
  s_cTz = s.tzIndex; s_cTimeValid = s.timeValid;
  s_cH = s.clockHour; s_cM = s.clockMin;
  if (s_ws.count() == 0) return;
  const int temp = (int)((useF ? ThermalModel::cToF(s.displayTempC)
                                : s.displayTempC) + 0.5f);
  const int rate = (int)(useF ? s.rateCPerMin * 9.0f / 5.0f : s.rateCPerMin);
  const int tgt = useF ? s.targetCenterF : (int)((s.targetCenterF - 32) * 5 / 9);
  char eta[16];
  if (s.etaSeconds >= 0) snprintf(eta, sizeof(eta), "%d:%02d", s.etaSeconds / 60,
                                  s.etaSeconds % 60);
  else strcpy(eta, "");
  char buf[256];
  snprintf(buf, sizeof(buf),
           "{\"t\":\"s\",\"temp\":%d,\"rate\":%d,\"eta\":\"%s\",\"tgt\":%d,"
           "\"u\":\"%s\",\"preset\":\"%s\",\"g\":\"%s\",\"bar\":\"%s\"}",
           temp, rate, eta, tgt, useF ? "F" : "C", preset(s.presetId).name,
           gname(s.guidance), bartext(s.guidance));
  s_ws.textAll(buf);
}

void publishThermal(const ThermalFrame& f) {
  if (s_ws.count() == 0 || !f.valid) return;
  String out;
  out.reserve(3200);
  out += "{\"t\":\"f\",\"px\":[";
  for (int r = 0; r < THERM_ROWS; ++r)
    for (int c = 0; c < THERM_COLS; ++c) {
      if (r || c) out += ',';
      out += (int)(f.px[r][c] + 0.5f);
    }
  out += "]}";
  s_ws.textAll(out);
}

}  // namespace net
#endif
