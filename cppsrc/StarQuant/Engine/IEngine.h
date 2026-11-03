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
#ifndef CPPSRC_STARQUANT_ENGINE_IENGINE_H_
#define CPPSRC_STARQUANT_ENGINE_IENGINE_H_

#include <Common/msgq.h>
#include <Common/logger.h>
#include <memory>

namespace StarQuant
{
//////////////////////////////////////////////////////////////////////////
// EState - 引擎连接状态枚举
//
// 用于 MD（行情）和 TD（交易）引擎的生命周期管理。
// CTP 与 TAP 的状态流转略有差异，子类在 connect/disconnect 中驱动状态变化。
//////////////////////////////////////////////////////////////////////////
enum EState : int32_t {
    DISCONNECTED = 0,    // 初始/已断开
    CONNECTING,           // 连接中
    CONNECT_ACK,          // CTP：前置已连接；TAP：已登录
    AUTHENTICATING,       // 认证中（CTP 交易）
    AUTHENTICATE_ACK,     // CTP 交易已认证
    LOGINING,             // 登录中
    LOGIN_ACK,            // CTP：已登录；TAP：API 就绪可操作
    LOGOUTING,             // 登出中
    STOP                  // 引擎已停止
};

//////////////////////////////////////////////////////////////////////////
// IEngine - 引擎抽象基类
//
// 功能概述：
//   所有行情引擎（CtpMDEngine、TapMDEngine）和交易引擎（CtpTDEngine、TapTDEngine、
//   PaperTDEngine）的基类，统一接口与生命周期（init/start/stop/connect/disconnect）。
//
// 设计模式：模板方法 + 接口抽象
//   - connect/disconnect 为纯虚函数，子类实现具体 API 连接逻辑
//   - init/start/stop 提供默认空实现，子类可按需重写
//
// 依赖：Common/msgq.h（IMessenger）、Common/logger.h（SQLogger）
//////////////////////////////////////////////////////////////////////////
class IEngine {
 public:
    std::atomic<EState> estate_;           // 引擎状态（原子变量，线程安全）
    std::unique_ptr<IMessenger> messenger_;  // 消息收发器（发送行情/订单等至客户端）

    IEngine();
    virtual ~IEngine();

    virtual void init();       // 初始化（默认空实现）
    virtual void start();      // 启动引擎主循环（默认空实现）
    virtual void stop();      // 停止引擎，将 estate_ 置为 STOP
    virtual bool connect() = 0;     // 连接交易所/行情源（纯虚，子类实现）
    virtual bool disconnect() = 0;  // 断开连接（纯虚，子类实现）

 protected:
    std::shared_ptr<SQLogger> logger;  // 日志器
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_ENGINE_IENGINE_H_
