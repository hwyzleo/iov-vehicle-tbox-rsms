//
// Created by hwyz_leo on 2025/8/6.
//

#ifndef RSMSAPP_RSMS_SIGNAL_CACHE_H
#define RSMSAPP_RSMS_SIGNAL_CACHE_H

#include <string>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>

// 信号类型
enum signal_t {
    SIGNAL_VEHICLE_STATE = 101, // 车辆状态
    SIGNAL_CHARGING_STATE = 102, // 充电状态
    SIGNAL_RUNNING_MODE = 103, // 运行模式
    SIGNAL_SPEED = 104, // 车速
    SIGNAL_TOTAL_ODOMETER = 105, // 累计里程
    SIGNAL_TOTAL_VOLTAGE = 106, // 总电压
    SIGNAL_TOTAL_CURRENT = 107, // 总电流
    SIGNAL_SOC = 108, // SOC
    SIGNAL_DCDC_STATE = 109, // DC/DC状态
    SIGNAL_DRIVING = 110, // 有驱动力
    SIGNAL_BRAKING = 111, // 制动动力
    SIGNAL_GEAR = 112, // 档位
    SIGNAL_INSULATION_RESISTANCE = 113, // 绝缘电阻
    SIGNAL_ACCELERATOR_PEDAL_POSITION = 114, // 加速踏板行程值
    SIGNAL_BRAKE_PEDAL_POSITION = 115, // 制动踏板状态
    SIGNAL_DM1_STATE = 201, // 驱动电机1状态
    SIGNAL_DM1_CONTROLLER_TEMPERATURE = 202, // 驱动电机1控制器温度
    SIGNAL_DM1_SPEED = 203, // 驱动电机1转速
    SIGNAL_DM1_TORQUE = 204, // 驱动电机1转矩
    SIGNAL_DM1_TEMPERATURE = 205, // 驱动电机1温度
    SIGNAL_DM1_CONTROLLER_INPUT_VOLTAGE = 206, // 驱动电机1控制器输入电压
    SIGNAL_DM1_CONTROLLER_DC_BUS_CURRENT = 207, // 驱动电机1控制器直流母线电流
    SIGNAL_DM2_STATE = 221, // 驱动电机2状态
    SIGNAL_DM2_CONTROLLER_TEMPERATURE = 222, // 驱动电机2控制器温度
    SIGNAL_DM2_SPEED = 223, // 驱动电机2转速
    SIGNAL_DM2_TORQUE = 224, // 驱动电机2转矩
    SIGNAL_DM2_TEMPERATURE = 225, // 驱动电机2温度
    SIGNAL_DM2_CONTROLLER_INPUT_VOLTAGE = 226, // 驱动电机2控制器输入电压
    SIGNAL_DM2_CONTROLLER_DC_BUS_CURRENT = 227, // 驱动电机2控制器直流母线电流
    SIGNAL_ENGINE_STATE = 301, // 发动机状态
    SIGNAL_ENGINE_CRANKSHAFT_SPEED = 302, // 发动机曲轴转速
    SIGNAL_ENGINE_CONSUMPTION_RATE = 303, // 发动机燃料消耗率
    SIGNAL_POSITION_VALID = 401, // 定位是否有效
    SIGNAL_SOUTH_LATITUDE = 402, // 纬度是否南纬
    SIGNAL_WEST_LONGITUDE = 403, // 经度是否西经
    SIGNAL_LONGITUDE = 404, // 经度
    SIGNAL_LATITUDE = 405, // 纬度
    SIGNAL_MAX_VOLTAGE_BATTERY_DEVICE_NO = 501, // 最高电压电池子系统号
    SIGNAL_MAX_VOLTAGE_CELL_NO = 502, // 最高电压电池单体代号
    SIGNAL_CELL_MAX_VOLTAGE = 503, // 电池单体电压最高值
    SIGNAL_MIN_VOLTAGE_BATTERY_DEVICE_NO = 504, // 最低电压电池子系统号
    SIGNAL_MIN_VOLTAGE_CELL_NO = 505, // 最低电压电池单体代号
    SIGNAL_CELL_MIN_VOLTAGE = 506, // 电池单体电压最低值
    SIGNAL_MAX_TEMPERATURE_DEVICE_NO = 507, // 最高温度子系统号
    SIGNAL_MAX_TEMPERATURE_PROBE_NO = 508, // 最高温度探针序号
    SIGNAL_MAX_TEMPERATURE = 509, // 最高温度值
    SIGNAL_MIN_TEMPERATURE_DEVICE_NO = 510, // 最低温度子系统号
    SIGNAL_MIN_TEMPERATURE_PROBE_NO = 511, // 最低温度探针序号
    SIGNAL_MIN_TEMPERATURE = 512, // 最低温度值
    SIGNAL_MAX_ALARM_LEVEL = 601, // 最高报警等级
    SIGNAL_ALARM_FLAG = 602, // 通用报警标志
    SIGNAL_BATTERY_FAULT_COUNT = 620, // 可充电储能装置故障总数
    SIGNAL_DRIVE_MOTOR_FAULT_COUNT = 640, // 驱动电机故障总数
    SIGNAL_ENGINE_FAULT_COUNT = 660, // 发动机故障总数
    SIGNAL_OTHER_FAULT_COUNT = 680, // 其他故障总数
    SIGNAL_BATTERY1_VOLTAGE = 701, // 电池子系统1电压
    SIGNAL_BATTERY1_CURRENT = 702, // 电池子系统1电流
    SIGNAL_BATTERY1_CELL_COUNT = 703, // 电池子系统1单体电池总数
    SIGNAL_BATTERY1_CELL1_VOLTAGE = 704, // 电池子系统1单体电池1电压
    SIGNAL_BATTERY1_CELL2_VOLTAGE = 705, // 电池子系统1单体电池2电压
    SIGNAL_BATTERY1_CELL3_VOLTAGE = 706, // 电池子系统1单体电池3电压
    SIGNAL_BATTERY1_CELL4_VOLTAGE = 707, // 电池子系统1单体电池4电压
    SIGNAL_BATTERY1_CELL5_VOLTAGE = 708, // 电池子系统1单体电池5电压
    SIGNAL_BATTERY1_CELL6_VOLTAGE = 709, // 电池子系统1单体电池6电压
    SIGNAL_BATTERY1_CELL7_VOLTAGE = 710, // 电池子系统1单体电池7电压
    SIGNAL_BATTERY1_CELL8_VOLTAGE = 711, // 电池子系统1单体电池8电压
    SIGNAL_BATTERY1_CELL9_VOLTAGE = 712, // 电池子系统1单体电池9电压
    SIGNAL_BATTERY1_CELL10_VOLTAGE = 713, // 电池子系统1单体电池10电压
    SIGNAL_BATTERY1_CELL11_VOLTAGE = 714, // 电池子系统1单体电池11电压
    SIGNAL_BATTERY1_CELL12_VOLTAGE = 715, // 电池子系统1单体电池12电压
    SIGNAL_BATTERY1_CELL13_VOLTAGE = 716, // 电池子系统1单体电池13电压
    SIGNAL_BATTERY1_CELL14_VOLTAGE = 717, // 电池子系统1单体电池14电压
    SIGNAL_BATTERY1_CELL15_VOLTAGE = 718, // 电池子系统1单体电池15电压
    SIGNAL_BATTERY1_CELL16_VOLTAGE = 719, // 电池子系统1单体电池16电压
    SIGNAL_BATTERY1_PROBE_COUNT = 800, // 电池子系统1温度探针个数
    SIGNAL_BATTERY1_PROBE1_TEMPERATURE = 801, // 电池子系统1温度探针1温度
    SIGNAL_BATTERY1_PROBE2_TEMPERATURE = 802, // 电池子系统1温度探针2温度
    SIGNAL_BATTERY1_PROBE3_TEMPERATURE = 803, // 电池子系统1温度探针3温度
    SIGNAL_BATTERY1_PROBE4_TEMPERATURE = 804, // 电池子系统1温度探针4温度
    SIGNAL_BATTERY1_PROBE5_TEMPERATURE = 805, // 电池子系统1温度探针5温度
    SIGNAL_BATTERY1_PROBE6_TEMPERATURE = 806, // 电池子系统1温度探针6温度
    SIGNAL_BATTERY1_PROBE7_TEMPERATURE = 807, // 电池子系统1温度探针7温度
    SIGNAL_BATTERY1_PROBE8_TEMPERATURE = 808, // 电池子系统1温度探针8温度
    SIGNAL_BATTERY1_PROBE9_TEMPERATURE = 809, // 电池子系统1温度探针9温度
    SIGNAL_BATTERY1_PROBE10_TEMPERATURE = 810, // 电池子系统1温度探针10温度
    SIGNAL_BATTERY1_PROBE11_TEMPERATURE = 811, // 电池子系统1温度探针11温度
    SIGNAL_BATTERY1_PROBE12_TEMPERATURE = 812, // 电池子系统1温度探针12温度
    SIGNAL_BATTERY1_PROBE13_TEMPERATURE = 813, // 电池子系统1温度探针13温度
    SIGNAL_BATTERY1_PROBE14_TEMPERATURE = 814, // 电池子系统1温度探针14温度
    SIGNAL_BATTERY1_PROBE15_TEMPERATURE = 815, // 电池子系统1温度探针15温度
};

