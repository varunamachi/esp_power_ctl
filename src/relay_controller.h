#pragma once

#include <WString.h>
#include <Preferences.h>

#include "constants.h"

class AsyncWebServer;
class AsyncWebServerRequest;

class RelayController {

public:
    enum class ResultType : int {
        Ok = 0x00,
        RelayIndexExceeded = 0x101,
        DefaultRelayStateSetFailed = 0x102,
    };

    ResultType setState(int slot, bool state);
    ResultType getState(int slot, bool& stateOut) const;
    int numRelays() const;
    void addHandlers(AsyncWebServer& server);

    void send(AsyncWebServerRequest* req,
              ResultType res,
              const String& data = "",
              ContentType contentType = ContentType::Text);

    static String getErrorString(ResultType err);
    static RelayController& get();

private:
    RelayController();

    static RelayController* s_instance;
};

extern Preferences prefs;
