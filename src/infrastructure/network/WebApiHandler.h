#pragma once
#include <functional>
#include "../../domain/entities/AppState.h"
#include "../storage/HistoryRepository.h"
#include "../storage/PreferencesRepository.h"

class WebServer;

class WebApiHandler {
public:
    using AppStateGetter = std::function<AppState()>;

    WebApiHandler(AppStateGetter getter,
                  HistoryRepository& history,
                  PreferencesRepository& config);
    ~WebApiHandler();

    void begin();
    void handle();

private:
    AppStateGetter getter_;
    HistoryRepository& history_;
    PreferencesRepository& config_;
    WebServer* server_;
};
