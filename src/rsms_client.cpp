//
// Created by hwyz_leo on 2025/8/14.
//
#include <fstream>
#include <sstream>
#include <algorithm>

#include "spdlog/spdlog.h"
#include "utils.h"

#include "rsms_client.h"
#include "rsms_signal_cache.h"
#include "mqtt_client.h"

RsmsClient &RsmsClient::get_instance() {
    static RsmsClient instance;
    return instance;
}

bool RsmsClient::init() {
    load_config();
    std::string vin = hwyz::Utils::global_read_string(hwyz::global_key_t::VIN);
    std::string iccid = hwyz::Utils::global_read_string(hwyz::global_key_t::CURRENT_ICCID);
    std::string battery_pack_sn = hwyz::Utils::global_read_string(hwyz::global_key_t::BATTERY_PACK_SN);
    if (vin.empty() || iccid.empty() || battery_pack_sn.empty()) {
        spdlog::error("VIN[{}]ICCID[{}]电池包序列号[{}]为空", vin, iccid, battery_pack_sn);
        return false;
    }
    vin_ = vin;
    iccid_ = iccid;
    battery_pack_sn_ = battery_pack_sn;
    return true;
}

void RsmsClient::load_config() {
    std::ifstream file(config_file_path_, std::ios::binary);
    if (!file.is_open()) {
        spdlog::info("配置文件不存在，使用默认配置");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            if (key == "login_sn") {
                login_sn_ = std::stoi(value);
            }
        }
    }
    file.close();
    spdlog::info("配置文件加载完成");
}

void RsmsClient::load_data() {
    std::ifstream file(data_file_path_, std::ios::binary);
    if (!file.is_open()) {
        spdlog::info("数据文件不存在");
        return;
    }
    reissue_messages_.clear();
    while (file.peek() != EOF) {
        // 读取消息长度
        uint32_t length;
        file.read(reinterpret_cast<char *>(&length), sizeof(length));
        // 检查是否读取成功
        if (file.gcount() != sizeof(length)) {
            break;
        }
        // 创建对应大小的消息缓冲区
        std::vector<uint8_t> message(length);
        // 读取消息内容
        file.read(reinterpret_cast<char *>(message.data()), length);
        // 检查是否读取成功
        if (file.gcount() != static_cast<std::streamsize>(length)) {
            break;
        }
        reissue_messages_.push_back(std::move(message));
    }
    file.close();
    spdlog::info("配置文件加载完成");
}

void RsmsClient::save_config() {
    std::string temp_file = config_file_path_ + ".tmp";
    std::ofstream file(temp_file, std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("无法创建配置文件: {}", temp_file);
        return;
    }
    file << "login_sn=" << login_sn_ << std::endl;
    file.close();

    if (!hwyz::Utils::rename_file(temp_file, config_file_path_)) {
        spdlog::error("重命名配置文件失败");
        std::remove(temp_file.c_str());
    }
    spdlog::info("配置文件保存完成");
}

void RsmsClient::save_data() {
    std::string temp_file = data_file_path_ + ".tmp";
    std::ofstream file(temp_file, std::ios::binary);
    if (!file.is_open()) {
        spdlog::error("无法创建数据文件: {}", temp_file);
        return;
    }
    for (const auto &message: reissue_messages_) {
        uint32_t length = static_cast<uint32_t>(message.size());
        file.write(reinterpret_cast<const char *>(&length), sizeof(length));
        file.write(reinterpret_cast<const char *>(message.data()), message.size());
    }
    file.close();

    if (hwyz::Utils::rename_file(temp_file, data_file_path_) != 0) {
        spdlog::error("重命名数据文件失败");
        std::remove(temp_file.c_str());
    }
    spdlog::info("数据文件保存完成");
}

