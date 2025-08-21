//
// Created by hwyz_leo on 2025/8/13.
//

#ifndef RSMSAPP_MQTT_MCU_HANDLER_H
#define RSMSAPP_MQTT_MCU_HANDLER_H
#include "mqtt_message_handler.h"

/**
 * 处理MCU来的MQTT消息
 */
class MqttMcuHandler : public MqttMessageHandler {
public:
    /**
     * 析构虚函数
     */
    ~MqttMcuHandler() override = default;

    /**
     * 防止对象被复制
     */
    MqttMcuHandler(const MqttMcuHandler &) = delete;

    /**
     * 防止对象被赋值
     * @return
     */
    MqttMcuHandler &operator=(const MqttMcuHandler &) = delete;

    /**
     * 获取单例
     * @return 单例
     */
    static MqttMcuHandler &get_instance();
public:
    /**
     * 处理MCU消息
     * @param payload 数据
     */
    void handle(std::string payload) override;
private:
    MqttMcuHandler() = default;
};
#endif //RSMSAPP_MQTT_MCU_HANDLER_H
