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

#ifndef CPPSRC_STARQUANT_ENGINE_CTPTDENGINE_H_
#define CPPSRC_STARQUANT_ENGINE_CTPTDENGINE_H_
#include <Common/datastruct.h>
#include <Common/config.h>
#include <Engine/IEngine.h>
#include <APIs/Ctp/ThostFtdcTraderApi.h>
#include <time.h>
#include <mutex>
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include <map>

using std::mutex;
using std::string;
using std::list;
using std::vector;

namespace StarQuant {
//////////////////////////////////////////////////////////////////////////
// CtpTDEngine - CTP 交易引擎
//
// 功能概述：
//   对接上期技术 CTP 交易接口（CThostFtdcTraderApi），实现下单、撤单、查询账户/持仓/订单/成交，
//   支持本地条件单（OT_LPT）、结算确认等。
//
// 继承关系：
//   - IEngine：引擎基类（状态管理、init/start/stop/connect/disconnect）
//   - CThostFtdcTraderSpi：CTP 交易回调接口
//
// 状态流转：DISCONNECTED -> CONNECTING -> CONNECT_ACK -> [AUTHENTICATING ->] AUTHENTICATE_ACK
//          -> LOGINING -> [结算确认] -> LOGIN_ACK
//
// 依赖：OrderManager、PortfolioManager、RiskManager、DataManager
//////////////////////////////////////////////////////////////////////////
class CtpTDEngine : public IEngine, CThostFtdcTraderSpi {
 public:
    string name_;                    // 引擎标识，取自网关配置 key（如 CTP.TD.xxx）
    bool needauthentication_;        // 是否需要认证（auth_code 非 NA 时 true）
    bool needsettlementconfirm_;     // 是否需要结算确认
    bool issettleconfirmed_;         // 是否已结算确认
    int32_t m_brokerOrderId_;        // 经纪商订单 ID 递增
    Gateway ctpacc_;                 // CTP 网关配置（td_address、brokerid、userid 等）

    explicit CtpTDEngine(const string& gw);
    ~CtpTDEngine();

    virtual void init();    // 初始化 API、messenger_、logger
    virtual void start();   // 主循环：接收消息并处理下单/撤单/查询等
    virtual void stop();   // 断开连接并置 estate_ 为 STOP
    virtual bool connect();     // 连接 CTP 前置并登录（含认证、结算确认）
    virtual bool disconnect();  // 登出
    void switchday();          // 切换交易日，重置 issettleconfirmed_
    void releaseapi();         // 释放 CTP API 资源
    void reset();              // 断开 -> 释放 -> 重读配置 -> init
    void timertask();          // 定时任务（autoqry 时轮询账户/持仓）
    void processbuf();         // 处理 qryBuffer_、checkLocalOrders

    void insertOrder(shared_ptr<CtpOrderMsg> pmsg);     // 下单（含 OT_LPT 本地条件单）
    void cancelOrder(shared_ptr<OrderActionMsg> pmsg); // 撤单
    void cancelAll(shared_ptr<CancelAllMsg> pmsg);      // 撤单全部
    void closeAll(const string& fullsym = "");         // 平仓全部（预留，未完整实现）
    void checkLocalOrders();   // 检查本地条件单是否触发

    void queryAccount(shared_ptr<MsgHeader> pmsg);
    void queryPosition(shared_ptr<MsgHeader> pmsg);
    void queryContract(shared_ptr<QryContractMsg> pmsg);
    void queryOrder(shared_ptr<MsgHeader> pmsg);
    void queryTrade(shared_ptr<MsgHeader> pmsg);
    void queryPositionDetail(shared_ptr<MsgHeader> pmsg);

