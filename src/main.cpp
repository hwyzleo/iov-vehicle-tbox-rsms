//
// Created by hwyz_leo on 2025/8/4.
//
#include "application.h"
#include "spdlog/spdlog.h"

#include "mqtt_client.h"
#include "rsms_signal_cache.h"
#include "rsms_client.h"

class MainApplication : public hwyz::Application {
protected:
    bool initialize() override {
        if (!MqttClient::get_instance().load_config(getConfig())) {
            return false;
        }
        return true;
    }

    void cleanup() override {
        MqttClient::get_instance().stop();
        RsmsSignalCache::get_instance().stop();
        RsmsClient::get_instance().stop();
    }

    int execute() override {
        MqttClient::get_instance().start();
        RsmsSignalCache::get_instance().start();
        RsmsClient::get_instance().start();
        spdlog::info("主函数运行");
        return 0;
    }
};

APPLICATION_ENTRY(MainApplication)