// net.cpp — see net.h.
#if !defined(PANPILOT_SIM) && defined(ENABLE_WIFI)
#include "net.h"

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>

#include "web_assets.h"
#include "core/thermal_model.h"
#include "hal/storage.h"

namespace net {
namespace {
WiFiManager s_wm;
AsyncWebServer s_server(80);
AsyncWebSocket s_ws("/ws");
bool s_portal = false;

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
  char ap[20];
  snprintf(ap, sizeof(ap), "PanPilot-%04X",
           (uint16_t)(ESP.getEfuseMac() & 0xFFFF));
  static WiFiManagerParameter mqttParam("mqtt", "MQTT broker (optional)",
                                        hal::storage_get_mqtt_broker().c_str(), 40);
  s_wm.addParameter(&mqttParam);
  s_wm.setConfigPortalBlocking(false);
  s_wm.setConfigPortalTimeout(180);
  s_portal = !s_wm.autoConnect(ap);      // opens captive portal if unprovisioned
  if (strlen(mqttParam.getValue()) > 0)
    hal::storage_set_mqtt_broker(mqttParam.getValue());

  if (MDNS.begin("panpilot")) MDNS.addService("http", "tcp", 80);

  s_server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", PANPILOT_INDEX_HTML);
  });
  s_ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType,
                  void*, uint8_t*, size_t) {});
  s_server.addHandler(&s_ws);
  s_server.begin();
  Serial.printf("[net] AP=%s  http://panpilot.local/\n", ap);
}

void loop() {
  if (s_portal) s_wm.process();
  s_ws.cleanupClients();
}

bool connected() { return WiFi.status() == WL_CONNECTED; }
String mqtt_broker() { return hal::storage_get_mqtt_broker(); }

void publishState(const UiState& s, bool useF) {
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