/**
 * 国标信号缓存
 */
class RsmsSignalCache {
public:
    /**
     * 析构虚函数
     */
    ~RsmsSignalCache() = default;

    /**
     * 防止对象被复制
     */
    RsmsSignalCache(const RsmsSignalCache &) = delete;

    /**
     * 防止对象被赋值
     * @return
     */
    RsmsSignalCache &operator=(const RsmsSignalCache &) = delete;

    /**
     * 获取单例
     * @return 单例
     */
    static RsmsSignalCache &get_instance();

public:
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
     * 设置无符号单字节整形
     * @param key 缓存Key
     * @param value 缓存值
     * @return 是否成功
     */
    bool set_byte(const int &key, const uint8_t &value);

    /**
     * 获取无符号单字节整形
     * @param key 缓存Key
     * @param out_value 缓存值
     * @return 是否成功
     */
    bool get_byte(const int &key, uint8_t &out_value);

    /**
     * 设置无符号双字节整形
     * @param key 缓存Key
     * @param value 缓存值
     * @return 是否成功
     */
    bool set_word(const int &key, const uint16_t &value);

    /**
     * 获取无符号双字节整形
     * @param key 缓存Key
     * @param out_value 缓存值
     * @return 是否成功
     */
    bool get_word(const int &key, uint16_t &out_value);

