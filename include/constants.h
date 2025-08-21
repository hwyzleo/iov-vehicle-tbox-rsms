//
// Created by hwyz_leo on 2024/9/5.
//
#include <string>
#include <set>

#ifndef TSPSERVICE_CONSTANTS_H
#define TSPSERVICE_CONSTANTS_H
// 信号缓存KEY
// 车辆状态
const std::string kSignalVehicleState = "vehicleState";
// 充电状态
const std::string kSignalChargingState = "chargingState";
// 运行模式
const std::string kSignalRunningMode = "runningMode";
// 车速
const std::string kSignalSpeed = "speed";
// 累计里程
const std::string kSignalTotalOdometer = "totalOdometer";
// 总电压
const std::string kSignalTotalVoltage = "totalVoltage";
// 总电流
const std::string kSignalTotalCurrent = "totalCurrent";
// SOC
const std::string kSignalSoc = "soc";
// DC/DC状态
const std::string kSignalDcdcState = "dcdcState";
// 有驱动力
const std::string kSignalDriving = "driving";
// 有制动力
const std::string kSignalBraking = "braking";
// 挡位
const std::string kSignalGear = "gear";
// 绝缘电阻
const std::string kSignalInsulationResistance = "insulationResistance";
// 加速踏板行程值
const std::string kSignalAcceleratorPedalPosition = "acceleratorPedalPosition";
// 制动踏板状态
const std::string kSignalBrakePedalState = "brakePedalState";
#endif //TSPSERVICE_CONSTANTS_H
