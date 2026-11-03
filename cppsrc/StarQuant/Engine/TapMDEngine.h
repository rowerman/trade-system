#ifndef _StarQuant_Engine_TapMDEngine_H_
#define _StarQuant_Engine_TapMDEngine_H_

#include <mutex>
#include <Common/config.h>
#include <Engine/IEngine.h>
#include <APIs/Tap/TapQuoteAPI.h>
#include <APIs/Tap/TapAPIError.h>

using std::mutex;
using std::string;
using std::list;
using std::vector;

namespace StarQuant
{
//////////////////////////////////////////////////////////////////////////
// TapMDEngine - TAP 行情引擎
//
// 功能概述：
//   对接易盛 TAP 行情接口（TapQuoteAPI），实现行情订阅、5 档深度数据接收，
//   并将 Tick 数据通过 IEngine::msgq_send_ 转发至订阅者。
//
// 继承关系：
//   - IEngine：引擎基类
//   - ITapQuoteAPINotify：TAP 行情回调接口
//
// 状态流转：DISCONNECTED -> CONNECTING -> CONNECT_ACK(OnRspLogin) -> LOGINING
//          -> LOGIN_ACK(OnAPIReady)
//
// 消息通道：使用 msgq_recv_(SUB) 接收指令，IEngine::msgq_send_(PUB) 发送行情
// 配置：CConfig::_accmap["TAP"]，需 md_ip、md_port、auth_code 等
//////////////////////////////////////////////////////////////////////////
class TapMDEngine : public IEngine, ITapQuoteAPINotify {
public:
    string name_;      // 引擎标识，默认 "TAP_MD"
    Gateway tapacc_;   // TAP 账户配置（md_ip、md_port、auth_code 等）

    TapMDEngine();
    ~TapMDEngine();

    virtual void init();    // 创建 msgq_recv_、TapQuoteAPI 实例
    virtual void start();   // 主循环：接收消息并处理连接/订阅/查询等
    virtual void stop();    // 断开、等待 DISCONNECTED、释放 API
    virtual bool connect();     // 连接 TAP 行情并登录
    virtual bool disconnect();  // 调用 api_->Disconnect

    void subscribe(const string& symbol);   // 订阅行情，symbol 格式 "交易所 类型 品种 合约号"
    void unsubscribe(const string& symbol); // 取消订阅
    void query(const MSG_TYPE & _type, const string& symbol);  // 查询合约/品种（MSG_TYPE_QRY_CONTRACT 等，当前未完整实现）

public:
    // ========== TAP 行情回调（ITapQuoteAPINotify 接口） ==========
    virtual void TAP_CDECL OnRspLogin(TAPIINT32 errorCode, const TapAPIQuotLoginRspInfo *info);  // 登录响应
    virtual void TAP_CDECL OnAPIReady();   // API 就绪，置 estate_=LOGIN_ACK
    virtual void TAP_CDECL OnDisconnect(TAPIINT32 reasonCode);  // 断开回调
    virtual void TAP_CDECL OnRspChangePassword(TAPIUINT32 sessionID, TAPIINT32 errorCode);
    virtual void TAP_CDECL OnRspQryExchange(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIExchangeInfo *info);
    virtual void TAP_CDECL OnRspQryCommodity(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteCommodityInfo *info);
    virtual void TAP_CDECL OnRspQryContract(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteContractInfo *info);
    virtual void TAP_CDECL OnRtnContract(const TapAPIQuoteContractInfo *info);
    virtual void TAP_CDECL OnRspSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIQuoteWhole *info);   // 订阅行情响应
    virtual void TAP_CDECL OnRspUnSubscribeQuote(TAPIUINT32 sessionID, TAPIINT32 errorCode, TAPIYNFLAG isLast, const TapAPIContract *info);
    virtual void TAP_CDECL OnRtnQuote(const TapAPIQuoteWhole *info);   // 行情推送（核心：解析为 Tick_L5 并发送）

private:
    uint32_t sessionId_;       // TAP 会话 ID
    ITapQuoteAPI* api_;        // TAP 行情 API 实例
    TapAPIContract stContract_;  // 当前订阅的合约（subscribe/unsubscribe 用）

};
}
template<size_t size> inline void APIStrncpy(char (&Dst)[size], const char* source)
{
#ifdef _WIN32
    strncpy_s(Dst, source, sizeof(Dst) - 1);
#else  
    strncpy(Dst, source, sizeof(Dst));
    //Dst[size] = '\0';
#endif
}
#endif
