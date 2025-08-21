//
// Created by hwyz_leo on 2025/8/13.
//

#ifndef RSMSAPP_MQTT_MESSAGE_HANDLER_H
#define RSMSAPP_MQTT_MESSAGE_HANDLER_H
class MqttMessageHandler {
public:
    /**
     * 处理MQTT消息
     * @param payload 数据
     */
    virtual void handle(std::string payload) = 0;

    virtual ~MqttMessageHandler() = default;
};

#endif //RSMSAPP_MQTT_MESSAGE_HANDLER_H
