//
// Created by hwyz_leo on 2025/8/20.
//
#include "spdlog/spdlog.h"

#include "mqtt_tsp_connect_handler.h"
#include "rsms_client.h"

MqttTspConnectHandler &MqttTspConnectHandler::get_instance() {
    static MqttTspConnectHandler instance;
    return instance;
}

void MqttTspConnectHandler::handle(std::string payload) {
    spdlog::info("收到TSP MQTT已连接[{}]消息", payload);
    RsmsClient::get_instance().set_tsp_login();
}