//
// Created by hwyz_leo on 2025/8/4.
//
#include <iostream>
#include <regex>

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "utils.h"

#include "mqtt_client.h"
#include "mqtt_mcu_handler.h"
#include "mqtt_tsp_connect_handler.h"

using json = nlohmann::json;

MqttClient::MqttClient() : mosqpp::mosquittopp() {}

MqttClient::~MqttClient() {
    mosqpp::lib_cleanup();
}

MqttClient &MqttClient::get_instance() {
    static MqttClient instance;
    return instance;
}

bool MqttClient::load_config(const YAML::Node &config) {
    spdlog::info("加载MQTT客户端配置信息");
    if (config["mqtt"]) {
        if (config["mqtt"]["host"]) {
            server_host_ = config["mqtt"]["host"].as<std::string>();
        }
        if (config["mqtt"]["port"]) {
            server_port_ = config["mqtt"]["port"].as<std::uint16_t>();
        }
        if (config["mqtt"]["keepalive"]) {
            keepalive_ = config["mqtt"]["keepalive"].as<std::uint16_t>();
        }
        if (config["mqtt"]["use-ssl"]) {
            use_ssl_ = config["mqtt"]["use-ssl"].as<bool>();
        }
        if (config["mqtt"]["reconnect-interval-second"]) {
            reconnect_interval_second_ = config["mqtt"]["reconnect-interval-second"].as<int>();
        }
        if (config["mqtt"]["username"]) {
            username_ = config["mqtt"]["username"].as<std::string>();
        }
        if (config["mqtt"]["password"]) {
            password_ = config["mqtt"]["password"].as<std::string>();
        }
        if (config["mqtt"]["client-id"]) {
            client_id_ = config["mqtt"]["client-id"].as<std::string>();
        }
    }

    return true;
}

bool MqttClient::start() {
    if (!is_started_) {
        spdlog::info("启动MQTT客户端");
        this->connect_manage();
        is_started_ = true;
    }
    return is_started_;
}

void MqttClient::stop() {
    if (!is_started_) {
        return;
    }
    this->disconnect();
    mosqpp::lib_cleanup();
    is_started_ = false;
}

bool MqttClient::is_connected() const {
    return is_connected_;
}

bool MqttClient::publish(int &mid, const std::string &topic, const void *payload, int payload_len, int qos) {
    if (nullptr == payload) {
        return false;
    }
    if (!is_connected_) {
        return false;
    }
    int rc = mosquittopp::publish(&mid, topic.c_str(), payload_len, payload, qos, false);
    std::vector<uint8_t> payload_vector(
            static_cast<const uint8_t *>(payload),
            static_cast<const uint8_t *>(payload) + payload_len
    );
    std::string hex_payload = hwyz::Utils::bytes_to_hex(payload_vector, true);
    spdlog::info("发送[{}]消息至主题[{}]QOS[{}]", mid, topic, qos);
    std::cout << hex_payload << std::endl;
    if (rc == MOSQ_ERR_SUCCESS) {
        cv_loop_.notify_all();
        return true;
    }
    return false;
}

bool MqttClient::publish(int &mid, const std::string &topic, std::vector<uint8_t> *payload, int qos) {
    if (nullptr == payload) {
        return false;
    }
    if (!is_connected_) {
        return false;
    }
    int rc = mosquittopp::publish(&mid, topic.c_str(), static_cast<int>(payload->size()), payload->data(), qos, false);
    std::string hex_payload = hwyz::Utils::bytes_to_hex(*payload, true);
    spdlog::info("发送[{}]消息至主题[{}]QOS[{}]", mid, topic, qos);
    std::cout << hex_payload << std::endl;
    if (rc == MOSQ_ERR_SUCCESS) {
        cv_loop_.notify_all();
        return true;
    }
    return false;
}

