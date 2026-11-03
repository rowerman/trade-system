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

#ifndef CPPSRC_STARQUANT_COMMON_DATASTRUCT_H_
#define CPPSRC_STARQUANT_COMMON_DATASTRUCT_H_

#include <APIs/Ctp/ThostFtdcUserApiDataType.h>
#include <APIs/Ctp/ThostFtdcUserApiStruct.h>
#include <string>
#include <iostream>
#include <mutex>
#include <memory>
#include <vector>
#include <atomic>

using namespace std;


#if defined(_WIN32) || defined(_WIN64)
#ifdef DLL_EXPORT
#define DLL_EXPORT_IMPORT  __declspec(dllexport)   // export DLL information
#else
#define DLL_EXPORT_IMPORT  __declspec(dllimport)   // import DLL information
#endif
#else
#define DLL_EXPORT_IMPORT
#endif

namespace StarQuant {

// macro definations used

#define SERIALIZATION_SEPARATOR '|'
#define RELAY_DESTINATION '@'
#define DESTINATION_ALL "*"
#define DESTINATION_SEPARATOR '.'
#define COMMODITY_SEPARATOR '&'

enum CurrencyType {
    USD,
    AUD,
    CAD,
    CHF,
    EUR,
    GBP,
    HKD,
    JPY,
    MXN,
    NZD,
    SEK
};
enum SymbolType {
    ST_Full = 0,
    ST_Ctp = 1
};
const std::string CurrencyTypeString[] = {
    "USD",
    "AUD",
    "CAD",
    "CHF",
    "EUR",
    "GBP",
    "HKD",
    "JPY",
    "MXN",
    "NZD",
    "SEK"
};

enum OrderType {
    OT_Market = 0,              // 市价单 - 市价类：以当前市场价格立即成交
    OT_MarketOnClose,           // 收盘市价单 - 市价类：在收盘时以市价成交
    OT_Limit = 2,               // 限价单 - 限价类：指定价格，仅在该价格或更优时成交
    OT_LimitOnClose,            // 收盘限价单 - 限价类：在收盘时以指定限价成交
    OT_PeggedToMarket,          // 跟市价单 - 市价类：价格随市场变动而调整的市价单
    OT_Stop = 5,                // 止损单 - 止损类：价格触及止损价后转为市价单
    OT_StopLimit = 6,           // 止损限价单 - 止损类：触及止损价后转为限价单
    OT_TrailingStop = 7,        // 追踪止损单 - 止损类：止损价随有利方向移动
    OT_Relative,                // 相对订单 - 相对订单类：相对于参考价格下单
    OT_VolumeWeightedAveragePrice,  // VWAP订单 - 算法订单类：按成交量加权平均价执行
    OT_TrailingStopLimit,       // 追踪止损限价单 - 止损类：追踪止损+限价组合
    OT_Volatility,              // 波动率订单 - 波动率类：基于波动率定价的订单
    OT_None,                    // 无类型 - 系统/占位类：未指定订单类型
    OT_Empty,                   // 空类型 - 系统/占位类：空占位符
    OT_Default,                 // 默认类型 - 系统/占位类：使用默认订单类型
    OT_Scale,                   // 分段订单 - 算法订单类：分批按比例执行
    OT_MarketIfTouched,         // 触及市价单 - 条件触发类：价格触及后以市价成交
    OT_LimitIfTouched,          // 触及限价单 - 条件触发类：价格触及后以限价成交
    OT_OptExec,                 // 期权执行 - 期权类：期权行权
    OT_OptAbandon,              // 期权放弃 - 期权类：期权放弃行权
    OT_Reqquot,                 // 请求报价 - 报价类：请求做市商报价
    OT_Rspquot,                 // 响应报价 - 报价类：做市商响应报价
    OT_Swap,                    // 互换 - 互换类：互换合约相关订单
    OT_FAK,                     // Fill And Kill - 成交方式类：部分成交即可，未成交部分立即取消
    OT_FOK,                     // Fill Or Kill - 成交方式类：全部成交或全部取消
    OT_LPT                      // 本地价格条件触及 - 条件触发类：本地价格条件满足时触发
};

enum OrderStatus {
    OS_UNKNOWN = 0,         // 未知 - 初始/异常类：状态未识别
    OS_NewBorn = 1,         // 新建 - 初始/创建类：订单已创建待提交
    OS_PendingSubmit = 2,   // 待提交 - 提交类：等待提交至交易所
    OS_Submitted = 3,       // 已提交 - 提交类：已发送至交易所
    OS_Acknowledged = 4,    // 已确认 - 提交类：交易所已确认接收
    OS_Queued = 5,          // 排队中 - 提交类：在交易所排队等待撮合
    OS_PartiallyFilled = 6,  // 部分成交 - 成交类：部分数量已成交
    OS_Filled = 7,          // 全部成交 - 成交类：订单已完全成交
    OS_PendingCancel = 8,   // 待取消 - 取消类：取消请求已发送待确认
    OS_PendingModify = 9,   // 待修改 - 修改类：修改请求已发送待确认
    OS_Canceled = 10,       // 已取消 - 取消类：订单已被取消
    OS_LeftDelete = 11,     // 左侧删除 - 取消类：做市商左侧报价已删除
    OS_Suspended = 12,      // 已暂停 - 异常类：订单被暂停
    OS_ApiPending = 13,     // API待处理 - API类：API层待处理状态
    OS_ApiCancelled = 14,   // API已取消 - 取消类：API层已取消
    OS_Fail = 15,           // 指令失败 - 异常类：下单或指令执行失败
    OS_Deleted = 16,        // 已删除 - 取消类：订单已被删除
    OS_Effect = 17,         // 已生效 - 询价类：询价成功已生效
    OS_Apply = 18,          // 已申请 - 期权类：行权、弃权等申请成功
    OS_Error = 19,          // 错误 - 异常类：发生错误
    OS_Trig = 20,           // 待触发 - 触发类：条件单等待触发
    OS_Exctrig = 21         // 已触发 - 触发类：条件已触发待执行
};

enum OrderFlag {  // for CTP,tap offset flag - 期货开平仓标志
    OF_None = -1,           // 无 - 占位类：未指定开平方向
    OF_OpenPosition = 0,    // 开仓 - 开仓类：开新仓位
    OF_ClosePosition = 1,   // 平仓 - 平仓类：平仓（不区分今昨）
    OF_CloseToday = 2,     // 平今 - 平仓类：平掉当日开仓
    OF_CloseYesterday = 3, // 平昨 - 平仓类：平掉历史持仓
    OF_ForceClose = 4,     // 强平 - 强制类：交易所强制平仓
    OF_ForceOff = 5,       // 强减 - 强制类：强制减仓
    OF_LocalForceClose = 6 // 本地强平 - 强制类：本地风控强制平仓
};

const std::string OrderStatusString[] = {
    "Unknown",
    "NewBorn",
    "PendingSubmit",
    "Submitted",
    "Acknowledged",
    "Queued",
    "PartiallyFilled",
    "Filled",
    "PendingCancel",
    "PendingModify",
    "Canceled",
    "LeftDelete",
    "Suspended",
    "ApiPending",
    "ApiCancelled",
    "Fail",
    "Deleted",
    "Effect",
    "Apply",
    "Error",
    "Trigerrring",
    "ExcTriggering",
    "None"
};

enum class MSGQ : uint8_t {
    NANOMSG = 0,    // 消息队列实现类：nanomsg 轻量级消息库
    ZMQ,            // 消息队列实现类：ZeroMQ 消息库
    KAFKA,          // 消息队列实现类：Apache Kafka 分布式流平台
    WEBSOCKET       // 消息队列实现类：WebSocket 全双工通信
};

enum class MSGQ_PROTOCOL : uint8_t {
    PAIR = 0,   // 通信模式类：一对一双向通信
    REQ,        // 通信模式类：请求端（客户端）
    REP,        // 通信模式类：应答端（服务端）
    PUB,        // 通信模式类：发布端
    SUB,        // 通信模式类：订阅端
    PUSH,       // 通信模式类：推送端（任务分发）
    PULL        // 通信模式类：拉取端（工作节点）
};
// ------msg type for interprocess communication-----
enum MSG_TYPE : int32_t {
  // 10xx 行情数据类
    MSG_TYPE_TICK = 1000,           // 期货行情类：基础 tick 数据
    MSG_TYPE_TICK_L1 = 1001,        // 期货行情类：1 档深度 tick
    MSG_TYPE_TICK_L5 = 1002,        // 期货行情类：5 档深度 tick
    MSG_TYPE_TICK_L10 = 1003,       // 期货行情类：10 档深度 tick
    MSG_TYPE_TICK_L20 = 1004,       // 期货行情类：20 档深度 tick
    MSG_TYPE_BAR = 1010,            // 期货K线类：通用 bar
    MSG_TYPE_BAR_1MIN = 1011,       // 期货K线类：1 分钟 K 线
    MSG_TYPE_BAR_5MIN = 1012,       // 期货K线类：5 分钟 K 线
    MSG_TYPE_BAR_15MIN = 1013,      // 期货K线类：15 分钟 K 线
    MSG_TYPE_BAR_1HOUR = 1014,      // 期货K线类：1 小时 K 线
    MSG_TYPE_BAR_1DAY = 1015,       // 期货K线类：日 K 线
    MSG_TYPE_BAR_1WEEK = 1016,      // 期货K线类：周 K 线
    MSG_TYPE_BAR_1MON = 1017,       // 期货K线类：月 K 线
    MSG_TYPE_STOCK_TICK = 1020,     // 股票行情类：股票 tick
    MSG_TYPE_STOCK_BAR = 1021,      // 股票行情类：股票 K 线
    MSG_TYPE_Trade = 1060,          // 行情字段类：成交
    MSG_TYPE_Bid = 1061,            // 行情字段类：买价
    MSG_TYPE_Ask = 1062,            // 行情字段类：卖价
    MSG_TYPE_Full = 1063,           // 行情字段类：全量快照
    MSG_TYPE_BidPrice = 1064,       // 行情字段类：买一价
    MSG_TYPE_BidSize = 1065,        // 行情字段类：买一量
    MSG_TYPE_AskPrice = 1066,       // 行情字段类：卖一价
    MSG_TYPE_AskSize = 1067,        // 行情字段类：卖一量
    MSG_TYPE_TradePrice = 1068,     // 行情字段类：成交价
    MSG_TYPE_TradeSize = 1069,      // 行情字段类：成交量
    MSG_TYPE_OpenPrice = 1070,      // 行情字段类：开盘价
    MSG_TYPE_HighPrice = 1071,      // 行情字段类：最高价
    MSG_TYPE_LowPrice = 1072,       // 行情字段类：最低价
    MSG_TYPE_ClosePrice = 1073,     // 行情字段类：收盘价
    MSG_TYPE_Volume = 1074,         // 行情字段类：成交量
    MSG_TYPE_OpenInterest = 1075,   // 行情字段类：持仓量（期货）
    MSG_TYPE_Hist = 1076,          // 行情字段类：历史数据
  // 11xx 引擎控制类
    MSG_TYPE_ENGINE_STATUS = 1101,      // 引擎控制类：查询引擎状态
    MSG_TYPE_ENGINE_START = 1111,       // 引擎控制类：启动引擎
    MSG_TYPE_ENGINE_STOP = 1112,        // 引擎控制类：停止引擎
    MSG_TYPE_ENGINE_RESET = 1113,       // 引擎控制类：重置引擎
    MSG_TYPE_ENGINE_CONNECT = 1120,     // 引擎控制类：连接
    MSG_TYPE_ENGINE_DISCONNECT = 1121,  // 引擎控制类：断开连接
    MSG_TYPE_SWITCH_TRADING_DAY = 1141, // 引擎控制类：切换交易日
  // 12xx 策略管理类
    MSG_TYPE_STRATEGY_STATUS = 1200,            // 策略管理类：策略状态
    MSG_TYPE_STRATEGY_ADD = 1210,               // 策略管理类：添加策略
    MSG_TYPE_STRATEGY_INIT = 1211,              // 策略管理类：初始化策略
    MSG_TYPE_STRATEGY_INIT_ALL = 1212,          // 策略管理类：初始化全部策略
    MSG_TYPE_STRATEGY_START = 1213,             // 策略管理类：启动策略
    MSG_TYPE_STRATEGY_START_ALL = 1214,         // 策略管理类：启动全部策略
    MSG_TYPE_STRATEGY_STOP = 1215,              // 策略管理类：停止策略
    MSG_TYPE_STRATEGY_STOP_ALL = 1216,          // 策略管理类：停止全部策略
    MSG_TYPE_STRATEGY_EDIT = 1220,              // 策略管理类：编辑策略
    MSG_TYPE_STRATEGY_REMOVE = 1221,            // 策略管理类：移除策略
    MSG_TYPE_STRATEGY_REMOVE_DUPLICATE = 1222,  // 策略管理类：移除重复策略
    MSG_TYPE_STRATEGY_RTN_REMOVE = 1223,        // 策略管理类：移除策略回报
    MSG_TYPE_STRATEGY_GET_DATA = 1230,          // 策略管理类：获取数据请求
    MSG_TYPE_STRATEGY_RTN_DATA = 1231,          // 策略管理类：数据回报
    MSG_TYPE_STRATEGY_GET_CLASS_NAME = 1232,    // 策略管理类：获取策略类名请求
    MSG_TYPE_STRATEGY_RTN_CLASS_NAME = 1233,    // 策略管理类：类名回报
    MSG_TYPE_STRATEGY_GET_CLASS_PARAMETERS = 1234,   // 策略管理类：获取类参数请求
    MSG_TYPE_STRATEGY_RTN_CLASS_PARAMETERS = 1235,   // 策略管理类：类参数回报
    MSG_TYPE_STRATEGY_GET_PARAMETERS = 1234,    // 策略管理类：获取参数请求
    MSG_TYPE_STRATEGY_RTN_PARAMETERS = 1235,    // 策略管理类：参数回报
  // 13xx 任务类
    MSG_TYPE_TIMER = 1301,         // 任务类：定时器触发
    MSG_TYPE_TASK_START = 1310,    // 任务类：启动任务
    MSG_TYPE_TASK_STOP = 1311,     // 任务类：停止任务
  // 20xx 引擎请求类
    MSG_TYPE_SUBSCRIBE_MARKET_DATA = 2001,   // 行情请求类：订阅行情
    MSG_TYPE_SUBSCRIBE_L2_MD = 2002,         // 行情请求类：订阅 L2 深度行情
    MSG_TYPE_SUBSCRIBE_INDEX = 2003,         // 行情请求类：订阅指数
    MSG_TYPE_SUBSCRIBE_ORDER_TRADE = 2004,   // 行情请求类：订阅成交回报
    MSG_TYPE_UNSUBSCRIBE = 2011,             // 行情请求类：取消订阅
    MSG_TYPE_QRY_COMMODITY = 2021,           // 交易请求类：查询品种
    MSG_TYPE_QRY_CONTRACT = 2022,           // 交易请求类：查询合约
    MSG_TYPE_QRY_POS = 2023,                 // 交易请求类：查询持仓
    MSG_TYPE_QRY_ACCOUNT = 2024,             // 交易请求类：查询账户
    MSG_TYPE_QRY_ORDER = 2025,               // 交易请求类：查询订单
    MSG_TYPE_QRY_TRADE = 2026,               // 交易请求类：查询成交
    MSG_TYPE_QRY_POSDETAIL = 2027,           // 交易请求类：查询持仓明细
    MSG_TYPE_ORDER = 2030,                   // 交易请求类：下单（通用）
    MSG_TYPE_ORDER_PAPER = 2031,             // 交易请求类：模拟盘下单
    MSG_TYPE_ORDER_CTP = 2032,               // 交易请求类：CTP 下单
    MSG_TYPE_ORDER_CTP_PARKED = 2033,        // 交易请求类：CTP 预埋单
    MSG_TYPE_ORDER_TAP = 2034,                // 交易请求类：TAP 下单
    MSG_TYPE_ORDER_XTP = 2035,               // 交易请求类：XTP 下单
    MSG_TYPE_ORDER_ACTION = 2040,            // 交易请求类：撤单（通用）
    MSG_TYPE_CANCEL_ORDER = 2041,            // 交易请求类：撤销指定订单
    MSG_TYPE_CANCEL_ALL = 2042,              // 交易请求类：撤销全部订单
    MSG_TYPE_ORDER_ACTION_CTP = 2043,         // 交易请求类：CTP 撤单
    MSG_TYPE_ORDER_ACTION_TAP = 2044,         // 交易请求类：TAP 撤单
    MSG_TYPE_ORDER_ACTION_XTP = 2045,         // 交易请求类：XTP 撤单
  // 25xx 引擎回报类
    MSG_TYPE_RSP_POS = 2500,           // 交易回报类：持仓查询响应
    MSG_TYPE_RTN_ORDER = 2510,        // 交易回报类：订单状态回报（通用）
    MSG_TYPE_RTN_ORDER_CTP = 2511,    // 交易回报类：CTP 订单回报
    MSG_TYPE_RTN_ORDER_TAP = 2512,    // 交易回报类：TAP 订单回报
    MSG_TYPE_RTN_ORDER_XTP = 2513,    // 交易回报类：XTP 订单回报
    MSG_TYPE_RTN_TRADE = 2520,        // 交易回报类：成交回报（通用）
    MSG_TYPE_RTN_TRADE_CTP = 2521,    // 交易回报类：CTP 成交回报
    MSG_TYPE_RTN_TRADE_TAP = 2522,    // 交易回报类：TAP 成交回报
    MSG_TYPE_RTN_TRADE_XTP = 2523,    // 交易回报类：XTP 成交回报
    MSG_TYPE_RSP_ACCOUNT = 2530,      // 交易回报类：账户查询响应
    MSG_TYPE_RSP_CONTRACT = 2540,     // 交易回报类：合约查询响应
    MSG_TYPE_RSP_COMMODITY = 2541,    // 交易回报类：品种查询响应
  // 31xx 系统信息类
    MSG_TYPE_INFO = 3100,                        // 系统信息类：通用信息
    MSG_TYPE_INFO_ENGINE_MDCONNECTED = 3101,     // 系统信息类：行情已连接
    MSG_TYPE_INFO_ENGINE_MDDISCONNECTED = 3102,  // 系统信息类：行情已断开
    MSG_TYPE_INFO_ENGINE_TDCONNECTED = 3103,     // 系统信息类：交易已连接
    MSG_TYPE_INFO_ENGINE_TDDISCONNECTED = 3104,  // 系统信息类：交易已断开
    MSG_TYPE_INFO_HEARTBEAT_WARNING = 3105,      // 系统信息类：心跳超时告警
    MSG_TYPE_INFO_ENGINE_STATUS = 3106,          // 系统信息类：引擎状态信息
  // 34xx 错误类
    MSG_TYPE_ERROR = 3400,                  // 错误类：通用错误
    MSG_TYPE_ERROR_ENGINENOTCONNECTED = 3401,  // 错误类：引擎未连接
    MSG_TYPE_ERROR_SUBSCRIBE = 3402,        // 错误类：订阅失败
    MSG_TYPE_ERROR_INSERTORDER = 3403,      // 错误类：下单失败
    MSG_TYPE_ERROR_CANCELORDER = 3404,      // 错误类：撤单失败
    MSG_TYPE_ERROR_ORGANORDER = 3405,      // 错误类：订单未被订单管理器跟踪
    MSG_TYPE_ERROR_QRY_ACC = 3406,          // 错误类：查询账户失败
    MSG_TYPE_ERROR_QRY_POS = 3407,          // 错误类：查询持仓失败
    MSG_TYPE_ERROR_QRY_CONTRACT = 3408,     // 错误类：查询合约失败
    MSG_TYPE_ERROR_CONNECT = 3409,          // 错误类：连接/登录失败
    MSG_TYPE_ERROR_DISCONNECT = 3410,       // 错误类：断开连接失败
    MSG_TYPE_ERROR_NOACCOUNT = 3411,        // 错误类：无有效账户
  // 40xx 测试类
    MSG_TYPE_TEST = 4000,   // 测试类：测试用消息
  // 9 基础类
    MSG_TYPE_BASE = 9       // 基础类：消息头/基类
};

MSG_TYPE MsgType(const string& str);
string accAddress(const string& str);

enum class RUN_MODE :uint8_t {
    TRADE_MODE = 0, RECORD_MODE, REPLAY_MODE
};

enum class BROKERS : uint8_t {
    IB = 0, CTP, GOOGLE, SINA, PAPER, TAP
};



struct Gateway {  // 交易网关配置
    string id;                    // 网关唯一标识
    string api;                   // API 类型（如 CTP、TAP、XTP）
    int32_t intid;                // 网关内部 ID
    string brokerid;              // 经纪商/期货公司代码
    string userid;                // 交易账号
    string password;              // 交易密码
    string auth_code;             // 认证码（部分 API 需填）
    string productinfo;           // 产品信息（CTP 等需填）
    vector<std::string> md_address;   // 行情服务器地址列表
    vector<std::string> td_address;    // 交易服务器地址列表
    string appid;                 // 应用 ID（部分 API 需填）
    string publicstream;          // 公有流地址（XTP 等）
    string privatestream;         // 私有流地址（XTP 等）
};

// reserved for furture use
class BaseData{
 public:
    virtual string serialize() {return string();}
    virtual void deserialize(const string& msgin) {}
};
// reserved for furture use
class MsgFrame{
 public:
    string destination_;
    string source_;
    MSG_TYPE msgtype_;
    std::shared_ptr<BaseData> dataPtr;

