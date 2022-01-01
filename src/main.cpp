#include "ESPAsyncWebServer.h"
#include "WiFi.h"
#include "constants.h"
#include "relay_controller.h"

// const int NUM_RELAYS = 4;
// const int GPIO_PINS[NUM_RELAYS] = {2, 26, 27, 25};
auto server = AsyncWebServer(80);

// void handleSetStateRequest(AsyncWebServerRequest* req) {
//     if (!req->hasParam("slot") || !req->hasParam("state")) {
//         req->send(400, "application/json", "ERROR:PARAM_NOT_FOUND");
//     }

//     auto slotParam = req->getParam("slot");
//     auto stateParam = req->getParam("state");
//     if (slotParam == nullptr || stateParam == nullptr) {
//         req->send(400, "text/plain", "ERROR:INVALID_PARAM");
//         return;
//     }

//     auto slot = static_cast<int>(slotParam->value().toInt());
//     auto stateCopy = stateParam->value();
//     stateCopy.toUpperCase();
//     auto state = static_cast<bool>(stateCopy == "TRUE");

//     if (slot > NUM_RELAYS) {
//         req->send(400, "text/plain", "ERROR:RELAY_INDEX_RANGE_EXCEEDED");
//         return;
//     }
//     digitalWrite(GPIO_PINS[slot], state ? LOW : HIGH);
//     req->send(200, "text/plain", "SUCCESS");
// }

// void handleGetStateRequest(AsyncWebServerRequest* req) {
//     auto getStateString = [](const int slot) -> const char* {
//         if (slot >= NUM_RELAYS) {
//             return "false";
//         }
//         return digitalRead(GPIO_PINS[slot]) == HIGH ? "false" : "true";
//     };

//     auto output = String("[");
//     for (auto slot = 0; slot < NUM_RELAYS; ++slot) {
//         output = output + getStateString(slot);
//         if (slot < NUM_RELAYS - 1) {
//             output += ",";
//         }
//     }
//     output += "]";
//     req->send(200, "text/plain", output);
// }

// void handleRelayNumRequest(AsyncWebServerRequest* req) {
//     req->send(200, "text/plain", String(NUM_RELAYS));
// }

// void setup() {
//     Serial.begin(115200);
//     for (int i = 0; i < NUM_RELAYS; i++) {
//         Serial.printf("Setting up relay[%d] at pin %d\n", i, GPIO_PINS[i]);
//         pinMode(GPIO_PINS[i], OUTPUT);
//         digitalWrite(GPIO_PINS[i], HIGH);
//     }

//     WiFi.begin(SSID, PASSPHRASE);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(1000);
//         Serial.printf("Attempting connection to %s...\n", SSID);
//     }
//     Serial.printf("Connected with IP %s\n",
//     WiFi.localIP().toString().c_str());

//     server.on("/set", HTTP_POST, &handleSetStateRequest);
//     server.on("/state", HTTP_GET, &handleGetStateRequest);
//     server.on("/numRelays", HTTP_GET, &handleRelayNumRequest);
//     server.begin();
// }

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSPHRASE);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.printf("Attempting connection to %s...\n", SSID);
    }
    Serial.printf("Connected with IP %s\n", WiFi.localIP().toString().c_str());
    RelayController::get().addHandlers(server);
    server.begin();
}

void loop() {
}
