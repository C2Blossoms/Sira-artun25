#include <HTTPClient.h>
#include <ArduinoJson.h>

inline bool post_json(const String& url, const char* apiKey,
                      DynamicJsonDocument& req, DynamicJsonDocument& resp){
  HTTPClient http; http.begin(url); http.addHeader("Content-Type","application/json");
  if(apiKey && strlen(apiKey)) http.addHeader("X-API-KEY", apiKey);
  String body; serializeJson(req, body);
  int code = http.POST(body); if(code<=0){ http.end(); return false; }
  auto err = deserializeJson(resp, http.getString()); http.end(); return !err;
}

inline void wifi_connect(){
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0=millis(); while(WiFi.status()!=WL_CONNECTED){ delay(400); if(millis()-t0>20000) ESP.restart(); }
}

inline bool api_topup(const String& uid, long amount, long& balance_out){
  DynamicJsonDocument req(256), resp(512);
  req["card_uid"]=uid; req["amount_cents"]=amount;
  if(!post_json(String(API_BASE_URL)+"/topup", API_KEY, req, resp)) return false;
  bool ok = resp["approved"] | false; balance_out = resp["balance_cents"] | -1; return ok;
}

inline bool api_payment(const String& uid, long amount, long& balance_out, String& reason){
  DynamicJsonDocument req(256), resp(512);
  req["card_uid"]=uid; req["amount_cents"]=amount;
  if(!post_json(String(API_BASE_URL)+"/payment", API_KEY, req, resp)) return false;
  bool ok = resp["approved"] | false; balance_out = resp["balance_cents"] | -1;
  if(!ok) reason = resp["reason"] | "ERR";
  return ok;
}