    MsgFrame() {}
    MsgFrame(string des, string sour, MSG_TYPE mt_) {
        destination_ = des;
        source_ = sour;
        msgtype_ = mt_;
    }
    virtual ~MsgFrame() {}

    string serialize();
    void deserialize(const string& msgin);
};



class MsgHeader{
 public:
    string destination_;
    string source_;
    MSG_TYPE msgtype_;

    MsgHeader() {}
    MsgHeader(string des, string sour, MSG_TYPE mt_) {
        destination_ = des;
        source_ = sour;
        msgtype_ = mt_;
    }
    virtual ~MsgHeader() {}

    virtual string serialize() {
        string tmp = destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_);
        return tmp;
    }
    virtual void deserialize(const string& msgin) {}
};

class DLL_EXPORT_IMPORT Tick {  // 行情 tick 数据结构
 public:
    Tick() { }
    ~Tick() { }

    string fullSymbol_;          // 合约全称（如 IF2403.CFFEX）
    string time_;                // 行情时间
    double price_ = 0.0;         // 最新价
    int32_t size_ = 0;          // 最新成交量
    int32_t depth_ = 1;         // 档位深度（1/5/10/20 档）
    double bidPrice_[20] = {0.0};  // 买价档位 [1~20]
    int32_t bidSize_[20] = {0};    // 买量档位 [1~20]
    double askPrice_[20] = {0.0};  // 卖价档位 [1~20]
    int32_t askSize_[20] = {0};   // 卖量档位 [1~20]
    int32_t openInterest_ = 0;   // 持仓量（期货）
    double open_ = 0.0;          // 开盘价
    double high_ = 0.0;          // 最高价
    double low_ = 0.0;           // 最低价
    double preClose_ = 0.0;     // 昨收价
    double upperLimitPrice_ = 0.0;  // 涨停价
    double lowerLimitPrice_ = 0.0;  // 跌停价
};