void MqttClient::on_connect(int rc) {
    is_connecting_ = false;
    is_connected_ = (rc == MOSQ_ERR_SUCCESS);
    if (is_connected_) {
        spdlog::info("MQTT客户端连接成功");
        int mid = 0;
        subscribe_topic(mid, "GLOBAL/TSP_CONNECT", MqttTspConnectHandler::get_instance(), 1);
        subscribe_topic(mid, topic_prefix_ + "MCU_DATA", MqttMcuHandler::get_instance(), 1);
        is_subscribed_ = true;
    }
}

void MqttClient::on_disconnect(int rc) {
    spdlog::info("MQTT客户端断开[{}]", rc);
    is_connecting_ = false;
    is_connected_ = false;
}

void MqttClient::on_publish(int rc) {
    spdlog::info("发送[{}]消息成功", rc);
}

void MqttClient::on_message(const struct mosquitto_message *message) {
    std::string topic = message->topic;
    spdlog::debug("收到消息主题[{}]内容[{}]", topic, static_cast<char *>(message->payload));
    std::string payload = hwyz::Utils::base64_decode(
            std::string(static_cast<char *>(message->payload), message->payloadlen));
    if (message_handler_[topic]) {
        message_handler_[topic]->handle(payload);
    }
}

void MqttClient::on_subscribe(int mid, int qos_count, const int *granted_qos) {

}

void MqttClient::on_unsubscribe(int mid) {

}

void MqttClient::on_log(int level, const char *str) {

}

void MqttClient::on_error() {
    spdlog::info("MQTT客户端报错");
}

bool MqttClient::init() {
    if (!is_inited_) {
        spdlog::info("初始化MQTT客户端");
        int rc = mosqpp::lib_init();
        if (rc == MOSQ_ERR_SUCCESS) {
            spdlog::info("MQTT客户端初始化成功");
            is_inited_ = true;
        }
    }
    return is_inited_;
}

void MqttClient::connect_manage() {
    std::thread th([&]() {
        bool is_first_connect = true;
        while (is_started_) {
            if (!init()) {
                spdlog::info("MQTT客户端初始化失败");
                std::this_thread::sleep_for(std::chrono::seconds(reconnect_interval_second_));
                continue;
            }
            if (!is_connected_ && !is_connecting_) {
                if (is_first_connect) {
                    is_first_connect = false;
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(reconnect_interval_second_));
                }
                if (connect()) {
                    is_connecting_ = true;
                }
            } else {
                this->loop();
            }
            std::unique_lock<std::mutex> lock(mtx_loop_);
            cv_loop_.wait_for(lock, std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::milliseconds(loop_interval_milli_second_)));
        }
    });
    connector.swap(th);
}

bool MqttClient::connect() {
    spdlog::info("重置客户端ID");
    int rc = this->reinitialise(client_id_.c_str(), true);
    if (rc != MOSQ_ERR_SUCCESS) {
        return false;
    }
    spdlog::info("设置用户名密码");
    rc = this->username_pw_set(username_.c_str(), password_.c_str());
    if (rc != MOSQ_ERR_SUCCESS) {
        return false;
    }
    spdlog::info("连接MQTT[{}:{}]", server_host_, server_port_);
    rc = mosquittopp::connect(server_host_.c_str(), server_port_, keepalive_);
    if (rc != MOSQ_ERR_SUCCESS) {
        spdlog::info("连接MQTT失败");
        return false;
    }
    return true;
}

bool MqttClient::subscribe_topic(int &mid, const std::string &topic, MqttMessageHandler &handler, int qos) {
    if (!is_connected_) {
        return false;
    }
    if (topic.empty()) {
        return false;
    }
    int rc = this->subscribe(&mid, topic.c_str(), qos);
    spdlog::info("订阅[{}]主题[{}]QOS[{}]", mid, topic, qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        spdlog::warn("订阅[{}]主题[{}]失败[{}]", mid, topic, rc);
        return false;
    }
    message_handler_[topic] = &handler;
    cv_loop_.notify_all();
    return true;
}