bool RsmsClient::start() {
    spdlog::info("启动国标客户端实例");
    while (!is_init_) {
        is_init_ = init();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (is_tsp_login_) {
        login();
    }
    is_start_ = true;
    collect_thread_ = std::thread(&RsmsClient::collect_thread, this);
    reissue_thread_ = std::thread(&RsmsClient::reissue_thread, this);
    return true;
}

bool RsmsClient::set_tsp_login() {
    if (!is_tsp_login_) {
        spdlog::info("设置TSP登录");
        is_tsp_login_ = true;
        login();
    }
    return true;
}

void RsmsClient::stop() {
    spdlog::info("停止国标客户端实例");
    logout();
    is_start_ = false;
    is_vehicle_login_ = false;
}

bool RsmsClient::login() {
    spdlog::info("车辆登录");
    std::vector<uint8_t> vehicle_login = build_vehicle_login();
    std::vector<uint8_t> message = build_message(VEHICLE_LOGIN, vehicle_login);
    int mid = 0;
    MqttClient::get_instance().publish(mid, mqtt_topic_, &message, 1);
    is_vehicle_login_ = true;
    save_config();
}

bool RsmsClient::collect_signal() {
    spdlog::debug("采集信号数据");
    std::vector<uint8_t> realtime_signal = build_realtime_signal();
    if (reserve_messages_.size() >= max_reserve_messages_) {
        reserve_messages_.pop_front();
    }
    reserve_messages_.push_back(realtime_signal);
    long long now = hwyz::Utils::get_current_timestamp_sec();
    if (is_alarm3()) {
        if (last_alarm_timestamp_ == 0) {
            spdlog::warn("发生三级报警[{}]", now);
            last_alarm_timestamp_ = now;
            for (const auto &reserved_message: reserve_messages_) {
                reissue_messages_.push_back(reserved_message);
            }
        }
        if (now - last_alarm_timestamp_ <= 30) {
            spdlog::warn("采集间隔调整为1秒[{}]", now);
            collect_interval_ = 1;
        }
    } else if (last_alarm_timestamp_ > 0 && now - last_alarm_timestamp_ > 30) {
        last_alarm_timestamp_ = 0;
        collect_interval_ = 10;
    }
    if (now - last_collect_timestamp_ >= collect_interval_) {
        last_collect_timestamp_ = now;
        if (is_tsp_login_ && is_vehicle_login_) {
            int mid = 0;
            std::vector<uint8_t> message = build_message(REALTIME_REPORT, realtime_signal);
            return MqttClient::get_instance().publish(mid, mqtt_topic_, &message, 1);
        }
        reissue_messages_.push_back(realtime_signal);
    }
    return true;
}

void RsmsClient::logout() {
    spdlog::info("车辆登出");
    std::vector<uint8_t> vehicle_logout = build_vehicle_logout();
    std::vector<uint8_t> message = build_message(VEHICLE_LOGOUT, vehicle_logout);
    int mid = 0;
    MqttClient::get_instance().publish(mid, mqtt_topic_, &message, 1);
}

std::vector<uint8_t> RsmsClient::get_current_time() {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    // 转换为本地时间
    std::tm *local_time = std::localtime(&time_t_now);
    std::vector<uint8_t> time_bytes(6);
    time_bytes[0] = static_cast<uint8_t>(local_time->tm_year % 100);
    time_bytes[1] = static_cast<uint8_t>(local_time->tm_mon + 1);
    time_bytes[2] = static_cast<uint8_t>(local_time->tm_mday);
    time_bytes[3] = static_cast<uint8_t>(local_time->tm_hour);
    time_bytes[4] = static_cast<uint8_t>(local_time->tm_min);
    time_bytes[5] = static_cast<uint8_t>(local_time->tm_sec);
    return time_bytes;
}

std::vector<uint8_t> RsmsClient::build_vehicle_login() {
    int total_length = 30 + battery_pack_count_ * battery_pack_sn_.size();
    int index = 0;
    std::vector<uint8_t> vehicle_login_bytes(total_length);
    std::vector<uint8_t> time_bytes = get_current_time();
    std::copy(time_bytes.begin(), time_bytes.end(), vehicle_login_bytes.begin() + index);
    index += time_bytes.size();
    login_sn_++;
    std::vector<uint8_t> login_sn_bytes = word_to_bytes(login_sn_);
    std::copy(login_sn_bytes.begin(), login_sn_bytes.end(), vehicle_login_bytes.begin() + index);
    index += login_sn_bytes.size();
    std::vector<uint8_t> iccid_bytes = string_to_bytes(iccid_);
    std::copy(iccid_bytes.begin(), iccid_bytes.end(), vehicle_login_bytes.begin() + index);
    index += iccid_bytes.size();
    vehicle_login_bytes[28] = battery_pack_count_;
    vehicle_login_bytes[29] = battery_pack_sn_.size();
    index += 2;
    std::vector<uint8_t> battery_pack_code_bytes = string_to_bytes(battery_pack_sn_);
    std::copy(battery_pack_code_bytes.begin(), battery_pack_code_bytes.end(), vehicle_login_bytes.begin() + index);
    return vehicle_login_bytes;
}

std::vector<uint8_t> RsmsClient::build_realtime_signal() {
    std::vector<uint8_t> time_bytes = get_current_time();
    std::vector<uint8_t> vehicle_data_bytes = build_vehicle_data();
    std::vector<uint8_t> drive_motor_bytes = build_drive_motor();
    std::vector<uint8_t> position_bytes = build_position();
    std::vector<uint8_t> extremum_bytes = build_extremum();
    std::vector<uint8_t> alarm_bytes = build_alarm();
    std::vector<uint8_t> battery_voltage_bytes = build_battery_voltage();
    std::vector<uint8_t> battery_temperature_bytes = build_battery_temperature();
    int total_length =
            time_bytes.size() + vehicle_data_bytes.size() + drive_motor_bytes.size() + position_bytes.size() +
            extremum_bytes.size() + alarm_bytes.size() + battery_voltage_bytes.size() +
            battery_temperature_bytes.size();
    int index = 0;
    std::vector<uint8_t> realtime_signal_bytes(total_length);
    std::copy(time_bytes.begin(), time_bytes.end(), realtime_signal_bytes.begin() + index);
    index += time_bytes.size();
    std::copy(vehicle_data_bytes.begin(), vehicle_data_bytes.end(), realtime_signal_bytes.begin() + index);
    index += vehicle_data_bytes.size();
    std::copy(drive_motor_bytes.begin(), drive_motor_bytes.end(), realtime_signal_bytes.begin() + index);
    index += drive_motor_bytes.size();
    std::copy(position_bytes.begin(), position_bytes.end(), realtime_signal_bytes.begin() + index);
    index += position_bytes.size();
    std::copy(extremum_bytes.begin(), extremum_bytes.end(), realtime_signal_bytes.begin() + index);
    index += extremum_bytes.size();
    std::copy(alarm_bytes.begin(), alarm_bytes.end(), realtime_signal_bytes.begin() + index);
    index += alarm_bytes.size();
    std::copy(battery_voltage_bytes.begin(), battery_voltage_bytes.end(), realtime_signal_bytes.begin() + index);
    index += battery_voltage_bytes.size();
    std::copy(battery_temperature_bytes.begin(), battery_temperature_bytes.end(),
              realtime_signal_bytes.begin() + index);
    return realtime_signal_bytes;
}

std::vector<uint8_t> RsmsClient::build_vehicle_data() {
    std::vector<uint8_t> vehicle_data_bytes(21);
    vehicle_data_bytes[0] = 0x01; // 整车数据
    uint8_t vehicle_state;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_VEHICLE_STATE, vehicle_state)) {
        vehicle_data_bytes[1] = vehicle_state;
    }
    uint8_t charging_state;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_CHARGING_STATE, charging_state)) {
        vehicle_data_bytes[2] = charging_state;
    }
    uint8_t running_mode;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_RUNNING_MODE, running_mode)) {
        vehicle_data_bytes[3] = running_mode;
    }
    uint16_t speed;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_SPEED, speed)) {
        std::vector<uint8_t> speed_bytes = word_to_bytes(speed);
        vehicle_data_bytes[4] = speed_bytes[0];
        vehicle_data_bytes[5] = speed_bytes[1];
    }
    uint32_t total_odometer;
    if (RsmsSignalCache::get_instance().get_dword(signal_t::SIGNAL_TOTAL_ODOMETER, total_odometer)) {
        std::vector<uint8_t> total_odometer_bytes = dword_to_bytes(total_odometer);
        vehicle_data_bytes[6] = total_odometer_bytes[0];
        vehicle_data_bytes[7] = total_odometer_bytes[1];
        vehicle_data_bytes[8] = total_odometer_bytes[2];
        vehicle_data_bytes[9] = total_odometer_bytes[3];
    }
    uint16_t total_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_TOTAL_VOLTAGE, total_voltage)) {
        std::vector<uint8_t> total_voltage_bytes = word_to_bytes(total_voltage);
        vehicle_data_bytes[10] = total_voltage_bytes[0];
        vehicle_data_bytes[11] = total_voltage_bytes[1];
    }
    uint16_t total_current;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_TOTAL_CURRENT, total_current)) {
        std::vector<uint8_t> total_current_bytes = word_to_bytes(total_current);
        vehicle_data_bytes[12] = total_current_bytes[0];
        vehicle_data_bytes[13] = total_current_bytes[1];
    }
    uint8_t soc;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_SOC, soc)) {
        vehicle_data_bytes[14] = soc;
    }
    uint8_t dcdc_state;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DCDC_STATE, dcdc_state)) {
        vehicle_data_bytes[15] = dcdc_state;
    }
    uint8_t gear;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_GEAR, gear)) {
        bool driving;
        if (RsmsSignalCache::get_instance().get_boolean(signal_t::SIGNAL_DRIVING, driving)) {
            gear = ((driving ? 1 : 0) << 5) + gear;
        }
        bool braking;
        if (RsmsSignalCache::get_instance().get_boolean(signal_t::SIGNAL_BRAKING, braking)) {
            gear = ((braking ? 1 : 0) << 4) + gear;
        }
        vehicle_data_bytes[16] = gear;
    }
    uint16_t insulation_resistance;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_INSULATION_RESISTANCE, insulation_resistance)) {
        std::vector<uint8_t> insulation_resistance_bytes = word_to_bytes(insulation_resistance);
        vehicle_data_bytes[17] = insulation_resistance_bytes[0];
        vehicle_data_bytes[18] = insulation_resistance_bytes[1];
    }
    uint8_t accelerator_pedal_position;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_ACCELERATOR_PEDAL_POSITION,
                                                 accelerator_pedal_position)) {
        vehicle_data_bytes[19] = accelerator_pedal_position;
    }
    uint8_t brake_pedal_position;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BRAKE_PEDAL_POSITION, brake_pedal_position)) {
        vehicle_data_bytes[20] = brake_pedal_position;
    }
    return vehicle_data_bytes;
}