class DLL_EXPORT_IMPORT TickMsg: public MsgHeader{
 public:
    TickMsg():MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_TICK_L1;
    }
    ~TickMsg() {}

    Tick data_;

    virtual string serialize();
    // virtual void deserialize(const string& msgin);
};

class DLL_EXPORT_IMPORT Security {  // 合约/标的证券信息
 public:
    Security() {}
    ~Security() {}
    string fullSymbol_;           // 合约全称（如 IF2403.CFFEX）
    string symbol_;               // 合约代码（不含交易所）
    char securityType_;          // 证券类型（期货/股票/期权等）
    string exchange_;             // 交易所代码
    string contractNo_;           // 合约编号
    int32_t multiplier_ = 0;      // 合约乘数
    string localName_;            // 本地名称（如中文名）
    string currency_;             // 交易币种
    double ticksize_ = 0.0;       // 最小变动价位
    double longMarginRatio_ = 0.0;   // 多头保证金比例
    double shortMarginRatio_ = 0.0;  // 空头保证金比例
    char postype_ = '2';          // 持仓类型（净/综合等）
    // 期权相关
    string underlyingSymbol_;     // 标的合约代码
    char optionType_;             // 期权类型（看涨 C / 看跌 P）
    double strikePrice_ = 0.0;    // 行权价
    char strikeMode_;             // 行权方式
    char strikeType_;             // 行权类型
    char applyType_;              // 申请类型（行权/弃权等）
    string expiryDate_;           // 到期日
};

