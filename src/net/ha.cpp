// ha.cpp — see ha.h.
#if !defined(PANPILOT_SIM) && defined(ENABLE_WIFI)
#include "ha.h"

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "core/thermal_model.h"
#include "core/presets.h"

namespace ha {
namespace {
WiFiClient s_tcp;
PubSubClient s_mqtt(s_tcp);
String s_broker;
uint16_t s_port = 1883;
MuteCb s_muteCb = nullptr;
TargetCb s_targetCb = nullptr;
PresetCb s_presetCb = nullptr;
uint32_t s_lastTry = 0;

const char* STATE = "panpilot/state";
const char* AVTY = "panpilot/status";
const char* ALERT = "panpilot/attention";
const char* MUTE_SET = "panpilot/mute/set";
const char* TGT_SET = "panpilot/target/set";
const char* PRESET_SET = "panpilot/preset/set";

const char* gname(GuidanceState g) {
  switch (g) {
    case GuidanceState::HEAT_MORE: return "Heat more";
    case GuidanceState::HOLD: return "Hold";
    case GuidanceState::TURN_DOWN_SOON: return "Turn down soon";
    case GuidanceState::TURN_DOWN_NOW: return "Turn down now";
    case GuidanceState::READY: return "Ready";
    case GuidanceState::TOO_HOT: return "Too hot";
    case GuidanceState::COOLING: return "Cooling";
    case GuidanceState::RECOVERING: return "Recovering";
    case GuidanceState::CHECK_AIM: return "Check aim";
    case GuidanceState::NO_PAN: return "No pan";
    default: return "Idle";
  }
}

// One retained discovery config. `dev` fragment ties entities to one HA device.
void disco(const char* comp, const char* obj, const String& cfg) {
  String topic = String("homeassistant/") + comp + "/panpilot/" + obj + "/config";
  s_mqtt.publish(topic.c_str(), cfg.c_str(), true);
}

const char* DEV =
    "\"dev\":{\"ids\":[\"panpilot\"],\"name\":\"PanPilot\",\"mf\":\"PanPilot\"}";

void publishDiscovery() {
  auto sensor = [&](const char* obj, const char* name, const char* tmpl,
                    const char* unit) {
    String c = String("{\"name\":\"") + name + "\",\"stat_t\":\"" + STATE +
               "\",\"avty_t\":\"" + AVTY + "\",\"uniq_id\":\"pp_" + obj +
               "\",\"val_tpl\":\"" + tmpl + "\"";
    if (unit) c += String(",\"unit_of_meas\":\"") + unit + "\"";
    c += String(",") + DEV + "}";
    disco("sensor", obj, c);
  };
  sensor("temp", "Pan temperature", "{{ value_json.temp }}", "°");
  sensor("rate", "Rate", "{{ value_json.rate }}", "°/min");
  sensor("guid", "Guidance", "{{ value_json.g }}", nullptr);

  // Attention mirror (roadmap §3.5): the current cue text, on its own topic so
  // an HA automation can fire the moment PanPilot escalates.
  disco("sensor", "alert",
        String("{\"name\":\"Alert\",\"stat_t\":\"") + ALERT + "\",\"avty_t\":\"" +
            AVTY + "\",\"uniq_id\":\"pp_alert\","
            "\"val_tpl\":\"{{ value_json.cue }}\"," + DEV + "}");

  disco("binary_sensor", "pan",
        String("{\"name\":\"Pan present\",\"stat_t\":\"") + STATE +
            "\",\"avty_t\":\"" + AVTY +
            "\",\"uniq_id\":\"pp_pan\",\"val_tpl\":\"{{ value_json.pan }}\","
            "\"pl_on\":\"1\",\"pl_off\":\"0\"," + DEV + "}");
  disco("switch", "mute",
        String("{\"name\":\"Mute\",\"stat_t\":\"") + STATE + "\",\"avty_t\":\"" +
            AVTY + "\",\"cmd_t\":\"" + MUTE_SET +
            "\",\"uniq_id\":\"pp_mute\",\"val_tpl\":\"{{ value_json.mute }}\","
            "\"pl_on\":\"1\",\"pl_off\":\"0\"," + DEV + "}");
  disco("number", "target",
        String("{\"name\":\"Target\",\"stat_t\":\"") + STATE + "\",\"avty_t\":\"" +
            AVTY + "\",\"cmd_t\":\"" + TGT_SET +
            "\",\"uniq_id\":\"pp_tgt\",\"val_tpl\":\"{{ value_json.tgt }}\","
            "\"min\":100,\"max\":600,\"step\":5," + DEV + "}");
  disco("select", "preset",
        String("{\"name\":\"Preset\",\"stat_t\":\"") + STATE + "\",\"avty_t\":\"" +
            AVTY + "\",\"cmd_t\":\"" + PRESET_SET +
            "\",\"uniq_id\":\"pp_preset\",\"val_tpl\":\"{{ value_json.preset }}\","
            "\"options\":[\"Eggs\",\"Pancakes\",\"Sear\","
            "\"Tortillas\",\"Generic\"]," + DEV + "}");
}

// SSR-box liveness (roadmap §3.2 / M14.5): the box publishes a retained
// birth/LWT on this topic; arming is gated on having seen "online".
const char* PLUG_STATUS = "panpilot/ssr/status";
volatile bool s_plugOnline = false;

void onMsg(char* topic, byte* payload, unsigned int len) {
  String t(topic), p;
  for (unsigned i = 0; i < len; ++i) p += (char)payload[i];
  if (t == MUTE_SET && s_muteCb) s_muteCb(p == "1" || p == "ON");
  else if (t == TGT_SET && s_targetCb) s_targetCb(p.toInt());
  else if (t == PRESET_SET && s_presetCb) {
    for (uint8_t i = 0; i < PRESET_COUNT; ++i)
      if (p == preset(i).name) { s_presetCb(i); break; }
  } else if (t == PLUG_STATUS) {
    s_plugOnline = (p == "online");
  }
}

void reconnect() {
  if (s_broker.isEmpty() || s_mqtt.connected()) return;
  // No Wi-Fi link -> a TCP connect would sit in its socket timeout for
  // seconds, ON THE LOOP CORE, freezing LVGL every 5 s retry. Skip until up.
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - s_lastTry < 5000) return;
  s_lastTry = millis();
  s_plugOnline = false;   // stale until the retained box status arrives
  if (s_mqtt.connect("panpilot", nullptr, nullptr, AVTY, 0, true, "offline")) {
    s_mqtt.publish(AVTY, "online", true);
    s_mqtt.subscribe(MUTE_SET);
    s_mqtt.subscribe(TGT_SET);
    s_mqtt.subscribe(PRESET_SET);
    s_mqtt.subscribe(PLUG_STATUS);
    publishDiscovery();
    Serial.println("[ha] MQTT connected, discovery published");
  }
}
}  // namespace