std::vector<uint8_t> RsmsClient::build_drive_motor() {
    std::vector<uint8_t> drive_motor_bytes(26);
    drive_motor_bytes[0] = 0x02; // 驱动电机数据
    drive_motor_bytes[1] = 0x02; // 2个电机
    drive_motor_bytes[2] = 0x01; // 第1个电机
    uint8_t dm1_state;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM1_STATE, dm1_state)) {
        drive_motor_bytes[3] = dm1_state;
    }
    uint8_t dm1_controller_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM1_CONTROLLER_TEMPERATURE,
                                                 dm1_controller_temperature)) {
        drive_motor_bytes[4] = dm1_controller_temperature;
    }
    uint16_t dm1_speed;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM1_SPEED, dm1_speed)) {
        std::vector<uint8_t> dm1_speed_bytes = word_to_bytes(dm1_speed);
        drive_motor_bytes[5] = dm1_speed_bytes[0];
        drive_motor_bytes[6] = dm1_speed_bytes[1];
    }
    uint16_t dm1_torque;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM1_TORQUE, dm1_torque)) {
        std::vector<uint8_t> dm1_torque_bytes = word_to_bytes(dm1_torque);
        drive_motor_bytes[7] = dm1_torque_bytes[0];
        drive_motor_bytes[8] = dm1_torque_bytes[1];
    }
    uint8_t dm1_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM1_TEMPERATURE, dm1_temperature)) {
        drive_motor_bytes[9] = dm1_temperature;
    }
    uint16_t dm1_controller_input_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM1_CONTROLLER_INPUT_VOLTAGE,
                                                 dm1_controller_input_voltage)) {
        std::vector<uint8_t> dm1_controller_input_voltage_bytes = word_to_bytes(dm1_controller_input_voltage);
        drive_motor_bytes[10] = dm1_controller_input_voltage_bytes[0];
        drive_motor_bytes[11] = dm1_controller_input_voltage_bytes[1];
    }
    uint16_t dm1_controller_dc_bus_current;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM1_CONTROLLER_DC_BUS_CURRENT,
                                                 dm1_controller_dc_bus_current)) {
        std::vector<uint8_t> dm1_controller_dc_bus_current_bytes = word_to_bytes(dm1_controller_dc_bus_current);
        drive_motor_bytes[12] = dm1_controller_dc_bus_current_bytes[0];
        drive_motor_bytes[13] = dm1_controller_dc_bus_current_bytes[1];
    }
    drive_motor_bytes[14] = 0x02; // 第2个电机
    uint8_t dm2_state;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM2_STATE, dm2_state)) {
        drive_motor_bytes[15] = dm2_state;
    }
    uint8_t dm2_controller_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM2_CONTROLLER_TEMPERATURE,
                                                 dm2_controller_temperature)) {
        drive_motor_bytes[16] = dm2_controller_temperature;
    }
    uint16_t dm2_speed;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM2_SPEED, dm2_speed)) {
        std::vector<uint8_t> dm2_speed_bytes = word_to_bytes(dm2_speed);
        drive_motor_bytes[17] = dm2_speed_bytes[0];
        drive_motor_bytes[18] = dm2_speed_bytes[1];
    }
    uint16_t dm2_torque;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM2_TORQUE, dm2_torque)) {
        std::vector<uint8_t> dm2_torque_bytes = word_to_bytes(dm2_torque);
        drive_motor_bytes[19] = dm2_torque_bytes[0];
        drive_motor_bytes[20] = dm2_torque_bytes[1];
    }
    uint8_t dm2_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DM2_TEMPERATURE, dm2_temperature)) {
        drive_motor_bytes[21] = dm2_temperature;
    }
    uint16_t dm2_controller_input_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM2_CONTROLLER_INPUT_VOLTAGE,
                                                 dm2_controller_input_voltage)) {
        std::vector<uint8_t> dm2_controller_input_voltage_bytes = word_to_bytes(dm2_controller_input_voltage);
        drive_motor_bytes[22] = dm2_controller_input_voltage_bytes[0];
        drive_motor_bytes[23] = dm2_controller_input_voltage_bytes[1];
    }
    uint16_t dm2_controller_dc_bus_current;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_DM2_CONTROLLER_DC_BUS_CURRENT,
                                                 dm2_controller_dc_bus_current)) {
        std::vector<uint8_t> dm2_controller_dc_bus_current_bytes = word_to_bytes(dm2_controller_dc_bus_current);
        drive_motor_bytes[24] = dm2_controller_dc_bus_current_bytes[0];
        drive_motor_bytes[25] = dm2_controller_dc_bus_current_bytes[1];
    }
    return drive_motor_bytes;
}