class DLL_EXPORT_IMPORT SecurityMsg: public MsgHeader{
 public:
    SecurityMsg() :MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_RSP_CONTRACT;
    }
    ~SecurityMsg() {}

    Security data_;

    virtual string serialize();
    // virtual void deserialize(const string& msgin);
};

class AccountInfo {  // 账户信息
 public:
    AccountInfo() {}
    ~AccountInfo() {}

    string accountID_;                    // 账户 ID
    string accountType_;                  // 账户类型（如 INDIVIDUAL 个人）
    double availableFunds_ = 0.0;        // 可用资金
    double netLiquidation_ = 0.0;        // 净资产/清算净值
    double equityWithLoanValue_ = 0.0;   // 含融资市值权益
    double previousDayEquityWithLoanValue_ = 0.0;  // 昨日含融资市值权益
    double fullInitialMargin_ = 0.0;     // 初始保证金
    double fullMaintainanceMargin_ = 0.0;  // 维持保证金
    double commission_ = 0.0;           // 手续费
    double buyingPower_ = 0.0;          // 购买力/可开仓金额
    double cashBalance_ = 0.0;          // 现金余额
    double realizedPnL_ = 0.0;          // 已实现盈亏
    double unrealizedPnL_ = 0.0;        // 未实现盈亏
    double frozen_ = 0.0;                // 冻结资金
    double balance_ = 0.0;               // 账户余额
};