void begin(const char* broker, uint16_t port, MuteCb m, TargetCb t, PresetCb p) {
  s_broker = broker ? broker : "";
  s_port = port ? port : 1883;
  s_muteCb = m; s_targetCb = t; s_presetCb = p;
  if (s_broker.isEmpty()) return;
  s_mqtt.setBufferSize(768);
  s_mqtt.setServer(s_broker.c_str(), s_port);
  s_mqtt.setCallback(onMsg);
}

bool enabled() { return !s_broker.isEmpty(); }

void loop() {
  if (s_broker.isEmpty()) return;
  if (!s_mqtt.connected()) { s_plugOnline = false; reconnect(); }
  s_mqtt.loop();
}

// Box liveness for the arming gate + interlock S7: broker link up AND the box's
// retained status says online (its LWT flips this to offline if it dies).
bool plug_online() { return s_mqtt.connected() && s_plugOnline; }

void publish(const UiState& s, bool useF) {
  if (!s_mqtt.connected()) return;
  const int temp = (int)((useF ? ThermalModel::cToF(s.displayTempC)
                                : s.displayTempC) + 0.5f);
  const int rate = (int)(useF ? s.rateCPerMin * 9.0f / 5.0f : s.rateCPerMin);
  const int pan = s.presence == PanPresence::PRESENT ? 1 : 0;
  char buf[256];
  snprintf(buf, sizeof(buf),
           "{\"temp\":%d,\"rate\":%d,\"g\":\"%s\",\"pan\":%d,\"mute\":%d,"
           "\"tgt\":%d,\"preset\":\"%s\"}",
           temp, rate, gname(s.guidance), pan, s.muted ? 1 : 0,
           s.targetCenterF, preset(s.presetId).name);
  s_mqtt.publish(STATE, buf, true);
}

bool connected() { return s_mqtt.connected(); }

// Attention mirror (roadmap §3.5). Retained so HA shows the last cue; level is
// 0..3 (passive/notify/act/alarm).
void publishAttention(int level, const char* verb, const char* sub) {
  if (!s_mqtt.connected()) return;
  char buf[160];
  snprintf(buf, sizeof(buf), "{\"level\":%d,\"cue\":\"%s\",\"sub\":\"%s\"}",
           level, verb ? verb : "", sub ? sub : "");
  s_mqtt.publish(ALERT, buf, true);
}

// ASSIST duty output (roadmap §3.2). Not retained — a stale duty must never
// re-energize a plug after a reconnect; the box's own watchdog fails safe.
void actuator_publish(const char* topic, const char* payload) {
  if (s_mqtt.connected()) s_mqtt.publish(topic, payload, false);
}

}  // namespace ha
#endif
