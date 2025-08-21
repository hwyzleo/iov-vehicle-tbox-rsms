//
// Created by hwyz_leo on 2025/8/13.
//

#ifndef RSMSAPP_MQTT_TSP_CONNECT_HANDLER_H
#define RSMSAPP_MQTT_TSP_CONNECT_HANDLER_H
#include <string>
#include "mqtt_message_handler.h"

/**
 * 处理TSP服务来的MQTT已连接消息
 */
class MqttTspConnectHandler : public MqttMessageHandler {
public:
    /**
     * 析构虚函数
     */
    ~MqttTspConnectHandler() override = default;

    /**
     * 防止对象被复制
     */
    MqttTspConnectHandler(const MqttTspConnectHandler &) = delete;

    /**
     * 防止对象被赋值
     * @return
     */
    MqttTspConnectHandler &operator=(const MqttTspConnectHandler &) = delete;

    /**
     * 获取单例
     * @return 单例
     */
    static MqttTspConnectHandler &get_instance();
public:
    /**
     * 处理MCU消息
     * @param payload 数据
     */
    void handle(std::string payload) override;
private:
    MqttTspConnectHandler() = default;
};
#endif //RSMSAPP_MQTT_TSP_CONNECT_HANDLER_H
