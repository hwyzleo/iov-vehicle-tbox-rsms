//
// Created by hwyz_leo on 2025/8/14.
//

#ifndef RSMSAPP_RSMS_CLIENT_H
#define RSMSAPP_RSMS_CLIENT_H
#include <vector>
#include <cstdint>
#include <map>

// 国标命令标识
enum command_flag_t {
    VEHICLE_LOGIN = 0x01, // 车辆登录
    REALTIME_REPORT = 0x02, // 实时信息上报
    REISSUE_REPORT = 0x03, // 补发信息上报
    VEHICLE_LOGOUT = 0x04, // 车辆登出
};

// 应答标志
enum ack_flag_t {
    COMMAND = 0xFE, // 命令
};

// 数据单元加密方式
enum data_unit_encrypt_type_t {
    NONE = 0x01, // 不加密
    RSA = 0x02, // RSA加密
    AES128 = 0x03, // AES128加密
    EXCEPTION = 0xFE, // 异常
    INVALID = 0xFF, // 无效
};

// 数据信息类型
enum data_info_type_t {
    VEHICLE = 0x01, // 整车数据
    DRIVE_MOTOR = 0x02, // 驱动电机数据
    FUEL_CELL = 0x03, // 燃料电池数据
    ENGINE = 0x04, // 发动机数据
    POSITION = 0x05, // 车辆位置数据
    EXTREMUM = 0x06, // 极值数据
    ALARM = 0x07, // 报警数据
    BATTERY_VOLTAGE = 0x08, // 可充电储能装置电压数据
    BATTERY_TEMPERATURE = 0x09, // 可充电储能装置温度数据
};

class RsmsClient {
public:
    /**
     * 析构虚函数
     */
    ~RsmsClient() = default;

    /**
     * 防止对象被复制
     */
    RsmsClient(const RsmsClient &) = delete;

    /**
     * 防止对象被赋值
     * @return
     */
    RsmsClient &operator=(const RsmsClient &) = delete;

    /**
     * 获取单例
     * @return 单例
     */
    static RsmsClient &get_instance();

public:
    /**
     * 启动
     * @return 启动是否成功
     */
    bool start();

    /**
     * 设置TSP登录
     * @return 设置是否成功
     */
    bool set_tsp_login();

    /**
     * 停止
     */
    void stop();

private:
    // 是否初始化
    std::atomic_bool is_init_{false};
    // 是否启动
    std::atomic_bool is_start_{false};
    // TSP服务是否登录
    std::atomic_bool is_tsp_login_{false};
    // 本车是否登录
    std::atomic_bool is_vehicle_login_{false};
    // 登录流水号
    uint16_t login_sn_ = 0;
    // 消息起始符
    std::string starting_symbols_ = "##";
    // 车架号
    std::string vin_;
    // ICCID
    std::string iccid_;
    // 可充电储能子系统数
    uint8_t battery_pack_count_ = 1;
    // 可充电储能子系统编码
    std::string battery_pack_sn_;
    // 配置文件路径
    std::string config_file_path_ = "/tmp/rsms_client.config";
    // 数据文件路径
    std::string data_file_path_ = "/tmp/rsms_message.dat";
    // 实时采集信号的线程
    std::thread collect_thread_;
    // 补发信号的线程
    std::thread reissue_thread_;
    // 补发消息
    std::vector<std::vector<uint8_t>> reissue_messages_;
    // MQTT主题
    std::string mqtt_topic_ = "TSP/RSMS";

private:
    /**
     * 构造函数
     */
    RsmsClient() = default;

    /**
     * 初始化
     * @return 初始化是否成功
     */
    bool init();

    /**
     * 从本地文件加载数据到配置参数
     */
    void load_config();

    /**
     * 从本地文件加载数据
     */
    void load_data();

    /**
     * 保存配置参数到本地文件
     */
    void save_config();

    /**
     * 保存数据到本地文件
     */
    void save_data();

    /**
     * 登录
     * @return 是否成功
     */
    bool login();

    /**
     * 采集信号
     * @return 是否成功
     */
    bool collect_signal();

    /**
     * 登出
     * @return 是否成功
     */
    void logout();

    /**
     * 获取当前时间
     * @return 当前时间（6字节）
     */
    std::vector<uint8_t> get_current_time();

    /**
     * 构造车辆登录数据单元
     * @return 车辆登录报文
     */
    std::vector<uint8_t> build_vehicle_login();

    /**
     * 构造实时信号数据单元
     * @return 实时上报报文
     */
    std::vector<uint8_t> build_realtime_signal();

    /**
     * 构造整车数据信息体
     * @return 整车数据信息体
     */
    std::vector<uint8_t> build_vehicle_data();

    /**
     * 构造驱动电机信息体
     * @return 驱动电机信息体
     */
    std::vector<uint8_t> build_drive_motor();

    /**
     * 构造发动机信息体
     * @return 发动机信息体
     */
    std::vector<uint8_t> build_engine();

    /**
     * 构造车辆位置信息体
     * @return 车辆位置信息体
     */
    std::vector<uint8_t> build_position();

    /**
     * 构造极值数据信息体
     * @return 极值数据信息体
     */
    std::vector<uint8_t> build_extremum();

    /**
     * 构造报警数据信息体
     * @return 报警数据信息体
     */
    std::vector<uint8_t> build_alarm();

    /**
     * 构造可充电储能装置电压数据信息体
     * @return 可充电储能装置电压数据信息体
     */
    std::vector<uint8_t> build_battery_voltage();

    /**
     * 构造可充电储能装置温度数据信息体
     * @return 可充电储能装置温度数据信息体
     */
    std::vector<uint8_t> build_battery_temperature();

    /**
     * 构造车辆登出数据单元
     * @return 车辆登出报文
     */
    std::vector<uint8_t> build_vehicle_logout();

    /**
     * 计算校验码
     * @param data_unit 数据单元
     * @return 校验码
     */
    uint8_t calculate_check_code(std::vector<uint8_t> data_unit);

    /**
     * 构造消息报文
     * @param command_flag 命令标识
     * @param data_unit 数据单元
     * @return 消息报文
     */
    std::vector<uint8_t> build_message(command_flag_t command_flag, const std::vector<uint8_t>& data_unit);

    /**
     * 双字节整形转数组（大端模式）
     * @param value 双字节整形数值
     * @return 数组
     */
    std::vector<uint8_t> word_to_bytes(uint16_t value);

    /**
     * 四字节整形转数组（大端模式）
     * @param value 四字节整形数值
     * @return 数组
     */
    std::vector<uint8_t> dword_to_bytes(uint32_t value);

    /**
     * 字符串转数组
     * @param value 字符串
     * @return 数组
     */
    std::vector<uint8_t> string_to_bytes(std::string value);

    /**
     * 定时采集信号的线程函数
     */
    void collect_thread();

    /**
     * 补发信号的线程函数
     */
    void reissue_thread();
};
#endif //RSMSAPP_RSMS_CLIENT_H