std::vector<uint8_t> RsmsClient::build_position() {
    std::vector<uint8_t> position_bytes(10);
    position_bytes[0] = 0x05; // 车辆位置数据
    bool position_valid;
    if (RsmsSignalCache::get_instance().get_boolean(signal_t::SIGNAL_POSITION_VALID, position_valid)) {
        uint8_t position = (position_valid) ? 0 : 1;
        bool south_latitude;
        if (RsmsSignalCache::get_instance().get_boolean(signal_t::SIGNAL_SOUTH_LATITUDE, south_latitude)) {
            position = position + ((south_latitude ? 1 : 0) << 1);
        }
        bool west_longitude;
        if (RsmsSignalCache::get_instance().get_boolean(signal_t::SIGNAL_WEST_LONGITUDE, west_longitude)) {
            position = position + ((west_longitude ? 1 : 0) << 2);
        }
        position_bytes[1] = position;
    }
    uint32_t longitude;
    if (RsmsSignalCache::get_instance().get_dword(signal_t::SIGNAL_LONGITUDE, longitude)) {
        std::vector<uint8_t> longitude_bytes = dword_to_bytes(longitude);
        position_bytes[2] = longitude_bytes[0];
        position_bytes[3] = longitude_bytes[1];
        position_bytes[4] = longitude_bytes[2];
        position_bytes[5] = longitude_bytes[3];
    }
    uint32_t latitude;
    if (RsmsSignalCache::get_instance().get_dword(signal_t::SIGNAL_LATITUDE, latitude)) {
        std::vector<uint8_t> latitude_bytes = dword_to_bytes(latitude);
        position_bytes[6] = latitude_bytes[0];
        position_bytes[7] = latitude_bytes[1];
        position_bytes[8] = latitude_bytes[2];
        position_bytes[9] = latitude_bytes[3];
    }
    return position_bytes;
}

