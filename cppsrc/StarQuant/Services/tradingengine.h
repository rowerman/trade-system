/*****************************************************************************
 * Copyright [2019] 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#ifndef CPPSRC_STARQUANT_SERVICES_TRADINGENGINE_H_
#define CPPSRC_STARQUANT_SERVICES_TRADINGENGINE_H_

#include <Common/datastruct.h>
#include <Engine/IEngine.h>
#include <atomic>
#include <thread>
#include <memory>
#include <vector>

using namespace std;

namespace StarQuant {

//////////////////////////////////////////////////////////////////////////
// startengine - 引擎启动辅助函数
// 在新线程中调用 IEngine::start()，供 std::thread 使用
//////////////////////////////////////////////////////////////////////////
void startengine(shared_ptr<IEngine> pe);

//////////////////////////////////////////////////////////////////////////
// tradingengine - 交易引擎核心控制器
//
// 功能概述：
//   管理多个行情/交易引擎实例，协调启动停止、定时任务（cronjobs）、
//   消息中继（msg_relay_）。支持 TRADE_MODE、REPLAY_MODE、RECORD_MODE。
//
// 消息流：策略/服务 PUSH -> SERVERPULL -> msg_relay_->relay() -> PUB -> 各引擎
//////////////////////////////////////////////////////////////////////////
class DLL_EXPORT_IMPORT tradingengine {
    RUN_MODE mode = RUN_MODE::TRADE_MODE;       // 运行模式：交易/回放/录制
    BROKERS _broker = BROKERS::PAPER;          // 券商类型
    vector<std::thread*> threads_;              // 引擎线程指针
    vector<std::shared_ptr<IEngine>> pengines_; // 引擎实例（CtpMDEngine、CtpTDEngine 等）
    std::unique_ptr<IMessenger> msg_relay_;     // 消息中继（CMsgqRMessenger，PULL 接收并 relay 到 PUB）
    std::shared_ptr<SQLogger> logger;

public:
    /** @brief 定时任务：SIGINT/SIGPWR 处理、每秒 MSG_TYPE_TIMER、2:35/16:00 ENGINE_RESET、20:30 日切、8:45/13:15/20:45 自动连接 */
    int32_t cronjobs(bool force = true);

    /** @brief 主运行：按 mode 启动引擎线程，执行 msg_relay_->relay() 循环 */
    int32_t run();

    /** @brief 是否存活（注意：当前实现返回 gShutdown，语义可能为“已关闭”） */
    bool live() const;

    tradingengine();
    ~tradingengine();

    // 禁止拷贝/移动（Boost.Python 等绑定需要）
    tradingengine(tradingengine&&) = delete;
    tradingengine(const tradingengine&) = delete;
    tradingengine& operator=(tradingengine&&) = delete;
    tradingengine& operator=(const tradingengine&) = delete;
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_SERVICES_TRADINGENGINE_H_

