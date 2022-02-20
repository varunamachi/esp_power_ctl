#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncElegantOTA.h>

#include "constants.h"
#include "relay_controller.h"

auto server = AsyncWebServer(80);

void setup() {
    Serial.begin(115200);
    WiFi.begin(SSID, PASSPHRASE);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.printf("Attempting connection to %s...\n", SSID);
    }

    if (!MDNS.begin(ESP_MDNS_NAME)) {
        Serial.printf("Failed to start mDNS\n");
        return;
    }

    AsyncElegantOTA.begin(&server);

    Serial.printf("Connected with IP %s\n", WiFi.localIP().toString().c_str());
    RelayController::get().addHandlers(server);
    server.begin();
}

void loop() {
}