std::vector<uint8_t> RsmsClient::build_extremum() {
    std::vector<uint8_t> extremum_bytes(15);
    extremum_bytes[0] = 0x06; // 极值数据
    uint8_t max_voltage_battery_device_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_VOLTAGE_BATTERY_DEVICE_NO,
                                                 max_voltage_battery_device_no)) {
        extremum_bytes[1] = max_voltage_battery_device_no;
    }
    uint8_t max_voltage_cell_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_VOLTAGE_CELL_NO, max_voltage_cell_no)) {
        extremum_bytes[2] = max_voltage_cell_no;
    }
    uint16_t cell_max_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_CELL_MAX_VOLTAGE, cell_max_voltage)) {
        std::vector<uint8_t> cell_max_voltage_bytes = word_to_bytes(cell_max_voltage);
        extremum_bytes[3] = cell_max_voltage_bytes[0];
        extremum_bytes[4] = cell_max_voltage_bytes[1];
    }
    uint8_t min_voltage_battery_device_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MIN_VOLTAGE_BATTERY_DEVICE_NO,
                                                 min_voltage_battery_device_no)) {
        extremum_bytes[5] = min_voltage_battery_device_no;
    }
    uint8_t min_voltage_cell_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MIN_VOLTAGE_CELL_NO, min_voltage_cell_no)) {
        extremum_bytes[6] = min_voltage_cell_no;
    }
    uint16_t cell_min_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_CELL_MIN_VOLTAGE, cell_min_voltage)) {
        std::vector<uint8_t> cell_min_voltage_bytes = word_to_bytes(cell_min_voltage);
        extremum_bytes[7] = cell_min_voltage_bytes[0];
        extremum_bytes[8] = cell_min_voltage_bytes[1];
    }
    uint8_t max_temperature_device_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_TEMPERATURE_DEVICE_NO,
                                                 max_temperature_device_no)) {
        extremum_bytes[9] = max_temperature_device_no;
    }
    uint8_t max_temperature_probe_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_TEMPERATURE_PROBE_NO,
                                                 max_temperature_probe_no)) {
        extremum_bytes[10] = max_temperature_probe_no;
    }
    uint8_t max_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_TEMPERATURE, max_temperature)) {
        extremum_bytes[11] = max_temperature;
    }
    uint8_t min_temperature_device_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MIN_TEMPERATURE_DEVICE_NO,
                                                 min_temperature_device_no)) {
        extremum_bytes[12] = min_temperature_device_no;
    }
    uint8_t min_temperature_probe_no;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MIN_TEMPERATURE_PROBE_NO,
                                                 min_temperature_probe_no)) {
        extremum_bytes[13] = min_temperature_probe_no;
    }
    uint8_t min_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MIN_TEMPERATURE, min_temperature)) {
        extremum_bytes[14] = min_temperature;
    }
    return extremum_bytes;
}

std::vector<uint8_t> RsmsClient::build_alarm() {
    uint8_t battery_fault_count;
    if (!RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY_FAULT_COUNT, battery_fault_count)) {
        return {};
    }
    uint8_t drive_motor_fault_count;
    if (!RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_DRIVE_MOTOR_FAULT_COUNT, drive_motor_fault_count)) {
        return {};
    }
    uint8_t engine_fault_count;
    if (!RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_ENGINE_FAULT_COUNT, engine_fault_count)) {
        return {};
    }
    uint8_t other_fault_count;
    if (!RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_OTHER_FAULT_COUNT, other_fault_count)) {
        return {};
    }
    int total_size =
            10 + battery_fault_count * 4 + drive_motor_fault_count * 4 + engine_fault_count * 4 + other_fault_count * 4;
    std::vector<uint8_t> alarm_bytes(total_size);
    alarm_bytes[0] = 0x07; // 报警数据
    uint8_t max_alarm_level;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_ALARM_LEVEL, max_alarm_level)) {
        alarm_bytes[1] = max_alarm_level;
    }
    uint32_t alarm_flag;
    if (RsmsSignalCache::get_instance().get_dword(signal_t::SIGNAL_ALARM_FLAG, alarm_flag)) {
        std::vector<uint8_t> alarm_flag_bytes = dword_to_bytes(alarm_flag);
        alarm_bytes[2] = alarm_flag_bytes[0];
        alarm_bytes[3] = alarm_flag_bytes[1];
        alarm_bytes[4] = alarm_flag_bytes[2];
        alarm_bytes[5] = alarm_flag_bytes[3];
    }
    // 默认现在都是0
    alarm_bytes[6] = battery_fault_count;
    alarm_bytes[7] = drive_motor_fault_count;
    alarm_bytes[8] = engine_fault_count;
    alarm_bytes[9] = other_fault_count;
    return alarm_bytes;
}

bool RsmsClient::is_alarm3() {
    uint8_t max_alarm_level;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_MAX_ALARM_LEVEL, max_alarm_level)) {
        return max_alarm_level == 3;
    }
    return false;
}

