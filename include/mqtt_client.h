//
// Created by hwyz_leo on 2025/8/4.
//

#ifndef RSMSAPP_MQTT_CLIENT_H
#define RSMSAPP_MQTT_CLIENT_H

#include <thread>

#include "mosquitto/mosquitto.h"
#include "mosquitto/mosquittopp.h"
#include "yaml-cpp/yaml.h"

#include "mqtt_message_handler.h"

/**
 * MQTT客户端
 */
class MqttClient : public mosqpp::mosquittopp {
public:
    /**
     * 析构虚函数
     */
    ~MqttClient() override;

    /**
     * 防止对象被复制
     */
    MqttClient(const MqttClient &) = delete;

    /**
     * 防止对象被赋值
     * @return
     */
    MqttClient &operator=(const MqttClient &) = delete;

    /**
     * 获取单例
     * @return 单例
     */
    static MqttClient &get_instance();

public:
    /**
     * 加载配置
     * @param config 配置信息
     * @return 是否加载成功
     */
    bool load_config(const YAML::Node &config);

    /**
     * 启动
     * @return 启动是否成功
     */
    bool start();

    /**
     * 停止
     */
    void stop();

    /**
     * 是否连接
     * @return 是否连接成功
     */
    bool is_connected() const;

    /**
     * 发布
     * @param mid 消息ID
     * @param topic 主题
     * @param payload 数据
     * @param payload_len 数据长度
     * @param qos 消息质量
     * @return 是否发布成功
     */
    bool publish(int &mid, const std::string &topic, const void *payload = nullptr, int payload_len = 0, int qos = 1);

    void on_connect(int rc) override;

    void on_disconnect(int rc) override;

    void on_publish(int mid) override;

    void on_message(const struct mosquitto_message *message) override;

    void on_subscribe(int mid, int qos_count, const int *granted_qos) override;

    void on_unsubscribe(int mid) override;

    void on_log(int level, const char *str) override;

    void on_error() override;

private:
    // 是否初始化
    std::atomic_bool is_inited_{false};
    // 是否启动
    std::atomic_bool is_started_{false};
    // 是否连接
    std::atomic_bool is_connected_{false};
    // 是否连接中
    std::atomic_bool is_connecting_{false};
    // 是否订阅
    std::atomic_bool is_subscribed_{false};
    // 连接器
    std::thread connector;
    // 轮询锁
    std::mutex mtx_loop_;
    // 轮询条件
    std::condition_variable cv_loop_;
    // 服务器地址
    std::string server_host_ = "127.0.0.1";
    // 服务器端口
    std::uint16_t server_port_ = 1884;
    // 保持连接时间
    int keepalive_ = 60;
    // 客户端ID
    std::string client_id_ = "RsmsApp";
    // 用户名
    std::string username_ = "RsmsApp";
    // 密码
    std::string password_ = "RsmsApp";
    // 使用SSL
    bool use_ssl_ = false;
    // 消息处理器
    std::unordered_map<std::string, MqttMessageHandler*> message_handler_;
    // 主题前缀
    std::string topic_prefix_ = "RSMS/";
    // 重连间隔时间
    int reconnect_interval_second_ = 15;
    // 轮询间隔时间
    int loop_interval_milli_second_ = 100;

private:

    MqttClient();

    /**
     * 初始化
     * @return 初始化是否成功
     */
    bool init();

    /**
     * 连接管理
     */
    void connect_manage();

    /**
     * 连接
     * @return 是否连接成功
     */
    bool connect();

    /**
     * 订阅主题
     * @param mid 消息ID
     * @param topic 主题
     * @param handler 处理器
     * @param qos 消息质量
     * @return 是否订阅成功
     */
    bool subscribe_topic(int &mid, const std::string &topic, MqttMessageHandler &handler, int qos = 1);

};

#endif //RSMSAPP_MQTT_CLIENT_H