class DLL_EXPORT_IMPORT AccMsg: public MsgHeader{
 public:
    AccMsg() :MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_RSP_ACCOUNT;
    }
    ~AccMsg() {}

    AccountInfo data_;

    virtual string serialize();
    // virtual void deserialize(const string& msgin);
};


class DLL_EXPORT_IMPORT Fill {  // 成交记录
 public:
    Fill() { }
    ~Fill() {}

    int64_t serverOrderID_ = -1;   // 本地服务端订单 ID
    int64_t clientOrderID_ = -1;   // 客户端订单 ID
    int64_t brokerOrderID_ = -1;   // 经纪商订单 ID
    int64_t tradeId_ = -1;         // 本地成交 ID
    int32_t clientID_ = 0;        // 客户端 ID（0 表示手动）
    string orderNo_;               // 交易所订单号
    string localNo_;               // 本地订单号
    string tradeNo_;               // 交易所成交号
    string tradeTime_;             // 成交时间
    string fullSymbol_;            // 合约全称
    string account_;               // 账户
    OrderFlag fillFlag_ = OrderFlag::OF_OpenPosition;  // 开平标志
    string api_;                   // 交易接口（如 IB、CTP）
    string currency_;              // 币种
    double tradePrice_ = 0.0;      // 成交价
    int32_t tradeSize_ = 0;        // 成交数量（<0 为卖/空）
    double commission_ = 0.0;      // 手续费
};