std::vector<uint8_t> RsmsClient::build_battery_voltage() {
    std::vector<uint8_t> battery_voltage_bytes(44);
    battery_voltage_bytes[0] = 0x08; // 可充电储能装置电压数据
    battery_voltage_bytes[1] = 0x01; // 电池包1个
    battery_voltage_bytes[2] = 0x01; // 第1个电池包
    uint16_t battery1_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_VOLTAGE, battery1_voltage)) {
        std::vector<uint8_t> battery1_voltage_bytes = word_to_bytes(battery1_voltage);
        battery_voltage_bytes[3] = battery1_voltage_bytes[0];
        battery_voltage_bytes[4] = battery1_voltage_bytes[1];
    }
    uint16_t battery1_current;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CURRENT, battery1_current)) {
        std::vector<uint8_t> battery1_current_bytes = word_to_bytes(battery1_current);
        battery_voltage_bytes[5] = battery1_current_bytes[0];
        battery_voltage_bytes[6] = battery1_current_bytes[1];
    }
    uint16_t battery1_cell_count;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL_COUNT, battery1_cell_count)) {
        std::vector<uint8_t> battery1_cell_count_bytes = word_to_bytes(battery1_cell_count);
        battery_voltage_bytes[7] = battery1_cell_count_bytes[0];
        battery_voltage_bytes[8] = battery1_cell_count_bytes[1];
        std::vector<uint8_t> frame_sn_bytes = word_to_bytes(1); // 起始电池序号
        battery_voltage_bytes[9] = frame_sn_bytes[0];
        battery_voltage_bytes[10] = frame_sn_bytes[1];
        battery_voltage_bytes[11] = battery1_cell_count;
    }
    uint16_t battery1_cell1_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL1_VOLTAGE, battery1_cell1_voltage)) {
        std::vector<uint8_t> battery1_cell1_voltage_bytes = word_to_bytes(battery1_cell1_voltage);
        battery_voltage_bytes[12] = battery1_cell1_voltage_bytes[0];
        battery_voltage_bytes[13] = battery1_cell1_voltage_bytes[1];
    }
    uint16_t battery1_cell2_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL2_VOLTAGE, battery1_cell2_voltage)) {
        std::vector<uint8_t> battery1_cell2_voltage_bytes = word_to_bytes(battery1_cell2_voltage);
        battery_voltage_bytes[14] = battery1_cell2_voltage_bytes[0];
        battery_voltage_bytes[15] = battery1_cell2_voltage_bytes[1];
    }
    uint16_t battery1_cell3_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL3_VOLTAGE, battery1_cell3_voltage)) {
        std::vector<uint8_t> battery1_cell3_voltage_bytes = word_to_bytes(battery1_cell3_voltage);
        battery_voltage_bytes[16] = battery1_cell3_voltage_bytes[0];
        battery_voltage_bytes[17] = battery1_cell3_voltage_bytes[1];
    }
    uint16_t battery1_cell4_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL4_VOLTAGE, battery1_cell4_voltage)) {
        std::vector<uint8_t> battery1_cell4_voltage_bytes = word_to_bytes(battery1_cell4_voltage);
        battery_voltage_bytes[18] = battery1_cell4_voltage_bytes[0];
        battery_voltage_bytes[19] = battery1_cell4_voltage_bytes[1];
    }
    uint16_t battery1_cell5_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL5_VOLTAGE, battery1_cell5_voltage)) {
        std::vector<uint8_t> battery1_cell5_voltage_bytes = word_to_bytes(battery1_cell5_voltage);
        battery_voltage_bytes[20] = battery1_cell5_voltage_bytes[0];
        battery_voltage_bytes[21] = battery1_cell5_voltage_bytes[1];
    }
    uint16_t battery1_cell6_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL6_VOLTAGE, battery1_cell6_voltage)) {
        std::vector<uint8_t> battery1_cell6_voltage_bytes = word_to_bytes(battery1_cell6_voltage);
        battery_voltage_bytes[22] = battery1_cell6_voltage_bytes[0];
        battery_voltage_bytes[23] = battery1_cell6_voltage_bytes[1];
    }
    uint16_t battery1_cell7_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL7_VOLTAGE, battery1_cell7_voltage)) {
        std::vector<uint8_t> battery1_cell7_voltage_bytes = word_to_bytes(battery1_cell7_voltage);
        battery_voltage_bytes[24] = battery1_cell7_voltage_bytes[0];
        battery_voltage_bytes[25] = battery1_cell7_voltage_bytes[1];
    }
    uint16_t battery1_cell8_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL8_VOLTAGE, battery1_cell8_voltage)) {
        std::vector<uint8_t> battery1_cell8_voltage_bytes = word_to_bytes(battery1_cell8_voltage);
        battery_voltage_bytes[26] = battery1_cell8_voltage_bytes[0];
        battery_voltage_bytes[27] = battery1_cell8_voltage_bytes[1];
    }
    uint16_t battery1_cell9_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL9_VOLTAGE, battery1_cell9_voltage)) {
        std::vector<uint8_t> battery1_cell9_voltage_bytes = word_to_bytes(battery1_cell9_voltage);
        battery_voltage_bytes[28] = battery1_cell9_voltage_bytes[0];
        battery_voltage_bytes[29] = battery1_cell9_voltage_bytes[1];
    }
    uint16_t battery1_cell10_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL10_VOLTAGE, battery1_cell10_voltage)) {
        std::vector<uint8_t> battery1_cell10_voltage_bytes = word_to_bytes(battery1_cell10_voltage);
        battery_voltage_bytes[30] = battery1_cell10_voltage_bytes[0];
        battery_voltage_bytes[31] = battery1_cell10_voltage_bytes[1];
    }
    uint16_t battery1_cell11_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL11_VOLTAGE, battery1_cell11_voltage)) {
        std::vector<uint8_t> battery1_cell11_voltage_bytes = word_to_bytes(battery1_cell11_voltage);
        battery_voltage_bytes[32] = battery1_cell11_voltage_bytes[0];
        battery_voltage_bytes[33] = battery1_cell11_voltage_bytes[1];
    }
    uint16_t battery1_cell12_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL12_VOLTAGE, battery1_cell12_voltage)) {
        std::vector<uint8_t> battery1_cell12_voltage_bytes = word_to_bytes(battery1_cell12_voltage);
        battery_voltage_bytes[34] = battery1_cell12_voltage_bytes[0];
        battery_voltage_bytes[35] = battery1_cell12_voltage_bytes[1];
    }
    uint16_t battery1_cell13_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL13_VOLTAGE, battery1_cell13_voltage)) {
        std::vector<uint8_t> battery1_cell13_voltage_bytes = word_to_bytes(battery1_cell13_voltage);
        battery_voltage_bytes[36] = battery1_cell13_voltage_bytes[0];
        battery_voltage_bytes[37] = battery1_cell13_voltage_bytes[1];
    }
    uint16_t battery1_cell14_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL14_VOLTAGE, battery1_cell14_voltage)) {
        std::vector<uint8_t> battery1_cell14_voltage_bytes = word_to_bytes(battery1_cell14_voltage);
        battery_voltage_bytes[38] = battery1_cell14_voltage_bytes[0];
        battery_voltage_bytes[39] = battery1_cell14_voltage_bytes[1];
    }
    uint16_t battery1_cell15_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL15_VOLTAGE, battery1_cell15_voltage)) {
        std::vector<uint8_t> battery1_cell15_voltage_bytes = word_to_bytes(battery1_cell15_voltage);
        battery_voltage_bytes[40] = battery1_cell15_voltage_bytes[0];
        battery_voltage_bytes[41] = battery1_cell15_voltage_bytes[1];
    }
    uint16_t battery1_cell16_voltage;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_CELL16_VOLTAGE, battery1_cell16_voltage)) {
        std::vector<uint8_t> battery1_cell16_voltage_bytes = word_to_bytes(battery1_cell16_voltage);
        battery_voltage_bytes[42] = battery1_cell16_voltage_bytes[0];
        battery_voltage_bytes[43] = battery1_cell16_voltage_bytes[1];
    }
    return battery_voltage_bytes;
}

