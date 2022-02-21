#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include "relay_controller.h"
#include "constants.h"

const int MAX_RELAYS = 32;
const int NUM_RELAYS = 4;
const int GPIO_PINS[NUM_RELAYS] = {2, 25, 26, 27};
const uint32_t DEFAULT_STATE = 0;

RelayController* RelayController::s_instance = nullptr;

Preferences prefs;

bool getDefaultState(int slot);

RelayController& RelayController::get() {
    if (s_instance == nullptr) {
        prefs.begin("powerctl");
        s_instance = new RelayController();
    }
    return *s_instance;
}

RelayController::RelayController() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        Serial.printf("Setting up relay[%d] at pin %d\n", i, GPIO_PINS[i]);
        pinMode(GPIO_PINS[i], OUTPUT);
        digitalWrite(GPIO_PINS[i], getDefaultState(i) ? LOW : HIGH);
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

String getJSONArray(std::function<const char*(int)> getter) {
    auto output = String("[");
    for (auto slot = 0; slot < NUM_RELAYS; ++slot) {
        output += getter(slot);
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

///////////////////////////////////////////////////////////////////////////////

struct SlotState {
    int slot;
    bool state;
    bool error;
};

bool getDefaultState(int slot) {
    auto states = prefs.getUInt("relayState", DEFAULT_STATE);
    auto state = ((states >> slot) & 0x01) != 0;
    Serial.printf("%d %08x %d\n", slot, states, state);
    return state;
}

bool setDefaultState(uint32_t slot, bool state) {
    auto states = prefs.getUInt("relayState", DEFAULT_STATE);
    auto iState = uint32_t(state ? 1 : 0);
    // auto newState = state ^ ((~iState ^ states) & (1U << slot));
    auto newState = (states & ~(1 << slot)) | (iState << slot);
    Serial.printf("%d:%d %08x -> %08x\n", slot, iState, states, newState);
    return prefs.putUInt("relayState", newState) == sizeof(uint32_t);
}

SlotState getSlotAndState(AsyncWebServerRequest* req) {
    if (!req->hasParam("slot") || !req->hasParam("state")) {
        req->send(400, "application/json", "ERROR:PARAM_NOT_FOUND");
        return SlotState{0, false, true};
    }

    auto slotParam = req->getParam("slot");
    auto stateParam = req->getParam("state");
    if (slotParam == nullptr || stateParam == nullptr) {
        req->send(400, "text/plain", "ERROR:INVALID_PARAM");
        return SlotState{0, false, true};
    }

    auto slot = static_cast<int>(slotParam->value().toInt());
    auto stateCopy = stateParam->value();
    stateCopy.toUpperCase();
    auto state = static_cast<bool>(stateCopy == "TRUE");
    return SlotState{slot, state, false};
}

void handleSetStateRequest(AsyncWebServerRequest* req) {
    auto sst = getSlotAndState(req);
    if (sst.error) {
        return;
    }
    auto res = RelayController::get().setState(sst.slot, sst.state);
    RelayController::get().send(req, res);
}

void handleSetDefaultState(AsyncWebServerRequest* req) {
    auto sst = getSlotAndState(req);
    if (sst.error) {
        return;
    }

    auto res = setDefaultState(sst.slot, sst.state);
    RelayController::get().send(
        req,
        res ? RelayController::ResultType::Ok
            : RelayController::ResultType::DefaultRelayStateSetFailed);
}

void handleGetStateRequest(AsyncWebServerRequest* req) {
    if (req->hasParam("slot")) {
        auto slotParam = req->getParam("slot");
        auto slot = static_cast<int>(slotParam->value().toInt());
        auto val = digitalRead(GPIO_PINS[slot]) == HIGH ? "[false]" : "[true]";
        RelayController::get().send(req, RelayController::ResultType::Ok, val);
        return;
    }

    auto json = getJSONArray([](int slot) -> const char* {
        return digitalRead(GPIO_PINS[slot]) == HIGH ? "false" : "true";
    });
    RelayController::get().send(
        req, RelayController::ResultType::Ok, json, ContentType::JSON);
}

void handleGetDefaultState(AsyncWebServerRequest* req) {
    if (req->hasParam("slot")) {
        auto slotParam = req->getParam("slot");
        auto slot = static_cast<int>(slotParam->value().toInt());
        auto val = getDefaultState(slot) ? "[true]" : "[false]";
        RelayController::get().send(req, RelayController::ResultType::Ok, val);
        return;
    }

    auto json = getJSONArray([](int slot) -> const char* {
        return getDefaultState(slot) ? "true" : "false";
    });
    RelayController::get().send(
        req, RelayController::ResultType::Ok, json, ContentType::JSON);
}

void handleSetAll(AsyncWebServerRequest* req) {
    bool toDefaultState = true;
    auto stateParam = req->getParam("state");
    auto state = false;
    if (req->hasParam("state") && stateParam != nullptr) {
        auto stateCopy = stateParam->value();
        stateCopy.toUpperCase();
        state = static_cast<bool>(stateCopy == "TRUE");
        toDefaultState = false;
    }

    for (auto i = 0; i < NUM_RELAYS; ++i) {
        if (toDefaultState) {
            RelayController::get().setState(i, getDefaultState(i));
        }
        RelayController::get().setState(i, state);
    }
}

void handleRelayNumRequest(AsyncWebServerRequest* req) {
    RelayController::get().send(
        req, RelayController::ResultType::Ok, String(NUM_RELAYS));
}

void RelayController::addHandlers(AsyncWebServer& server) {
    server.on("/set", HTTP_POST, &handleSetStateRequest);
    server.on("/setDefault", HTTP_POST, &handleSetDefaultState);
    server.on("/setAll", HTTP_POST, &handleSetAll);

    server.on("/state", HTTP_GET, &handleGetStateRequest);
    server.on("/stateDefault", HTTP_GET, &handleGetDefaultState);
    server.on("/numRelays", HTTP_GET, &handleRelayNumRequest);
    server.on("/rawPrefs", HTTP_GET, [](AsyncWebServerRequest* req) {
        auto val = prefs.getUInt("relayState", 0xFFFFFFFF);
        req->send(toInt(HttpStatus::OK), "text/plain", String(val, HEX));
    });
    server.on("/resetPrefs", HTTP_POST, [](AsyncWebServerRequest* req) {
        prefs.putUInt("relayState", 0U);
        req->send(toInt(HttpStatus::OK), "text/plain", "SUCCESS");
    });
}
