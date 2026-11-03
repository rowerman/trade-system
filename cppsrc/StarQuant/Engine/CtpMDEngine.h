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

#ifndef CPPSRC_STARQUANT_ENGINE_CTPMDENGINE_H_
#define CPPSRC_STARQUANT_ENGINE_CTPMDENGINE_H_

#include <Common/datastruct.h>
#include <Engine/IEngine.h>
#include <APIs/Ctp/ThostFtdcMdApi.h>
#include <mutex>
#include <string>
#include <vector>

using std::mutex;
using std::string;
using std::vector;

namespace StarQuant
{
//////////////////////////////////////////////////////////////////////////
// CtpMDEngine - CTP 行情引擎
//
// 功能概述：
//   对接上期技术 CTP 行情接口（CThostFtdcMdApi），实现行情订阅、深度数据接收，
//   并将 Tick 数据通过 IMessenger 转发至策略/DataBoard 等订阅者。
//
// 继承关系：
//   - IEngine：引擎基类（状态管理、init/start/stop/connect/disconnect）
//   - CThostFtdcMdSpi：CTP 行情回调接口（OnFrontConnected、OnRtnDepthMarketData 等）
//
// 状态流转：DISCONNECTED -> CONNECTING -> CONNECT_ACK -> LOGINING -> LOGIN_ACK
// 依赖：Common/config（Gateway、SERVERSUB_URL）、Data/DataManager（ctp2Full_、full2Ctp_）
//////////////////////////////////////////////////////////////////////////
class CtpMDEngine : public IEngine, CThostFtdcMdSpi {
 public:
    string name_;      // 引擎标识，默认 "CTP.MD"
    Gateway ctpacc_;  // CTP 网关配置（md_address、brokerid 等）

    CtpMDEngine();
    ~CtpMDEngine();

    virtual void init();    // 初始化 API、messenger_、logger，创建 CThostFtdcMdApi 实例
    virtual void start();   // 主循环：接收消息并处理连接/订阅/状态查询等指令
    virtual void stop();   // 断开连接并置 estate_ 为 STOP

    virtual bool connect();     // 连接 CTP 前置并登录
    virtual bool disconnect();  // 登出并断开
    void releaseapi();         // 释放 CTP API 资源（RegisterSpi(nullptr)、Release）
    void reset();              // 断开 -> 释放 API -> 重读配置 -> init
    void switchday() {}        // 切换交易日（预留，空实现）
    void timertask();          // 定时任务（预留，当前仅递增 timercount_）
    void processbuf();         // 处理缓冲区/杂项（如 DataManager 合约文件保存）

    void subscribe(const vector<string>& symbols, SymbolType st = ST_Ctp);    // 订阅行情
    void unsubscribe(const vector<string>& symbols, SymbolType st = ST_Ctp);  // 取消订阅

 public:
    // ========== CTP 行情回调（CThostFtdcMdSpi 接口） ==========
    virtual void OnFrontConnected();   // 前置连接成功
    virtual void OnFrontDisconnected(int32_t nReason);  // 前置断开
    virtual void OnHeartBeatWarning(int32_t nTimeLapse);  // 心跳超时告警
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 登录响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 登出响应
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 错误应答
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 订阅行情响应
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 取消订阅行情响应
    virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 订阅询价响应
    virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 取消订阅询价响应
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);   // 深度行情推送（核心：解析为 TickMsg 并转发）
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);   // 询价通知

 private:
    int32_t loginReqId_;       // 登录请求计数（用于 OnFrontDisconnected 节流）
    int32_t reqId_;           // 请求 ID 递增
    CThostFtdcMdApi* api_;    // CTP 行情 API 实例
    bool apiinited_;          // API 是否已 Init
    bool inconnectaction_;    // 是否处于 connect/disconnect 流程中
    bool autoconnect_;        // 前置连接成功后是否自动发起 connect
    vector<string> lastsubs_;  // 上次订阅列表，登录成功后自动重订
    int32_t timercount_;      // 定时计数（预留）
    int32_t msgqMode_;        // messenger_->recv 的 mode 参数（0=阻塞等）
};
}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_ENGINE_CTPMDENGINE_H_