    /**
     * 设置无符号四字节整形
     * @param key 缓存Key
     * @param value 缓存值
     * @return 是否成功
     */
    bool set_dword(const int &key, const uint32_t &value);

    /**
     * 获取无符号四字节整形
     * @param key 缓存Key
     * @param out_value 缓存值
     * @return 是否成功
     */
    bool get_dword(const int &key, uint32_t &out_value);

    /**
     * 设置ASCII字符码
     * @param key 缓存Key
     * @param value 缓存值
     * @return 是否成功
     */
    bool set_string(const int &key, const std::string &value);

    /**
     * 获取ASCII字符码
     * @param key 缓存Key
     * @param out_value 缓存值
     * @return 是否成功
     */
    bool get_string(const int &key, std::string &out_value);

    /**
     * 设置布尔值
     * @param key 缓存Key
     * @param value 缓存值
     * @return 是否成功
     */
    bool set_boolean(const int &key, const bool &value);

    /**
     * 获取布尔值
     * @param key 缓存Key
     * @param out_value 缓存值
     * @return 是否成功
     */
    bool get_boolean(const int &key, bool &out_value);

private:
    // 信号缓存数据
    std::unordered_map<int, std::string> signal_map_;
    // 实例运行状态
    std::atomic<bool> is_running_{false};
    // 写入文件的线程
    std::thread write_thread_;
    // 写入文件的间隔时间
    int write_interval_seconds_ = 5;
    // 缓存文件路径
    std::string cache_file_path_ = "/tmp/gb_signal_cache.dat";

private:
    /**
     * 构造函数
     */
    RsmsSignalCache() = default;

    /**
     * 初始化
     * @return 初始化是否成功
     */
    bool init();

    /**
     * 从本地文件加载数据到信号缓存
     */
    void load_file_data();

    /**
     * 定时写入文件的线程函数
     */
    void write_file_thread();

    /**
     * 将信号缓存数据写入本地文件
     */
    void write_file();

};

#endif //RSMSAPP_RSMS_SIGNAL_CACHE_H
