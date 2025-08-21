//
// Created by hwyz_leo on 2025/8/13.
//
#include "spdlog/spdlog.h"

#include "mqtt_mcu_handler.h"
#include "rsms_data_v1.pb.h"
#include "rsms_signal_cache.h"

MqttMcuHandler &MqttMcuHandler::get_instance() {
    static MqttMcuHandler instance;
    return instance;
}

void MqttMcuHandler::handle(std::string payload) {
    tbox::mcu::rsms::v1::RsmsData rsms_data;
    if (!rsms_data.ParseFromString(payload)) {
        spdlog::error("解析MCU国标数据失败");
        return;
    }

    RsmsSignalCache &instance = RsmsSignalCache::get_instance();
    instance.set_byte(SIGNAL_VEHICLE_STATE, rsms_data.vehicle_data().vehicle_state());
    instance.set_byte(SIGNAL_CHARGING_STATE, rsms_data.vehicle_data().charging_state());
    instance.set_byte(SIGNAL_RUNNING_MODE, rsms_data.vehicle_data().running_mode());
    instance.set_word(SIGNAL_SPEED, rsms_data.vehicle_data().speed());
    instance.set_dword(SIGNAL_TOTAL_ODOMETER, rsms_data.vehicle_data().total_odometer());
    instance.set_word(SIGNAL_TOTAL_VOLTAGE, rsms_data.vehicle_data().total_voltage());
    instance.set_word(SIGNAL_TOTAL_CURRENT, rsms_data.vehicle_data().total_current());
    instance.set_byte(SIGNAL_SOC, rsms_data.vehicle_data().soc());
    instance.set_byte(SIGNAL_DCDC_STATE, rsms_data.vehicle_data().dcdc_state());
    instance.set_boolean(SIGNAL_DRIVING, rsms_data.vehicle_data().driving());
    instance.set_boolean(SIGNAL_BRAKING, rsms_data.vehicle_data().braking());
    instance.set_byte(SIGNAL_GEAR, rsms_data.vehicle_data().gear());
    instance.set_word(SIGNAL_INSULATION_RESISTANCE, rsms_data.vehicle_data().insulation_resistance());
    instance.set_byte(SIGNAL_ACCELERATOR_PEDAL_POSITION, rsms_data.vehicle_data().accelerator_pedal_position());
    instance.set_byte(SIGNAL_BRAKE_PEDAL_POSITION, rsms_data.vehicle_data().brake_pedal_position());
    instance.set_byte(SIGNAL_DM1_STATE, rsms_data.drive_motor().drive_motor_list(0).state());
    instance.set_byte(SIGNAL_DM1_CONTROLLER_TEMPERATURE,
                      rsms_data.drive_motor().drive_motor_list(0).controller_temperature());
    instance.set_word(SIGNAL_DM1_SPEED, rsms_data.drive_motor().drive_motor_list(0).speed());
    instance.set_word(SIGNAL_DM1_TORQUE, rsms_data.drive_motor().drive_motor_list(0).torque());
    instance.set_byte(SIGNAL_DM1_TEMPERATURE, rsms_data.drive_motor().drive_motor_list(0).temperature());
    instance.set_word(SIGNAL_DM1_CONTROLLER_INPUT_VOLTAGE,
                      rsms_data.drive_motor().drive_motor_list(0).controller_input_voltage());
    instance.set_word(SIGNAL_DM1_CONTROLLER_DC_BUS_CURRENT,
                      rsms_data.drive_motor().drive_motor_list(0).controller_dc_bus_current());
    instance.set_byte(SIGNAL_DM2_STATE, rsms_data.drive_motor().drive_motor_list(1).state());
    instance.set_byte(SIGNAL_DM2_CONTROLLER_TEMPERATURE,
                      rsms_data.drive_motor().drive_motor_list(1).controller_temperature());
    instance.set_word(SIGNAL_DM2_SPEED, rsms_data.drive_motor().drive_motor_list(1).speed());
    instance.set_word(SIGNAL_DM2_TORQUE, rsms_data.drive_motor().drive_motor_list(1).torque());
    instance.set_byte(SIGNAL_DM2_TEMPERATURE, rsms_data.drive_motor().drive_motor_list(1).temperature());
    instance.set_word(SIGNAL_DM2_CONTROLLER_INPUT_VOLTAGE,
                      rsms_data.drive_motor().drive_motor_list(1).controller_input_voltage());
    instance.set_word(SIGNAL_DM2_CONTROLLER_DC_BUS_CURRENT,
                      rsms_data.drive_motor().drive_motor_list(1).controller_dc_bus_current());
    instance.set_byte(SIGNAL_POSITION_VALID, rsms_data.position().position_valid());
    instance.set_boolean(SIGNAL_SOUTH_LATITUDE, rsms_data.position().south_latitude());
    instance.set_boolean(SIGNAL_WEST_LONGITUDE, rsms_data.position().west_longitude());
    instance.set_dword(SIGNAL_LONGITUDE, rsms_data.position().longitude());
    instance.set_dword(SIGNAL_LATITUDE, rsms_data.position().latitude());
    instance.set_byte(SIGNAL_MAX_VOLTAGE_BATTERY_DEVICE_NO, rsms_data.extremum().max_voltage_battery_device_no());
    instance.set_byte(SIGNAL_MAX_VOLTAGE_CELL_NO, rsms_data.extremum().max_voltage_cell_no());
    instance.set_word(SIGNAL_CELL_MAX_VOLTAGE, rsms_data.extremum().cell_max_voltage());
    instance.set_byte(SIGNAL_MIN_VOLTAGE_BATTERY_DEVICE_NO, rsms_data.extremum().min_voltage_battery_device_no());
    instance.set_byte(SIGNAL_MIN_VOLTAGE_CELL_NO, rsms_data.extremum().min_voltage_cell_no());
    instance.set_word(SIGNAL_CELL_MIN_VOLTAGE, rsms_data.extremum().cell_min_voltage());
    instance.set_byte(SIGNAL_MAX_TEMPERATURE_DEVICE_NO, rsms_data.extremum().max_temperature_device_no());
    instance.set_byte(SIGNAL_MAX_TEMPERATURE_PROBE_NO, rsms_data.extremum().max_temperature_probe_no());
    instance.set_word(SIGNAL_MAX_TEMPERATURE, rsms_data.extremum().max_temperature());
    instance.set_byte(SIGNAL_MIN_TEMPERATURE_DEVICE_NO, rsms_data.extremum().min_temperature_device_no());
    instance.set_byte(SIGNAL_MIN_TEMPERATURE_PROBE_NO, rsms_data.extremum().min_temperature_probe_no());
    instance.set_word(SIGNAL_MIN_TEMPERATURE, rsms_data.extremum().min_temperature());
    instance.set_byte(SIGNAL_MAX_ALARM_LEVEL, rsms_data.alarm().max_alarm_level());
    instance.set_dword(SIGNAL_ALARM_FLAG, rsms_data.alarm().alarm_flag());
    instance.set_byte(SIGNAL_BATTERY_FAULT_COUNT, rsms_data.alarm().battery_fault_count());
    instance.set_byte(SIGNAL_DRIVE_MOTOR_FAULT_COUNT, rsms_data.alarm().drive_motor_fault_count());
    instance.set_byte(SIGNAL_ENGINE_FAULT_COUNT, rsms_data.alarm().engine_fault_count());
    instance.set_byte(SIGNAL_OTHER_FAULT_COUNT, rsms_data.alarm().other_fault_count());
    instance.set_word(SIGNAL_BATTERY1_VOLTAGE, rsms_data.battery_voltage().battery_voltage_list(0).voltage());
    instance.set_word(SIGNAL_BATTERY1_CURRENT, rsms_data.battery_voltage().battery_voltage_list(0).current());
    instance.set_word(SIGNAL_BATTERY1_CELL_COUNT, rsms_data.battery_voltage().battery_voltage_list(0).cell_count());
    instance.set_word(SIGNAL_BATTERY1_CELL1_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(0));
    instance.set_word(SIGNAL_BATTERY1_CELL2_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(1));
    instance.set_word(SIGNAL_BATTERY1_CELL3_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(2));
    instance.set_word(SIGNAL_BATTERY1_CELL4_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(3));
    instance.set_word(SIGNAL_BATTERY1_CELL5_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(4));
    instance.set_word(SIGNAL_BATTERY1_CELL6_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(5));
    instance.set_word(SIGNAL_BATTERY1_CELL7_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(6));
    instance.set_word(SIGNAL_BATTERY1_CELL8_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(7));
    instance.set_word(SIGNAL_BATTERY1_CELL9_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(8));
    instance.set_word(SIGNAL_BATTERY1_CELL10_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(9));
    instance.set_word(SIGNAL_BATTERY1_CELL11_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(10));
    instance.set_word(SIGNAL_BATTERY1_CELL12_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(11));
    instance.set_word(SIGNAL_BATTERY1_CELL13_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(12));
    instance.set_word(SIGNAL_BATTERY1_CELL14_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(13));
    instance.set_word(SIGNAL_BATTERY1_CELL15_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(14));
    instance.set_word(SIGNAL_BATTERY1_CELL16_VOLTAGE,
                      rsms_data.battery_voltage().battery_voltage_list(0).cell_voltage_list(15));
    instance.set_word(SIGNAL_BATTERY1_PROBE_COUNT,
                      rsms_data.battery_temperature().battery_temperature_list(0).probe_count());
    instance.set_word(SIGNAL_BATTERY1_PROBE1_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(0));
    instance.set_word(SIGNAL_BATTERY1_PROBE2_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(1));
    instance.set_word(SIGNAL_BATTERY1_PROBE3_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(2));
    instance.set_word(SIGNAL_BATTERY1_PROBE4_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(3));
    instance.set_word(SIGNAL_BATTERY1_PROBE5_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(4));
    instance.set_word(SIGNAL_BATTERY1_PROBE6_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(5));
    instance.set_word(SIGNAL_BATTERY1_PROBE7_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(6));
    instance.set_word(SIGNAL_BATTERY1_PROBE8_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(7));
    instance.set_word(SIGNAL_BATTERY1_PROBE9_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(8));
    instance.set_word(SIGNAL_BATTERY1_PROBE10_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(9));
    instance.set_word(SIGNAL_BATTERY1_PROBE11_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(10));
    instance.set_word(SIGNAL_BATTERY1_PROBE12_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(11));
    instance.set_word(SIGNAL_BATTERY1_PROBE13_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(12));
    instance.set_word(SIGNAL_BATTERY1_PROBE14_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(13));
    instance.set_word(SIGNAL_BATTERY1_PROBE15_TEMPERATURE,
                      rsms_data.battery_temperature().battery_temperature_list(0).temperatures(14));
    spdlog::debug("写入信号缓存");
}