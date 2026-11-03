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

/*****************************************************************************
 * 交易引擎实现：核心控制器，管理引擎生命周期、定时任务、消息中继
 *
 * 依赖：CtpMDEngine、CtpTDEngine、PaperTDEngine、DataManager、OrderManager、
 *       PortfolioManager、RiskManager、CMsgqEMessenger、CMsgqRMessenger
 *****************************************************************************/
#include <Services/tradingengine.h>
#include <Common/datastruct.h>
#include <Common/config.h>
#include <Common/util.h>
#include <Common/logger.h>
#include <Data/datamanager.h>
#include <Engine/IEngine.h>
#include <Engine/CtpMDEngine.h>
#include <Engine/CtpTDEngine.h>
#include <Engine/PaperTDEngine.h>
#include <Trade/ordermanager.h>
#include <Trade/portfoliomanager.h>
#include <Trade/riskmanager.h>
#include <Services/dataservice.h>

#include <pthread.h>
#include <signal.h>
#include <future>
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>
#include <functional>

namespace StarQuant {

extern std::atomic<bool> gShutdown;
extern Except_frame g_except_stack;
extern atomic<uint64_t> MICRO_SERVICE_NUMBER;

/** @brief 引擎启动辅助函数：在新线程中调用 pe->start()，进入引擎主循环 */
void startengine(shared_ptr<IEngine> pe) {
    pe->start();
}

/** @brief 构造函数：初始化单例、消息队列、消息中继 */
tradingengine::tradingengine() {
    // 触发各单例初始化
    CConfig::instance();
    DataManager::instance();
    OrderManager::instance();
    PortfolioManager::instance();
    RiskManager::instance();

    _broker = CConfig::instance()._broker;
    mode = CConfig::instance()._mode;

    if (logger == nullptr) {
        logger = SQLogger::getLogger("SYS");
    }

    // 引擎消息发送端：PUB -> SERVERPUB_URL，策略/服务 SUB 接收
    if (CMsgqEMessenger::msgq_send_ == nullptr) {
        CMsgqEMessenger::msgq_send_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PUB, CConfig::instance().SERVERPUB_URL);
        msleep(100);
    }
    // 中继消息发送端：PUB -> SERVERSUB_URL，各引擎 SUB 接收
    if (CMsgqRMessenger::msgq_send_ == nullptr) {
        CMsgqRMessenger::msgq_send_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PUB, CConfig::instance().SERVERSUB_URL);
        msleep(100);
    }
    // 消息中继：PULL 接收策略/服务推送的消息，relay() 时分发到上述两个 PUB
    msg_relay_ = std::make_unique<CMsgqRMessenger>(CConfig::instance().SERVERPULL_URL);
    msleep(100);
}

/** @brief 析构函数：停止所有引擎、等待微服务退出、nn_term、join 线程 */
tradingengine::~tradingengine() {
    for (auto& e : pengines_) {
        while ((e != nullptr) && (e->estate_ != STOP)) {
            e->stop();
            msleep(100);
        }
    }
    while (MICRO_SERVICE_NUMBER > 0) {
        msleep(100);
    }
    if (CConfig::instance()._msgq == MSGQ::NANOMSG)
        nn_term();
    for (thread* t : threads_) {
        if (t->joinable()) {
            t->join();
            delete t;
        }
    }
    LOG_DEBUG(logger, "Exit trading engine");
}

/** @brief 定时任务：注册信号、每秒发送 MSG_TYPE_TIMER、按时间点发送 ENGINE_RESET/SWITCH_TRADING_DAY/ENGINE_CONNECT */
int32_t tradingengine::cronjobs(bool force) {
    // 注册 Ctrl-C、断电等信号处理
    signal(SIGINT, ConsoleControlHandler);
    signal(SIGPWR, ConsoleControlHandler);

    time_t timer;
    struct tm tm_info;

    while (!gShutdown) {
        msleep(1 * 1000);  // 每秒执行一次

        time(&timer);
        localtime_r(&timer, &tm_info);

        // 周日不执行定时任务
        if (tm_info.tm_wday == 0) {
            continue;
        }

        // 每秒：发送 MSG_TYPE_TIMER 给所有引擎
        std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL, "0", MSG_TYPE_TIMER);
        msg_relay_->send(pmsg);

        // 每秒：风控流控计数清零
        RiskManager::instance().resetflow();

        // 2:35 夜盘结束后：引擎重置
        if (tm_info.tm_hour == 2 && tm_info.tm_min == 35 && tm_info.tm_sec == 0) {
            std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_ENGINE_RESET);
            msg_relay_->send(pmsg);
        }
        if (tm_info.tm_wday != 6) {  // 非周六
            // 16:00 日盘结束后：引擎重置
            if (tm_info.tm_hour == 16 && tm_info.tm_min == 0 && tm_info.tm_sec == 0) {
                std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_ENGINE_RESET);
                msg_relay_->send(pmsg);
            }
            // 20:30 日切：切换交易日，重置 TD 引擎，风控日计数清零
            if (tm_info.tm_hour == 20 && tm_info.tm_min == 30 && tm_info.tm_sec == 0) {
                std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_SWITCH_TRADING_DAY);
                msg_relay_->send(pmsg);
                RiskManager::instance().switchday();
            }
            // 8:45 早盘前、13:15 午盘前、20:45 夜盘前：自动连接
            if (tm_info.tm_hour == 8 && tm_info.tm_min == 45 && tm_info.tm_sec == 0) {
                std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_ENGINE_CONNECT);
                msg_relay_->send(pmsg);
            }
            if (tm_info.tm_hour == 13 && tm_info.tm_min == 15 && tm_info.tm_sec == 0) {
                std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_ENGINE_CONNECT);
                msg_relay_->send(pmsg);
            }
            if (tm_info.tm_hour == 20 && tm_info.tm_min == 45 && tm_info.tm_sec == 0) {
                std::shared_ptr<MsgHeader> pmsg = make_shared<MsgHeader>(DESTINATION_ALL,"0", MSG_TYPE_ENGINE_CONNECT);
                msg_relay_->send(pmsg);
            }
        }
    }
    if (force) {
        throw runtime_error("ctrl-c triggered shutdown");
    }
    return 0;
}