class DLL_EXPORT_IMPORT FillMsg: public MsgHeader{
 public:
    FillMsg(): MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_RTN_TRADE;
    }
    ~FillMsg() {}

    Fill data_;

    virtual string serialize();
    // virtual void deserialize(const string& msgin);
};

extern int64_t m_serverOrderId;  // unique order id on server side defined in ordermanager.cpp.
extern std::mutex oid_mtx;  // mutex for increasing order id
extern std::mutex orderStatus_mtx;  // mutex for changing order status


class DLL_EXPORT_IMPORT Order {  // 订单
 public:
    Order() { }
    ~Order() {}
    // 请求头
    string api_;                 // 交易接口（如 CTP、TAP）
    string account_;             // 账户
    int32_t clientID_ = 0;       // 客户端 ID（0 表示手动）
    int64_t clientOrderID_ = -1; // 客户端订单 ID
    string tag_;                 // 预留扩展字段
    // 服务端与回报内容
    string fullSymbol_;          // 合约全称（交易所+类型+品种+合约号）
    double price_ = 0.0;         // 委托价格
    int32_t quantity_ = 0;       // 委托数量
    int32_t tradedvol_ = 0;      // 已成交数量
    OrderFlag flag_ = OrderFlag::OF_OpenPosition;  // 开平标志
    int64_t serverOrderID_ = -1;  // 服务端订单 ID（orderref）
    int64_t brokerOrderID_ = -1; // 经纪商订单 ID
    string orderNo_;             // 交易所订单号（CTP 为 exchangeID+orsysid）
    string localNo_;             // 本地订单号（frontid+sessionid+orderref）
    string createTime_;          // 创建时间
    string updateTime_;          // 更新时间
    OrderStatus orderStatus_ = OrderStatus::OS_UNKNOWN;  // 订单状态
    OrderType orderType_ = OrderType::OT_Default;        // 订单类型
};

bool isActiveOrder(const Order& o);   // 判断订单是否处于活跃状态（可撤单）
bool isActiveOS(const OrderStatus& os);  // 判断订单状态是否为活跃状态

