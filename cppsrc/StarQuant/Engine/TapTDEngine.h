#ifndef _StarQuant_Engine_TapTDEngine_H_
#define _StarQuant_Engine_TapTDEngine_H_

#include <mutex>
#include <Common/config.h>
#include <Engine/IEngine.h>
#include <APIs/Tap/TapTradeAPI.h>
#include <APIs/Tap/TapAPIError.h>
#include <Trade/order.h>
#include <Trade/fill.h>

using std::mutex;
using std::string;
using std::list;
using std::vector;

namespace StarQuant
{
struct Security;

//////////////////////////////////////////////////////////////////////////
// TapTDEngine - TAP 交易引擎
//
// 功能概述：
//   对接易盛 TAP 交易接口（TapTradeAPI），实现下单、撤单、查询账户/持仓/订单/成交，
//   接收报单通知、成交通知、持仓变动等回调。
//
// 继承关系：
//   - IEngine：引擎基类
//   - ITapTradeAPINotify：TAP 交易回调接口
//
// 状态流转：DISCONNECTED -> CONNECTING -> CONNECT_ACK(OnConnect) -> LOGINING
//          -> LOGIN_ACK(OnAPIReady)
//
// 消息通道：msgq_recv_(SUB) 接收指令，IEngine::msgq_send_(PUB) 发送回报
// 配置：CConfig::_accmap["TAP"]，需 td_ip、td_port、auth_code 等
//////////////////////////////////////////////////////////////////////////
class TapTDEngine : public IEngine, ITapTradeAPINotify {
public:
    string name_;      // 引擎标识，默认 "TAP_TD"
    Gateway tapacc_;    // TAP 账户配置（td_ip、td_port、auth_code 等）

    TapTDEngine();
    ~TapTDEngine();

    virtual void init();    // 创建 msgq_recv_、TapTradeAPI 实例
    virtual void start();   // 主循环：接收消息并处理下单/撤单/查询等
    virtual void stop();    // 断开、等待 DISCONNECTED、释放 API
    virtual bool connect();     // 连接 TAP 交易并登录
    virtual bool disconnect();  // 调用 api_->Disconnect

    void insertOrder(const vector<string>& msgv);   // 下单，msgv 为序列化消息按 '|' 分割后的向量
    void cancelOrder(const vector<string>& msgv);   // 撤单（按 clientOrderId 查 Order）
    void cancelOrder(long oid);                     // 撤单（按 serverOrderId）
    void cancelOrders(const string & ono);         // 撤单（按 orderNo，预留）
    void queryAccount();    // 查询账户与资金
    void queryOrder(const vector<string>& msgorder_);  // 查询订单
    void queryPosition();   // 查询持仓

public:
    // ========== TAP 交易回调（ITapTradeAPINotify 接口） ==========
    virtual void TAP_CDECL OnConnect();   // 连接成功，置 estate_=CONNECT_ACK
    virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPITradeLoginRspInfo *loginRspInfo);  // 登录响应
    virtual void TAP_CDECL OnAPIReady();   // API 就绪，置 estate_=LOGIN_ACK
    virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);   // 断开连接
    virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode);
    virtual void TAP_CDECL OnRspSetReservedInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, const TAPISTR_50 info);
    virtual void TAP_CDECL OnRspQryAccount(TAPIUINT32 sessionID, TAPIUINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountInfo *info);
    virtual void TAP_CDECL OnRspQryFund(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFundData *info);  // 查询资金响应
    virtual void TAP_CDECL OnRtnFund(const TapAPIFundData *info);  // 资金推送
    virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeInfo *info);
    virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICommodityInfo *info);
    virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPITradeContractInfo *info);
    virtual void TAP_CDECL OnRtnContract(const TapAPITradeContractInfo *info);
    virtual void TAP_CDECL OnRtnOrder(const TapAPIOrderInfoNotice *info);  // 报单状态回报
    virtual void TAP_CDECL OnRspOrderAction(TAPIUINT32 sessionID, TAPIUINT32 errorCode, const TapAPIOrderActionRsp *info);
    virtual void TAP_CDECL OnRspQryOrder(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info);
    virtual void TAP_CDECL OnRspQryOrderProcess(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIOrderInfo *info);
    virtual void TAP_CDECL OnRspQryFill(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIFillInfo *info);
    virtual void TAP_CDECL OnRtnFill(const TapAPIFillInfo *info);  // 成交通知
    virtual void TAP_CDECL OnRspQryPosition(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIPositionInfo *info);  // 查询持仓响应
    virtual void TAP_CDECL OnRtnPosition(const TapAPIPositionInfo *info);  // 持仓推送
    virtual void TAP_CDECL OnRspQryClose(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPICloseInfo *info);
    virtual void TAP_CDECL OnRtnClose(const TapAPICloseInfo *info);  // 平仓回报
    virtual void TAP_CDECL OnRtnPositionProfit(const TapAPIPositionProfitNotice *info);  // 持仓盈亏推送
    virtual void TAP_CDECL OnRspQryDeepQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIDeepQuoteQryRsp *info);
    virtual void TAP_CDECL OnRspQryExchangeStateInfo(TAPIUINT32 sessionID,TAPIINT32 errorCode, TAPIYNFLAG isLast,const TapAPIExchangeStateInfo * info);
    virtual void TAP_CDECL OnRtnExchangeStateInfo(const TapAPIExchangeStateInfoNotice * info);
    virtual void TAP_CDECL OnRtnReqQuoteNotice(const TapAPIReqQuoteNotice *info);
    virtual void TAP_CDECL OnRspUpperChannelInfo(TAPIUINT32 sessionID,TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIUpperChannelInfo * info);
    virtual void TAP_CDECL OnRspAccountRentInfo(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIAccountRentInfo * info);

private:
    long m_brokerOrderId_;       // 经纪商订单 ID 自增序列
    unsigned int32_t sessionID_; // TAP 会话编号，用于下单/撤单/查询

    ITapTradeAPI* api_;          // TAP 交易 API 实例

    OrderStatus TapOrderStatusToOrderStatus(const TAPIOrderStateType);  // TAP 订单状态 -> OrderStatus
    OrderFlag TapPositionEffectToOrderFlag(const TAPIPositionEffectType flag);  // TAP 开平 -> OrderFlag
    OrderType TapOrderTypeToOrderType(const TAPIOrderTypeType type);  // TAP 订单类型 -> OrderType
    TAPIPositionEffectType OrderFlagToTapPositionEffect(const OrderFlag flag);  // OrderFlag -> TAP 开平
    TAPIOrderTypeType OrderTypeToTapOrderType(const OrderType type);  // OrderType -> TAP 订单类型
};
}

#endif