std::vector<uint8_t> RsmsClient::build_battery_temperature() {
    std::vector<uint8_t> battery_temperature_bytes(20);
    battery_temperature_bytes[0] = 0x09; // 可充电储能装置温度数据
    battery_temperature_bytes[1] = 0x01;
    battery_temperature_bytes[2] = 0x01;
    uint16_t battery1_probe_count;
    if (RsmsSignalCache::get_instance().get_word(signal_t::SIGNAL_BATTERY1_PROBE_COUNT, battery1_probe_count)) {
        std::vector<uint8_t> battery1_probe_count_bytes = word_to_bytes(battery1_probe_count);
        battery_temperature_bytes[3] = battery1_probe_count_bytes[0];
        battery_temperature_bytes[4] = battery1_probe_count_bytes[1];
    }
    uint8_t battery1_probe1_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE1_TEMPERATURE,
                                                 battery1_probe1_temperature)) {
        battery_temperature_bytes[5] = battery1_probe1_temperature;
    }
    uint8_t battery1_probe2_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE2_TEMPERATURE,
                                                 battery1_probe2_temperature)) {
        battery_temperature_bytes[6] = battery1_probe2_temperature;
    }
    uint8_t battery1_probe3_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE3_TEMPERATURE,
                                                 battery1_probe3_temperature)) {
        battery_temperature_bytes[7] = battery1_probe3_temperature;
    }
    uint8_t battery1_probe4_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE4_TEMPERATURE,
                                                 battery1_probe4_temperature)) {
        battery_temperature_bytes[8] = battery1_probe4_temperature;
    }
    uint8_t battery1_probe5_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE5_TEMPERATURE,
                                                 battery1_probe5_temperature)) {
        battery_temperature_bytes[9] = battery1_probe5_temperature;
    }
    uint8_t battery1_probe6_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE6_TEMPERATURE,
                                                 battery1_probe6_temperature)) {
        battery_temperature_bytes[10] = battery1_probe6_temperature;
    }
    uint8_t battery1_probe7_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE7_TEMPERATURE,
                                                 battery1_probe7_temperature)) {
        battery_temperature_bytes[11] = battery1_probe7_temperature;
    }
    uint8_t battery1_probe8_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE8_TEMPERATURE,
                                                 battery1_probe8_temperature)) {
        battery_temperature_bytes[12] = battery1_probe8_temperature;
    }
    uint8_t battery1_probe9_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE9_TEMPERATURE,
                                                 battery1_probe9_temperature)) {
        battery_temperature_bytes[13] = battery1_probe9_temperature;
    }
    uint8_t battery1_probe10_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE10_TEMPERATURE,
                                                 battery1_probe10_temperature)) {
        battery_temperature_bytes[14] = battery1_probe10_temperature;
    }
    uint8_t battery1_probe11_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE11_TEMPERATURE,
                                                 battery1_probe11_temperature)) {
        battery_temperature_bytes[15] = battery1_probe11_temperature;
    }
    uint8_t battery1_probe12_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE12_TEMPERATURE,
                                                 battery1_probe12_temperature)) {
        battery_temperature_bytes[16] = battery1_probe12_temperature;
    }
    uint8_t battery1_probe13_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE13_TEMPERATURE,
                                                 battery1_probe13_temperature)) {
        battery_temperature_bytes[17] = battery1_probe13_temperature;
    }
    uint8_t battery1_probe14_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE14_TEMPERATURE,
                                                 battery1_probe14_temperature)) {
        battery_temperature_bytes[18] = battery1_probe14_temperature;
    }
    uint8_t battery1_probe15_temperature;
    if (RsmsSignalCache::get_instance().get_byte(signal_t::SIGNAL_BATTERY1_PROBE15_TEMPERATURE,
                                                 battery1_probe15_temperature)) {
        battery_temperature_bytes[19] = battery1_probe15_temperature;
    }
    return battery_temperature_bytes;
}

