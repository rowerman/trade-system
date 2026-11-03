#ifndef _StarQuant_Services_DataService_H_
#define _StarQuant_Services_DataService_H_

#include <memory>
#include <Common/config.h>

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// 数据服务函数声明
//
// 功能概述：
//   提供行情数据相关的服务：数据看板、Bar 聚合、Tick 录制、Tick 回放。
//   与消息队列（SUB/PUB）及 DataManager、TickWriter、TickReader 协作。
//////////////////////////////////////////////////////////////////////////

/** @brief 数据看板服务：订阅行情，更新 DataManager（当前主逻辑已注释） */
void DataBoardService();

/** @brief Bar 聚合服务：将 Tick 聚合成 K 线（当前空实现，TODO: 从 DataBoardService 分离） */
void BarAggregatorServcie();  // 注：函数名拼写为 Servcie，应为 Service

/** @brief Tick 录制服务：从 msgq_sub_ 接收行情，追加写入 dataDir/marketdata-YYYYMMDD.txt */
void TickRecordingService();

/** @brief Tick 回放服务：从文件读取历史行情，按 tickinterval 间隔发布到 msgq_pub_
 *  @param filetoreplay 回放文件路径
 *  @param tickinterval 每条消息间隔毫秒数，默认 0 */
void TickReplayService(const std::string& filetoreplay, int32_t tickinterval = 0);

}  // namespace StarQuant

#endif  // _StarQuant_Services_DataService_H_
