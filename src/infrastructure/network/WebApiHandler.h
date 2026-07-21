#pragma once
#include <functional>
#include "../../domain/entities/AppState.h"
#include "../storage/HistoryRepository.h"
#include "../storage/PreferencesRepository.h"

class AsyncWebServer;

class WebApiHandler {
public:
    using AppStateGetter = std::function<AppState()>;

    WebApiHandler(AppStateGetter getter,
                  HistoryRepository& history,
                  PreferencesRepository& config);
    ~WebApiHandler();

    void begin();

private:
    AppStateGetter getter_;
    HistoryRepository& history_;
    PreferencesRepository& config_;
    AsyncWebServer* server_;
};