std::vector<uint8_t> RsmsClient::build_vehicle_logout() {
    int index = 0;
    std::vector<uint8_t> vehicle_logout_bytes(8);
    std::vector<uint8_t> time_bytes = get_current_time();
    std::copy(time_bytes.begin(), time_bytes.end(), vehicle_logout_bytes.begin() + index);
    index += time_bytes.size();
    std::vector<uint8_t> login_sn_bytes = word_to_bytes(login_sn_);
    std::copy(login_sn_bytes.begin(), login_sn_bytes.end(), vehicle_logout_bytes.begin() + index);
    return vehicle_logout_bytes;
}

uint8_t RsmsClient::calculate_check_code(std::vector<uint8_t> data_unit) {
    if (data_unit.empty()) {
        return 0;
    }
    uint8_t check_code = 0;
    for (uint8_t byte_val: data_unit) {
        check_code ^= byte_val;
    }
    return check_code;
}

std::vector<uint8_t> RsmsClient::build_message(command_flag_t command_flag, const std::vector<uint8_t> &data_unit) {
    int total_length = 24 + data_unit.size() + 1;
    std::vector<uint8_t> message_bytes(total_length);
    std::vector<uint8_t> starting_symbols_bytes = string_to_bytes(starting_symbols_);
    std::copy(starting_symbols_bytes.begin(), starting_symbols_bytes.end(), message_bytes.begin());
    message_bytes[2] = command_flag;
    message_bytes[3] = COMMAND;
    std::vector<uint8_t> vin_bytes = string_to_bytes(vin_);
    std::copy(vin_bytes.begin(), vin_bytes.end(), message_bytes.begin() + 4);
    message_bytes[21] = NONE;
    std::vector<uint8_t> data_unit_length = word_to_bytes(data_unit.size());
    std::copy(data_unit_length.begin(), data_unit_length.end(), message_bytes.begin() + 22);
    std::copy(data_unit.begin(), data_unit.end(), message_bytes.begin() + 24);
    int check_length = 22 + data_unit.size();
    std::vector<uint8_t> check_bytes(check_length);
    std::copy(message_bytes.begin() + 2, message_bytes.end() - 1, check_bytes.begin());
    message_bytes[total_length - 1] = calculate_check_code(check_bytes);
    return message_bytes;
}

std::vector<uint8_t> RsmsClient::word_to_bytes(uint16_t value) {
    std::vector<uint8_t> word_bytes(2);
    word_bytes[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    word_bytes[1] = static_cast<uint8_t>(value & 0xFF);
    return word_bytes;
}

std::vector<uint8_t> RsmsClient::dword_to_bytes(uint32_t value) {
    std::vector<uint8_t> dword_bytes(4);
    dword_bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    dword_bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    dword_bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    dword_bytes[3] = static_cast<uint8_t>(value & 0xFF);
    return dword_bytes;
}

std::vector<uint8_t> RsmsClient::string_to_bytes(std::string value) {
    std::vector<uint8_t> string_bytes(value.begin(), value.end());
    return string_bytes;
}

void RsmsClient::collect_thread() {
    spdlog::info("初始化采集线程");
    while (is_start_) {
        collect_signal();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void RsmsClient::reissue_thread() {
    spdlog::info("初始化补发线程");
    while (is_start_) {
        if (is_tsp_login_ && is_vehicle_login_) {
            std::vector<std::vector<uint8_t>> messages_to_send;
            {
                std::unique_lock<std::mutex> lock(reissue_mutex_);
                int count = 0;
                while (!reissue_messages_.empty() && count < reissue_count_per_second_) {
                    messages_to_send.push_back(reissue_messages_.front());
                    reissue_messages_.pop_front();
                    count++;
                }
            }
            if (!messages_to_send.empty()) {
                spdlog::info("开始补发数据[{}]，数量[{}]",
                             hwyz::Utils::get_current_timestamp_sec(), messages_to_send.size());
                for (const auto &realtime_signal: messages_to_send) {
                    int mid = 0;
                    std::vector<uint8_t> message = build_message(REISSUE_REPORT, realtime_signal);
                    MqttClient::get_instance().publish(mid, mqtt_topic_, &message, 1);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}