 public:
    // ========== CTP 交易回调（CThostFtdcTraderSpi 接口） ==========
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int32_t nReason);
    virtual void OnHeartBeatWarning(int32_t nTimeLapse);
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    ///登出请求响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    ///用户口令更新请求响应
    virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///资金账户口令更新请求响应
    virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///报单录入请求响应
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    ///预埋单录入请求响应
    virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///预埋撤单录入请求响应
    virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///报单操作请求响应
    virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    ///查询最大报单数量响应
    virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///投资者结算结果确认响应
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);
    ///删除预埋单响应
    virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};
    ///删除预埋撤单响应
    virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///执行宣告录入请求响应
    virtual void OnRspExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///执行宣告操作请求响应
    virtual void OnRspExecOrderAction(CThostFtdcInputExecOrderActionField *pInputExecOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///询价录入请求响应
    virtual void OnRspForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///报价录入请求响应
    virtual void OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///报价操作请求响应
    virtual void OnRspQuoteAction(CThostFtdcInputQuoteActionField *pInputQuoteAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///批量报单操作请求响应
    virtual void OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField *pInputBatchOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///申请组合录入请求响应
    virtual void OnRspCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询报单响应
    virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) ;

    ///请求查询成交响应
    virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) ;

    ///请求查询投资者持仓响应
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);

    ///请求查询资金账户响应
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);

    ///请求查询投资者响应
    virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询交易编码响应
    virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询合约保证金率响应
    virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询合约手续费率响应
    virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询交易所响应
    virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询产品响应
    virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询合约响应
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);

    ///请求查询行情响应
    virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询投资者结算结果响应
    virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询转帐银行响应
    virtual void OnRspQryTransferBank(CThostFtdcTransferBankField *pTransferBank, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询投资者持仓明细响应
    virtual void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);

    ///请求查询客户通知响应
    virtual void OnRspQryNotice(CThostFtdcNoticeField *pNotice, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询结算信息确认响应
    virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询投资者持仓明细响应
    virtual void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///查询保证金监管系统经纪公司资金账户密钥响应
    virtual void OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询仓单折抵信息响应
    virtual void OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField *pEWarrantOffset, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询投资者品种/跨品种保证金响应
    virtual void OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询交易所保证金率响应
    virtual void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField *pExchangeMarginRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询交易所调整保证金率响应
    virtual void OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询汇率响应
    virtual void OnRspQryExchangeRate(CThostFtdcExchangeRateField *pExchangeRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询二级代理操作员银期权限响应
    virtual void OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询产品报价汇率
    virtual void OnRspQryProductExchRate(CThostFtdcProductExchRateField *pProductExchRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询产品组
    virtual void OnRspQryProductGroup(CThostFtdcProductGroupField *pProductGroup, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询做市商合约手续费率响应
    virtual void OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField *pMMInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询做市商期权合约手续费响应
    virtual void OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField *pMMOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询报单手续费响应
    virtual void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询期权交易成本响应
    virtual void OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询期权合约手续费响应
    virtual void OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询执行宣告响应
    virtual void OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询询价响应
    virtual void OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询报价响应
    virtual void OnRspQryQuote(CThostFtdcQuoteField *pQuote, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询组合合约安全系数响应
    virtual void OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询申请组合响应
    virtual void OnRspQryCombAction(CThostFtdcCombActionField *pCombAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询转帐流水响应
    virtual void OnRspQryTransferSerial(CThostFtdcTransferSerialField *pTransferSerial, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询银期签约关系响应
    virtual void OnRspQryAccountregister(CThostFtdcAccountregisterField *pAccountregister, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast);   // 错误应答
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);   // 报单通知（核心：更新 OrderManager、发送 OrderStatusMsg）
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);   // 成交通知（核心：gotFill、发送 FillMsg）
    virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);   // 报单录入错误回报
    virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);   // 报单操作错误回报

    ///合约交易状态通知
    virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus) {};

    ///交易所公告通知
    virtual void OnRtnBulletin(CThostFtdcBulletinField *pBulletin) {};

    ///交易通知
    virtual void OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo) {};

    ///提示条件单校验错误
    virtual void OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder) {};

    ///执行宣告通知
    virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) {};

    ///执行宣告录入错误回报
    virtual void OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField *pInputExecOrder, CThostFtdcRspInfoField *pRspInfo) {};

    ///执行宣告操作错误回报
    virtual void OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField *pExecOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

    ///询价录入错误回报
    virtual void OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField *pInputForQuote, CThostFtdcRspInfoField *pRspInfo) {};

    ///报价通知
    virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote) {};

    ///报价录入错误回报
    virtual void OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote, CThostFtdcRspInfoField *pRspInfo) {};

    ///报价操作错误回报
    virtual void OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction, CThostFtdcRspInfoField *pRspInfo) {};

    ///询价通知
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {};

    ///保证金监控中心用户令牌
    virtual void OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken) {};

    ///批量报单操作错误回报
    virtual void OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField *pBatchOrderAction, CThostFtdcRspInfoField *pRspInfo) {};

    ///申请组合通知
    virtual void OnRtnCombAction(CThostFtdcCombActionField *pCombAction) {};

    ///申请组合录入错误回报
    virtual void OnErrRtnCombActionInsert(CThostFtdcInputCombActionField *pInputCombAction, CThostFtdcRspInfoField *pRspInfo) {};

    ///请求查询签约银行响应
    virtual void OnRspQryContractBank(CThostFtdcContractBankField *pContractBank, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询预埋单响应
    virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询预埋撤单响应
    virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询交易通知响应
    virtual void OnRspQryTradingNotice(CThostFtdcTradingNoticeField *pTradingNotice, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询经纪公司交易参数响应
    virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField *pBrokerTradingParams, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询经纪公司交易算法响应
    virtual void OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///请求查询监控中心用户令牌
    virtual void OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///银行发起银行资金转期货通知
    virtual void OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField *pRspTransfer) {};

    ///银行发起期货资金转银行通知
    virtual void OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField *pRspTransfer) {};

    ///银行发起冲正银行转期货通知
    virtual void OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField *pRspRepeal) {};

    ///银行发起冲正期货转银行通知
    virtual void OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField *pRspRepeal) {};

    ///期货发起银行资金转期货通知
    virtual void OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

    ///期货发起期货资金转银行通知
    virtual void OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField *pRspTransfer) {};

    ///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
    virtual void OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

    ///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
    virtual void OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField *pRspRepeal) {};

    ///期货发起查询银行余额通知
    virtual void OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount) {};

    ///期货发起银行资金转期货错误回报
    virtual void OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

    ///期货发起期货资金转银行错误回报
    virtual void OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo) {};

    ///系统运行时期货端手工发起冲正银行转期货错误回报
    virtual void OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

    ///系统运行时期货端手工发起冲正期货转银行错误回报
    virtual void OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo) {};

    ///期货发起查询银行余额错误回报
    virtual void OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo) {};

    ///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
    virtual void OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

    ///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
    virtual void OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField *pRspRepeal) {};

    ///期货发起银行资金转期货应答
    virtual void OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///期货发起期货资金转银行应答
    virtual void OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField *pReqTransfer, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///期货发起查询银行余额应答
    virtual void OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField *pReqQueryAccount, CThostFtdcRspInfoField *pRspInfo, int32_t nRequestID, bool bIsLast) {};

    ///银行发起银期开户通知
    virtual void OnRtnOpenAccountByBank(CThostFtdcOpenAccountField *pOpenAccount) {};

    ///银行发起银期销户通知
    virtual void OnRtnCancelAccountByBank(CThostFtdcCancelAccountField *pCancelAccount) {};

    ///银行发起变更银行账号通知
    virtual void OnRtnChangeAccountByBank(CThostFtdcChangeAccountField *pChangeAccount) {};

 private:
    int32_t reqId_;           // 请求 ID 递增
    int32_t frontID_;         // 前置机编号（OnRspUserLogin 返回）
    int32_t sessionID_;       // 会话编号（与 frontID、orderRef 唯一确定订单）
    int32_t orderRef_;        // 订单引用号（localNo 格式：frontID.sessionID.orderRef）
    CThostFtdcTraderApi* api_;
    bool apiinited_;
    bool inconnectaction_;
    bool autoconnect_;
    bool autoqry_;            // 定时自动查询账户/持仓

    map<string, std::shared_ptr<Position> > posbuffer_;   // 查询持仓时的临时缓冲
    std::queue<std::shared_ptr<MsgHeader> > qryBuffer_;   // 查询请求队列（限流 <1s/次）
    std::vector<shared_ptr<CtpOrderMsg> > localorders_;   // 本地条件单（OT_LPT）待触发列表
    uint64_t lastQryTime_;    // 上次查询时间（微秒）
    int32_t timercount_;      // 定时计数
    int32_t msgqMode_;        // messenger_->recv 的 mode（0=阻塞）

    OrderStatus CtpOrderStatusToOrderStatus(const char status);   // CTP 状态 -> OrderStatus
    OrderFlag CtpComboOffsetFlagToOrderFlag(const char flag);     // CTP 开平标志 -> OrderFlag
    char OrderFlagToCtpComboOffsetFlag(const OrderFlag flag);     // OrderFlag -> CTP 开平标志
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_ENGINE_CTPTDENGINE_H_
