#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "relay_controller.h"
#include "constants.h"

const int NUM_RELAYS = 4;
const int GPIO_PINS[NUM_RELAYS] = {2, 26, 27, 25};
RelayController* RelayController::s_instance = nullptr;

RelayController& RelayController::get() {
    if (s_instance == nullptr) {
        s_instance = new RelayController();
    }
    return *s_instance;
}

RelayController::RelayController() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        Serial.printf("Setting up relay[%d] at pin %d\n", i, GPIO_PINS[i]);
        pinMode(GPIO_PINS[i], OUTPUT);
        digitalWrite(GPIO_PINS[i], HIGH);
    }
}

RelayController::ResultType RelayController::setState(int slot, bool state) {
    if (slot >= NUM_RELAYS) {
        return ResultType::RelayIndexExceeded;
    }
    digitalWrite(GPIO_PINS[slot], state ? LOW : HIGH);
    return ResultType::Ok;
}

RelayController::ResultType RelayController::getState(int slot,
                                                      bool& stateOut) const {
    if (slot >= NUM_RELAYS) {
        return ResultType::RelayIndexExceeded;
    }
    stateOut = digitalRead(GPIO_PINS[slot]) != HIGH;
    return ResultType::Ok;
}

String RelayController::getStateAsJSON() const {
    auto output = String("[");
    for (auto slot = 0; slot < NUM_RELAYS; ++slot) {
        output = output
            + (digitalRead(GPIO_PINS[slot]) == HIGH ? "false" : "true");
        if (slot < NUM_RELAYS - 1) {
            output += ",";
        }
    }
    output += "]";
    return output;
}

int RelayController::numRelays() const {
    return NUM_RELAYS;
}

void RelayController::send(AsyncWebServerRequest* req,
                           ResultType res,
                           const String& data,
                           ContentType contentType) {

    switch (res) {
    case ResultType::Ok: {
        auto ct = String("text/plain");
        if (contentType == ContentType::JSON) {
            ct = "application/json";
        }
        req->send(toInt(HttpStatus::OK), ct, data == "" ? "SUCCESS" : data);
        break;
    }
    case ResultType::RelayIndexExceeded:
        req->send(toInt(HttpStatus::BadRequest),
                  "text/plain",
                  "ERROR:RelayIndexExceeded");
        break;
    default:
        req->send(toInt(HttpStatus::InternalServerError),
                  "text/plain",
                  "ERROR:InternalServerError");
        break;
    }
}

void handleSetStateRequest(AsyncWebServerRequest* req) {
    if (!req->hasParam("slot") || !req->hasParam("state")) {
        req->send(400, "application/json", "ERROR:PARAM_NOT_FOUND");
    }

    auto slotParam = req->getParam("slot");
    auto stateParam = req->getParam("state");
    if (slotParam == nullptr || stateParam == nullptr) {
        req->send(400, "text/plain", "ERROR:INVALID_PARAM");
        return;
    }

    auto slot = static_cast<int>(slotParam->value().toInt());
    auto stateCopy = stateParam->value();
    stateCopy.toUpperCase();
    auto state = static_cast<bool>(stateCopy == "TRUE");

    auto res = RelayController::get().setState(slot, state);
    RelayController::get().send(req, res);
}

void handleGetStateRequest(AsyncWebServerRequest* req) {
    auto json = RelayController::get().getStateAsJSON();
    RelayController::get().send(
        req, RelayController::ResultType::Ok, json, ContentType::JSON);
}

void handleRelayNumRequest(AsyncWebServerRequest* req) {
    RelayController::get().send(
        req, RelayController::ResultType::Ok, String(NUM_RELAYS));
}

void RelayController::addHandlers(AsyncWebServer& server) {
    server.on("/set", HTTP_POST, &handleSetStateRequest);
    server.on("/state", HTTP_GET, &handleGetStateRequest);
    server.on("/numRelays", HTTP_GET, &handleRelayNumRequest);
}