/** @brief 主运行：按 mode 启动引擎线程，执行 msg_relay_->relay() 循环 */
int32_t tradingengine::run() {
    if (gShutdown)
        return 1;

    // SIGSEGV 时 sigsetjmp 保存现场，recvSignal 中 siglongjmp 恢复
    g_except_stack.flag = sigsetjmp(g_except_stack.env, 1);
    if (!g_except_stack.isDef()) {
        signal(SIGSEGV, recvSignal);
        try {
            // 异步启动 cronjobs（定时任务循环）
            auto fu1 = async(launch::async, std::bind(&tradingengine::cronjobs, this, std::placeholders::_1), true);

            if (mode == RUN_MODE::RECORD_MODE) {
                LOG_INFO(logger, "RECORD_MODE");
            } else if (mode == RUN_MODE::REPLAY_MODE) {
                LOG_INFO(logger, "REPLAY_MODE");
            } else if (mode == RUN_MODE::TRADE_MODE) {
                LOG_INFO(logger, "TRADE_MODE");
                // 按 _gatewaymap 配置创建并启动各引擎
                for (auto iter = CConfig::instance()._gatewaymap.begin(); iter != CConfig::instance()._gatewaymap.end(); iter++) {
                    if (iter->second.api == "CTP.TD") {
                        std::shared_ptr<IEngine> ctptdengine = make_shared<CtpTDEngine>(iter->first);
                        threads_.push_back(new std::thread(startengine, ctptdengine));
                        pengines_.push_back(ctptdengine);
                    } else if (iter->second.api == "CTP.MD") {
                        std::shared_ptr<IEngine> ctpmdengine = make_shared<CtpMDEngine>();
                        pengines_.push_back(ctpmdengine);
                        threads_.push_back(new std::thread(startengine, ctpmdengine));
                    } else if (iter->second.api == "PAPER.TD") {
                        std::shared_ptr<IEngine> papertdengine = make_shared<PaperTDEngine>();
                        threads_.push_back(new std::thread(startengine, papertdengine));
                        pengines_.push_back(papertdengine);
                    } else if (iter->second.api == "TAP.TD") {
                        // TODO: TAP 交易引擎
                    } else if (iter->second.api == "TAP.MD") {
                        // TODO: TAP 行情引擎
                    } else {
                        LOG_INFO(logger, "API not supported ,ignore it!");
                    }
                }
            } else {
                LOG_ERROR(logger, "Mode doesn't exist,exit.");
                return 1;
            }

            // 可选：设置线程 CPU 亲和性，将引擎线程绑定到不同核
            if (CConfig::instance().cpuaffinity) {
                int32_t num_cpus = std::thread::hardware_concurrency();
                for (int32_t i = 0; i< threads_.size(); i ++) {
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    CPU_SET(i%num_cpus, &cpuset);
                    int32_t rc = pthread_setaffinity_np(threads_[i]->native_handle(),sizeof(cpu_set_t),&cpuset);
                    if (rc != 0) {
                        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
                    }
                }
                // 主线程也绑定到某一核
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(threads_.size()%num_cpus, &cpuset);
                int32_t rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                    if (rc != 0) {
                        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
                }
            }

            // 主循环：从 PULL 接收消息，按目标分发到 PUB（策略或引擎）
            while (!gShutdown) {
                msg_relay_->relay();
            }

            fu1.get();  // 等待 cronjobs 结束
        }
        catch (exception& e) {
            LOG_INFO(logger, e.what());
        }
        catch (...) {
            LOG_ERROR(logger, "StarQuant terminated in error!");
        }
    } else {
        signal(SIGSEGV, SIG_IGN);
        LOG_ERROR(logger, "StarQuant terminated by SEGSEGV!");
        exit(0);
    }
    return 0;
}

/** @brief 是否存活（当前实现返回 gShutdown，即“已请求关闭”时为 true） */
bool tradingengine::live() const {
    return gShutdown == true;
}





}  // namespace StarQuant