class DLL_EXPORT_IMPORT PaperOrder : public Order {  // 模拟盘订单
 public:
    PaperOrder() {}
    ~PaperOrder() {}

    OrderFlag orderFlag_ = OrderFlag::OF_OpenPosition;  // 开平标志
    int32_t orderSize_ = 0;       // 委托数量
    string fillNo_;                // 成交编号（<0 表示卖/空）
    int32_t filledSize_ = 0;       // 已成交数量
    double lastFilledPrice_ = 0.0; // 最近成交价
    double avgFilledPrice_ = 0.0;   // 平均成交价
    double limitPrice_ = 0.0;      // 限价
    double stopPrice_ = 0.0;       // 止损价
    double trailPrice_ = 0.0;      // 追踪止损价
    double trailingPercent_ = 0.0; // 追踪止损百分比
    string timeInForce_;           // 有效期（如 GTC、DAY）
    bool outsideRegularTradingHour_ = false;  // 是否盘外交易
    bool hidden_ = false;          // 是否冰山单（隐藏数量）
    bool allOrNone_ = false;       // 是否全部成交否则取消（FOK）
};



class DLL_EXPORT_IMPORT CtpOrder : public Order {  // CTP 实盘订单
 public:
    CtpOrder() {}
    ~CtpOrder() {}
    struct CThostFtdcInputOrderField orderField_;  // CTP 下单请求字段
};

class DLL_EXPORT_IMPORT CtpParkedOrder : public Order {  // CTP 预埋单
 public:
    CtpParkedOrder() {}
    ~CtpParkedOrder() {}
    struct CThostFtdcParkedOrderField parkedOrderField_;  // CTP 预埋单字段
};

class DLL_EXPORT_IMPORT OrderMsg : public MsgHeader {  // 通用订单消息
 public:
    OrderMsg(): MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_ORDER;
    }
    ~OrderMsg() {}

    Order data_;  // 订单数据

    virtual void deserialize(const string& msgin);
    std::shared_ptr<Order> toPOrder();  // 将 data_ 转为 shared_ptr<Order>，供 OrderManager 跟踪
};

class DLL_EXPORT_IMPORT PaperOrderMsg : public MsgHeader {  // 模拟盘订单消息
 public:
    PaperOrderMsg(): MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_ORDER_PAPER;
    }
    ~PaperOrderMsg() {}

    PaperOrder data_;

    virtual void deserialize(const string& msgin);
    std::shared_ptr<Order> toPOrder();  // 转为 shared_ptr<PaperOrder>（继承自 Order）
};

class DLL_EXPORT_IMPORT CtpOrderMsg : public MsgHeader {  // CTP 实盘订单消息
 public:
    CtpOrderMsg(): MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_ORDER_CTP;
    }
    ~CtpOrderMsg() {}

    CtpOrder data_;

    virtual void deserialize(const string& msgin);
    std::shared_ptr<Order> toPOrder();  // 转为 shared_ptr<CtpOrder>（继承自 Order）
};

class DLL_EXPORT_IMPORT CtpParkedOrderMsg : public MsgHeader {  // CTP 预埋单消息
 public:
    CtpParkedOrderMsg(): MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_ORDER_CTP;
    }
    ~CtpParkedOrderMsg() {}

    CtpParkedOrder data_;

    virtual void deserialize(const string& msgin);
    std::shared_ptr<Order> toPOrder();  // 转为 shared_ptr<CtpParkedOrder>（继承自 Order）
};

class DLL_EXPORT_IMPORT OrderStatusMsg : public MsgHeader {  // 订单状态回报消息
 public:
    OrderStatusMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE::MSG_TYPE_RTN_ORDER), data_() {
    }
    ~OrderStatusMsg() {}

    Order data_;  // 订单状态数据

    void set(std::shared_ptr<Order> po);  // 从 Order 指针填充 data_
    virtual string serialize();
};



class DLL_EXPORT_IMPORT Position {  // 持仓
 public:
    Position() {}
    ~Position() {}

    string key_ = "";              // 持仓唯一键（账户+合约等）
    string account_ = "";          // 账户
    string api_ = "";              // 交易接口
    string fullSymbol_ = "";       // 合约全称
    double avgPrice_ = 0;          // 持仓均价
    int32_t size_ = 0;             // 当前持仓数量
    int32_t preSize_ = 0;          // 昨日持仓数量
    int32_t freezedSize_ = 0;      // 冻结数量
    double openpl_ = 0;            // 未实现盈亏
    double closedpl_ = 0;          // 已实现盈亏
    char type_ = '1';               // 方向（多/空）
    string posNo_ = "";            // 持仓编号
};

class DLL_EXPORT_IMPORT PosMsg : public MsgHeader {  // 持仓查询响应消息
 public:
    PosMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_RSP_POS), data_() {}
    ~PosMsg() {}

    Position data_;  // 持仓数据

    virtual string serialize();
    void set(std::shared_ptr<Position>);  // 从 Position 指针填充 data_
};


class DLL_EXPORT_IMPORT PositionDetail {  // 持仓明细（含开平仓来源）
 public:
    PositionDetail() {}
    ~PositionDetail() {}

    string key_ = "";              // 持仓唯一键
    string account_ = "";          // 账户
    string api_ = "";              // 交易接口
    string fullSymbol_ = "";       // 合约全称
    double avgPrice_ = 0;          // 持仓均价
    int32_t size_ = 0;             // 当前持仓数量
    int32_t preSize_ = 0;          // 昨日持仓数量
    int32_t freezedSize_ = 0;      // 冻结数量
    double openpl_ = 0;            // 未实现盈亏
    double closedpl_ = 0;          // 已实现盈亏
    char type_ = '1';               // 方向（多/空）
    string posNo_ = "";            // 持仓编号
    string openapi_ = "";          // 开仓接口
    string openOrderNo_ = "";      // 开仓订单号
    int32_t openClientID_ = -1;    // 开仓客户端 ID
    string closeapi_ = "";         // 平仓接口
    string closeOrderNo_ = "";     // 平仓订单号
    int32_t closeClientID_ = -1;   // 平仓客户端 ID
};

class DLL_EXPORT_IMPORT PosDetailMsg : public MsgHeader {  // 持仓明细查询响应消息
 public:
    PosDetailMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_RSP_POS), data_() {}
    ~PosDetailMsg() {}

    PositionDetail data_;

    virtual string serialize() {return string();}
    void set(std::shared_ptr<PositionDetail>) {}
};




class OrderAction {  // 撤单请求
 public:
    OrderAction() {}
    ~OrderAction() {}

    int32_t clientID_ = 0;        // 客户端 ID
    int64_t serverOrderID_ = -1;  // 服务端订单 ID
    int64_t clientOrderID_ = -1;  // 客户端订单 ID
    string orderNo_;               // 交易所订单号
};

class DLL_EXPORT_IMPORT OrderActionMsg : public MsgHeader {  // 撤单消息
 public:
    OrderActionMsg():MsgHeader(), data_() {
         msgtype_ = MSG_TYPE::MSG_TYPE_ORDER_ACTION;
    }
    OrderActionMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_ORDER_ACTION), data_() {}
    ~OrderActionMsg() {}

    OrderAction data_;  // 撤单请求数据

    virtual void deserialize(const string& msgin);
};




class DLL_EXPORT_IMPORT SubscribeMsg : public MsgHeader {  // 订阅行情消息
 public:
    SubscribeMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_SUBSCRIBE_MARKET_DATA), data_() {}
    SubscribeMsg():MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_SUBSCRIBE_MARKET_DATA;
    }
    ~SubscribeMsg() {}
    SymbolType symtype_;   // 标的类型（期货/股票等）
    vector<string> data_; // 待订阅合约列表

    virtual void deserialize(const string& msgin);
};

class DLL_EXPORT_IMPORT UnSubscribeMsg : public MsgHeader {  // 取消订阅行情消息
 public:
    UnSubscribeMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_UNSUBSCRIBE), data_() {}
    UnSubscribeMsg():MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_UNSUBSCRIBE;
    }
    ~UnSubscribeMsg() {}

    SymbolType symtype_;   // 标的类型
    vector<string> data_;  // 待取消订阅的合约列表

    virtual void deserialize(const string& msgin);
};

class DLL_EXPORT_IMPORT QryContractMsg : public MsgHeader {  // 查询合约消息
 public:
    QryContractMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_QRY_CONTRACT), data_() {}
    QryContractMsg():MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_QRY_CONTRACT;
    }
    ~QryContractMsg() {}

    SymbolType symtype_;  // 标的类型
    string data_;         // 查询条件（如合约代码）

    virtual void deserialize(const string& msgin);
};

class DLL_EXPORT_IMPORT CancelAllMsg : public MsgHeader {  // 撤销全部订单消息
 public:
    CancelAllMsg(string des, string src):
    MsgHeader(des, src, MSG_TYPE_CANCEL_ALL), data_() {}
    CancelAllMsg():MsgHeader(), data_() {
        msgtype_ = MSG_TYPE::MSG_TYPE_CANCEL_ALL;
    }
    ~CancelAllMsg() {}

    SymbolType symtype_;  // 标的类型
    string data_;         // 筛选条件（如合约代码，空表示全部）

    virtual void deserialize(const string& msgin);
};

class DLL_EXPORT_IMPORT ErrorMsg : public MsgHeader {  // 错误消息
 public:
    ErrorMsg(string des, string src, MSG_TYPE mt, string em) :
    MsgHeader(des, src, mt), data_(em) {}
    ~ErrorMsg() {}

    string data_;  // 错误描述

    virtual string serialize();
};

class DLL_EXPORT_IMPORT InfoMsg : public MsgHeader {  // 系统信息消息
 public:
    InfoMsg(string des, string src, MSG_TYPE mt, string em):
    MsgHeader(des, src, mt), data_(em) {}
    ~InfoMsg() {}

    string data_;  // 信息内容

    virtual string serialize();
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_COMMON_DATASTRUCT_H_
