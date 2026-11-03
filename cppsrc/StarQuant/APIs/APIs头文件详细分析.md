# APIs 子目录头文件(.h)详细分析

本文档对 `cppsrc/StarQuant/APIs` 下各子目录中的 `.h` 文件进行逐文件、逐类/结构体的详细分析，包括变量、公私有函数及关键行代码解释。按工作计划分阶段完成。

---

## 一、Ctp 子目录

### 1.1 DataCollect.h

**路径**：`Ctp/DataCollect.h`  
**用途**：CTP 终端数据采集接口，用于获取 AES/RSA 加密所需的终端信息（如机器码等）。  
**依赖**：无其他项目头文件。

#### 预处理与宏（逐行）

| 行号 | 代码 | 说明 |
|------|------|------|
| 1 | `#ifndef  DATA_COLLECT_H` | 头文件保护开始，防止重复包含 |
| 2 | `#define  DATA_COLLECT_H` | 定义保护宏 |
| 4 | `#define DLL_EXPORT   __declspec(dllexport)` | Windows 下 DLL 导出属性宏，用于标记需导出的符号 |
| 6 | `#if defined(IS_WINCOLLECT_LIB) && defined(WIN32)` | 仅在“Windows 且定义为数据采集库”时使用 DLL 导入/导出 |
| 7 | `#ifdef LIB_DATA_COLLECT_API_EXPORT` | 若在编译数据采集库本身，则导出 |
| 8 | `#define DATA_COLLECT_API_EXPORT __declspec(dllexport)` | 导出宏 |
| 9 | `#else` | 否则视为使用方 |
| 10 | `#define DATA_COLLECT_API_EXPORT __declspec(dllimport)` | 导入宏 |
| 11 | `#endif` | 结束 LIB_DATA_COLLECT_API_EXPORT 分支 |
| 12 | `#else` | 非 Windows 或非 IS_WINCOLLECT_LIB |
| 13 | `#define DATA_COLLECT_API_EXPORT ` | 空宏，非 Windows 下不修饰 |
| 14 | `#endif` | 结束 IS_WINCOLLECT_LIB/WIN32 分支 |
| 17-21 | 注释 + 函数声明 | 见下方“全局函数” |

#### 全局函数

| 声明 | 说明 |
|------|------|
| `DATA_COLLECT_API_EXPORT int CTP_GetSystemInfo(char* pSystemInfo, int& nLen);` | **获取终端信息**。`pSystemInfo`：出参，调用方分配至少 270 字节；`nLen`：出参，实际写入长度。内容可能含 `\0`，建议用内存复制。返回值为错误码或成功标识。 |

#### 类/结构体

本文件无类或结构体定义。

---

### 1.2 ThostFtdcMdApi.h

**路径**：`Ctp/ThostFtdcMdApi.h`  
**用途**：CTP 行情 API：客户端与 CTP 行情前置交互的接口与回调（Spi）定义。  
**依赖**：`#include "ThostFtdcUserApiStruct.h"`（所有请求/响应/通知所用结构体）。

#### 预处理与宏

| 行号 | 代码 | 说明 |
|------|------|------|
| 10-11 | `#if !defined(THOST_FTDCMDAPI_H)` / `#define THOST_FTDCMDAPI_H` | 头文件保护 |
| 13-15 | `#if _MSC_VER > 1000` / `#pragma once` | MSVC 下附加的防重包含 |
| 19-27 | `#if defined(ISLIB) && defined(WIN32)` 等 | Windows 且为库时：`LIB_MD_API_EXPORT` 则 `MD_API_EXPORT` 为 dllexport，否则 dllimport；非 Windows 时 `MD_API_EXPORT` 为空 |

#### 类 CThostFtdcMdSpi（行情回调接口）

**说明**：用户实现此类并注册给 `CThostFtdcMdApi`，用于接收连接、登录、订阅、行情等回调。所有方法均为 `public`、`virtual`、带空实现，可只重写需要的回调。

**成员函数（均为 public virtual）：**

| 方法 | 参数 | 说明 |
|------|------|------|
| `OnFrontConnected` | 无 | 与交易后台建立连接时调用（登录前） |
| `OnFrontDisconnected` | `int nReason` | 连接断开时调用；nReason：0x1001 网络读失败，0x1002 网络写失败，0x2001 收心跳超时，0x2002 发心跳失败，0x2003 错误报文 |
| `OnHeartBeatWarning` | `int nTimeLapse` | 长时间未收到报文时的心跳超时警告 |
| `OnRspUserLogin` | `CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast` | 登录请求响应 |
| `OnRspUserLogout` | `CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast` | 登出请求响应 |
| `OnRspError` | `CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast` | 错误应答 |
| `OnRspSubMarketData` | `CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast` | 订阅行情应答 |
| `OnRspUnSubMarketData` | 同上 | 取消订阅行情应答 |
| `OnRspSubForQuoteRsp` | 同上 | 订阅询价应答 |
| `OnRspUnSubForQuoteRsp` | 同上 | 取消订阅询价应答 |
| `OnRtnDepthMarketData` | `CThostFtdcDepthMarketDataField *pDepthMarketData` | 深度行情推送 |
| `OnRtnForQuoteRsp` | `CThostFtdcForQuoteRspField *pForQuoteRsp` | 询价通知 |

**成员变量**：无。

#### 类 CThostFtdcMdApi（行情 API）

**说明**：行情接口主类，带 `MD_API_EXPORT` 导出。纯虚接口，由 CTP 库提供实现。

**静态方法（public）：**

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `CreateFtdcMdApi` | `const char *pszFlowPath = "", const bool bIsUsingUdp=false, const bool bIsMulticast=false` | `CThostFtdcMdApi*` | 创建行情 API；pszFlowPath 为订阅信息存储目录；后两参数支持 UDP/组播行情 |
| `GetApiVersion` | 无 | `const char*` | 返回 API 版本字符串 |

**公有虚方法（=0 纯虚）：**

| 方法 | 说明 |
|------|------|
| `Release()` | 释放接口对象 |
| `Init()` | 初始化，调用后接口才开始工作 |
| `Join()` | 等待接口线程结束，返回线程退出码 |
| `GetTradingDay()` | 获取当前交易日（需登录成功后有效） |
| `RegisterFront(char *pszFrontAddress)` | 注册前置地址，格式如 "tcp://127.0.0.1:17001" |
| `RegisterNameServer(char *pszNsAddress)` | 注册名字服务器地址，优先于 RegisterFront |
| `RegisterFensUserInfo(CThostFtdcFensUserInfoField * pFensUserInfo)` | 注册 FENS 用户信息 |
| `RegisterSpi(CThostFtdcMdSpi *pSpi)` | 注册回调 Spi |
| `SubscribeMarketData(char *ppInstrumentID[], int nCount)` | 订阅行情 |
| `UnSubscribeMarketData(char *ppInstrumentID[], int nCount)` | 退订行情 |
| `SubscribeForQuoteRsp` / `UnSubscribeForQuoteRsp` | 订阅/退订询价 |
| `ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLoginField, int nRequestID)` | 登录请求 |
| `ReqUserLogout(CThostFtdcUserLogoutField *pUserLogout, int nRequestID)` | 登出请求 |

**保护成员：**

| 方法 | 说明 |
|------|------|
| `~CThostFtdcMdApi(){}` | 析构为 protected，防止栈上创建，只能通过 Release 或工厂管理生命周期 |

---

### 1.3 ThostFtdcTraderApi.h

**路径**：`Ctp/ThostFtdcTraderApi.h`  
**用途**：CTP 交易 API：报单、撤单、查询、银期转账、结算确认等交易相关请求与回调。  
**依赖**：`ThostFtdcUserApiStruct.h`。

#### 预处理与宏

与 MdApi 类似：头文件保护 `THOST_FTDCTRADERAPI_H`、`#pragma once`、Windows 下 `TRADER_API_EXPORT` 的 dllexport/dllimport。

#### 类 CThostFtdcTraderSpi（交易回调接口）

**说明**：交易侧回调，全部为 `public virtual`，默认空实现。仅列出类别与代表性方法，便于查阅。

**连接与认证：**  
`OnFrontConnected` / `OnFrontDisconnected(int nReason)` / `OnHeartBeatWarning(int nTimeLapse)` — 同行情；  
`OnRspAuthenticate` — 客户端认证响应；  
`OnRspUserLogin` / `OnRspUserLogout` — 登录/登出响应；  
`OnRspUserPasswordUpdate` / `OnRspTradingAccountPasswordUpdate` — 密码更新响应；  
`OnRspUserAuthMethod` / `OnRspGenUserCaptcha` / `OnRspGenUserText` — 认证模式与验证码相关。

**报单与操作响应（OnRsp*）：**  
报单：`OnRspOrderInsert`、`OnRspParkedOrderInsert`、`OnRspParkedOrderAction`、`OnRspOrderAction`、`OnRspQueryMaxOrderVolume`、`OnRspSettlementInfoConfirm`、`OnRspRemoveParkedOrder`、`OnRspRemoveParkedOrderAction`；  
期权/组合：`OnRspExecOrderInsert`、`OnRspExecOrderAction`、`OnRspForQuoteInsert`、`OnRspQuoteInsert`、`OnRspQuoteAction`、`OnRspBatchOrderAction`、`OnRspOptionSelfCloseInsert`、`OnRspOptionSelfCloseAction`、`OnRspCombActionInsert`。

**查询响应（OnRspQry*）：**  
订单与成交：`OnRspQryOrder`、`OnRspQryTrade`；  
持仓与资金：`OnRspQryInvestorPosition`、`OnRspQryTradingAccount`、`OnRspQryInvestor`、`OnRspQryTradingCode`；  
合约与费率：`OnRspQryInstrumentMarginRate`、`OnRspQryInstrumentCommissionRate`、`OnRspQryExchange`、`OnRspQryProduct`、`OnRspQryInstrument`、`OnRspQryDepthMarketData`；  
结算与通知：`OnRspQrySettlementInfo`、`OnRspQryTransferBank`、`OnRspQryInvestorPositionDetail`、`OnRspQryNotice`、`OnRspQrySettlementInfoConfirm`、`OnRspQryInvestorPositionCombineDetail`；  
以及大量其他查询（CFMMC、仓单、保证金、汇率、二级代理、做市商、期权手续费、执行宣告、询价、报价、期权自对冲、组合、转账流水、银期签约、预埋单、交易通知、经纪公司参数/算法、监控中心令牌等）。

**错误与通知：**  
`OnRspError` — 通用错误应答；  
`OnRtnOrder` / `OnRtnTrade` — 报单/成交通知；  
`OnErrRtnOrderInsert` / `OnErrRtnOrderAction` — 报单/撤单错误回报；  
`OnRtnInstrumentStatus` / `OnRtnBulletin` / `OnRtnTradingNotice` / `OnRtnErrorConditionalOrder` — 合约状态、公告、交易通知、条件单错误；  
执行宣告/询价/报价：`OnRtnExecOrder`、`OnErrRtnExecOrderInsert`、`OnErrRtnExecOrderAction`、`OnErrRtnForQuoteInsert`、`OnRtnQuote`、`OnErrRtnQuoteInsert`、`OnErrRtnQuoteAction`、`OnRtnForQuoteRsp`；  
`OnRtnCFMMCTradingAccountToken` — 保证金监控中心令牌；  
批量报单错误：`OnErrRtnBatchOrderAction`；  
期权自对冲：`OnRtnOptionSelfClose`、`OnErrRtnOptionSelfCloseInsert`、`OnErrRtnOptionSelfCloseAction`；  
组合：`OnRtnCombAction`、`OnErrRtnCombActionInsert`；  
更多查询响应：预埋单、预埋撤单、交易通知、经纪公司交易参数/算法、监控中心令牌查询等。

**银期与银行相关：**  
银行发起：`OnRtnFromBankToFutureByBank`、`OnRtnFromFutureToBankByBank`、`OnRtnRepealFromBankToFutureByBank`、`OnRtnRepealFromFutureToBankByBank`；  
期货发起：`OnRtnFromBankToFutureByFuture`、`OnRtnFromFutureToBankByFuture`、`OnRtnRepealFromBankToFutureByFutureManual`、`OnRtnRepealFromFutureToBankByFutureManual`、`OnRtnQueryBankBalanceByFuture`；  
错误回报：`OnErrRtnBankToFutureByFuture`、`OnErrRtnFutureToBankByFuture`、`OnErrRtnRepealBankToFutureByFutureManual`、`OnErrRtnRepealFutureToBankByFutureManual`、`OnErrRtnQueryBankBalanceByFuture`；  
冲正与应答：`OnRtnRepealFromBankToFutureByFuture`、`OnRtnRepealFromFutureToBankByFuture`、`OnRspFromBankToFutureByFuture`、`OnRspFromFutureToBankByFuture`、`OnRspQueryBankAccountMoneyByFuture`；  
银期开户/销户/变更：`OnRtnOpenAccountByBank`、`OnRtnCancelAccountByBank`、`OnRtnChangeAccountByBank`。

**成员变量**：无。

#### 类 CThostFtdcTraderApi（交易 API）

**说明**：交易主接口，`TRADER_API_EXPORT`，纯虚接口。

**静态方法：**  
`CreateFtdcTraderApi(const char *pszFlowPath = "")` — 创建交易 API；  
`GetApiVersion()` — 版本字符串。

**生命周期与环境：**  
`Release()`、`Init()`、`Join()`、`GetTradingDay()` — 同行情 API；  
`RegisterFront` / `RegisterNameServer` / `RegisterFensUserInfo` / `RegisterSpi` — 同行情；  
`SubscribePrivateTopic(THOST_TE_RESUME_TYPE nResumeType)` — 订阅私有流（需在 Init 前调用）；  
`SubscribePublicTopic(THOST_TE_RESUME_TYPE nResumeType)` — 订阅公共流。

**认证与登录：**  
`ReqAuthenticate`、`RegisterUserSystemInfo`、`SubmitUserSystemInfo`、`ReqUserLogin`、`ReqUserLogout`、  
`ReqUserPasswordUpdate`、`ReqTradingAccountPasswordUpdate`、`ReqUserAuthMethod`、`ReqGenUserCaptcha`、`ReqGenUserText`、  
`ReqUserLoginWithCaptcha`、`ReqUserLoginWithText`、`ReqUserLoginWithOTP`。

**报单与操作：**  
`ReqOrderInsert`、`ReqParkedOrderInsert`、`ReqParkedOrderAction`、`ReqOrderAction`、`ReqQueryMaxOrderVolume`、  
`ReqSettlementInfoConfirm`、`ReqRemoveParkedOrder`、`ReqRemoveParkedOrderAction`、  
`ReqExecOrderInsert`、`ReqExecOrderAction`、`ReqForQuoteInsert`、`ReqQuoteInsert`、`ReqQuoteAction`、  
`ReqBatchOrderAction`、`ReqOptionSelfCloseInsert`、`ReqOptionSelfCloseAction`、`ReqCombActionInsert`。

**查询（ReqQry*）：**  
与 Spi 中 OnRspQry* 一一对应，包括：Order、Trade、InvestorPosition、TradingAccount、Investor、TradingCode、InstrumentMarginRate、InstrumentCommissionRate、Exchange、Product、Instrument、DepthMarketData、SettlementInfo、TransferBank、InvestorPositionDetail、Notice、SettlementInfoConfirm、InvestorPositionCombineDetail、CFMMCTradingAccountKey、EWarrantOffset、InvestorProductGroupMargin、ExchangeMarginRate、ExchangeMarginRateAdjust、ExchangeRate、SecAgentACIDMap、ProductExchRate、ProductGroup、MMInstrumentCommissionRate、MMOptionInstrCommRate、InstrumentOrderCommRate、SecAgentTradingAccount、SecAgentCheckMode、SecAgentTradeInfo、OptionInstrTradeCost、OptionInstrCommRate、ExecOrder、ForQuote、Quote、OptionSelfClose、InvestUnit、CombInstrumentGuard、CombAction、TransferSerial、Accountregister、ContractBank、ParkedOrder、ParkedOrderAction、TradingNotice、BrokerTradingParams、BrokerTradingAlgos、QueryCFMMCTradingAccountToken。

**银期：**  
`ReqFromBankToFutureByFuture`、`ReqFromFutureToBankByFuture`、`ReqQueryBankAccountMoneyByFuture`。

**保护析构：**  
`~CThostFtdcTraderApi(){}` — 仅能通过 Release 或工厂释放。

---

### 1.4 ThostFtdcUserApiDataType.h（摘要）

**路径**：`Ctp/ThostFtdcUserApiDataType.h`  
**行数**：约 6663 行。  
**用途**：定义 CTP 接口使用的所有**业务数据类型**：枚举、typedef（多为定长 char 数组或数值类型）。

#### 文件结构说明

- **头尾**：`THOST_FTDCDATATYPE_H` 保护，无 include。
- **内容模式**：大量 `typedef char TThostFtdcXXXType[N];` 或 `typedef int/double/short TThostFtdcXXXType;`，以及若干 `enum` 和单字节 `typedef char TThostFtdcXXXType;`（表示枚举字符）。

#### 枚举（摘）

- **THOST_TE_RESUME_TYPE**（第13–18行）：流重传方式  
  - `THOST_TERT_RESTART = 0` 从本交易日重传  
  - `THOST_TERT_RESUME` 从上次收到的续传  
  - `THOST_TERT_QUICK` 只传登录后流内容  

#### 类型定义模式与样例（逐行解释）

以文件前若干行为例：

| 行号 | 代码 | 说明 |
|------|------|------|
| 13 | `enum THOST_TE_RESUME_TYPE` | 流重传类型枚举开始 |
| 14-17 | `THOST_TERT_RESTART = 0` 等 | 见上 |
| 23 | `typedef char TThostFtdcTraderIDType[21];` | 交易所交易员代码，最长 20 字符 + '\0' |
| 28 | `typedef char TThostFtdcInvestorIDType[13];` | 投资者代码 |
| 33 | `typedef char TThostFtdcBrokerIDType[11];` | 经纪公司代码 |
| 38 | `typedef char TThostFtdcBrokerAbbrType[9];` | 经纪公司简称 |
| 78 | `typedef char TThostFtdcInstrumentIDType[31];` | 合约代码 |
| 138 | `typedef char TThostFtdcDateType[9];` | 日期，如 YYYYMMDD |
| 143 | `typedef char TThostFtdcTimeType[9];` | 时间 |
| 183 | `typedef int TThostFtdcIPPortType;` | IP 端口 |
| 652 | `typedef char TThostFtdcDirectionType;` | 买卖方向（单字符枚举） |
| 718 | `typedef char TThostFtdcHedgeFlagType;` | 投机套保标志 |
| 1102 | `typedef double TThostFtdcPriceType;` | 价格 |
| 1122 | `typedef double TThostFtdcMoneyType;` | 金额 |

全文件包含数百个类似 typedef，命名规则为 `TThostFtdc` + 业务含义 + `Type`，用于在 `ThostFtdcUserApiStruct.h` 中定义各字段类型。完整列表见原文件或按 “typedef” / “enum” 检索。

---

### 1.5 ThostFtdcUserApiStruct.h（摘要）

**路径**：`Ctp/ThostFtdcUserApiStruct.h`  
**行数**：约 9492 行。  
**用途**：定义 CTP 接口使用的所有**业务结构体**，依赖 `ThostFtdcUserApiDataType.h`。

#### 文件结构说明

- **头**：`THOST_FTDCSTRUCT_H`、`#pragma once`、`#include "ThostFtdcUserApiDataType.h"`。
- **内容**：均为 `struct CThostFtdcXXXField { ... };`，字段全部使用 `TThostFtdc*Type` 或嵌套其他 `CThostFtdc*Field`。

#### 结构体样例（逐行解释）：CThostFtdcReqUserLoginField

| 行号（约） | 代码 | 说明 |
|------------|------|------|
| 29 | `struct CThostFtdcReqUserLoginField` | 用户登录请求结构体 |
| 31 | `TThostFtdcDateType TradingDay;` | 交易日 |
| 32 | `TThostFtdcBrokerIDType BrokerID;` | 经纪公司代码 |
| 33 | `TThostFtdcUserIDType UserID;` | 用户代码 |
| 34 | `TThostFtdcPasswordType Password;` | 密码 |
| 35 | `TThostFtdcProductInfoType UserProductInfo;` | 用户端产品信息 |
| 36 | `TThostFtdcProductInfoType InterfaceProductInfo;` | 接口端产品信息 |
| 37 | `TThostFtdcProtocolInfoType ProtocolInfo;` | 协议信息 |
| 38 | `TThostFtdcMacAddressType MacAddress;` | Mac 地址 |
| 39 | `TThostFtdcPasswordType OneTimePassword;` | 动态密码 |
| 40 | `TThostFtdcIPAddressType ClientIPAddress;` | 终端 IP |
| 41 | `TThostFtdcLoginRemarkType LoginRemark;` | 登录备注 |
| 42 | `TThostFtdcIPPortType ClientIPPort;` | 终端 IP 端口 |

#### 结构体列表（按出现顺序，部分）

信息与登录：CThostFtdcDisseminationField、CThostFtdcReqUserLoginField、CThostFtdcRspUserLoginField、CThostFtdcUserLogoutField、CThostFtdcForceUserLogoutField、CThostFtdcReqAuthenticateField、CThostFtdcRspAuthenticateField、CThostFtdcAuthenticationInfoField、CThostFtdcRspUserLogin2Field。  
转账与银行：CThostFtdcTransferHeaderField、CThostFtdcTransferBankToFutureReqField、CThostFtdcTransferBankToFutureRspField、CThostFtdcTransferFutureToBankReqField/RspField、CThostFtdcTransferQryBankReqField/RspField、CThostFtdcTransferQryDetailReqField/RspField。  
通用：CThostFtdcRspInfoField（应答信息）。  
基础主数据：CThostFtdcExchangeField、CThostFtdcProductField、CThostFtdcInstrumentField、CThostFtdcBrokerField、CThostFtdcTraderField、CThostFtdcInvestorField、CThostFtdcTradingCodeField、CThostFtdcPartBrokerField、CThostFtdcSuperUserField、CThostFtdcSuperUserFunctionField、CThostFtdcInvestorGroupField。  
账户与持仓：CThostFtdcTradingAccountField、CThostFtdcInvestorPositionField、CThostFtdcInstrumentMarginRateField、CThostFtdcInstrumentCommissionRateField。  
行情：CThostFtdcDepthMarketDataField、CThostFtdcInstrumentTradingRightField。  
经纪与用户：CThostFtdcBrokerUserField、CThostFtdcBrokerUserPasswordField、CThostFtdcBrokerUserFunctionField、CThostFtdcTraderOfferField。  
结算与保证金：CThostFtdcSettlementInfoField、CThostFtdcInstrumentMarginRateAdjustField、CThostFtdcExchangeMarginRateField、CThostFtdcExchangeMarginRateAdjustField、CThostFtdcExchangeRateField。  
订单与成交：CThostFtdcInputOrderField、CThostFtdcOrderField、CThostFtdcExchangeOrderField、CThostFtdcInputOrderActionField、CThostFtdcOrderActionField、CThostFtdcExchangeOrderActionField、CThostFtdcTradeField、CThostFtdcExchangeTradeField 等。  
查询条件（Qry*）：CThostFtdcQryOrderField、CThostFtdcQryTradeField、CThostFtdcQryInvestorPositionField、CThostFtdcQryTradingAccountField 等大量 Qry* 结构体。  
执行宣告、询价、报价、期权自对冲、组合：Input/Field/Qry/Exchange 一系列。  
银期、签约、开户/销户/变更：CThostFtdcReqTransferField、CThostFtdcRspTransferField、CThostFtdcReqRepealField、CThostFtdcRspRepealField、CThostFtdcAccountregisterField、CThostFtdcOpenAccountField、CThostFtdcCancelAccountField、CThostFtdcChangeAccountField 等。  
其他：公告、交易通知、错误条件单、同步、密钥、CFMMC、产品组、做市商费率、二级代理等对应结构体。  

完整名称列表见源码中所有 `struct CThostFtdc*Field` 定义（约 200+ 个结构体）。

---

## 二、Tap 子目录

### 2.1 TapAPICommDef.h

**路径**：`Tap/TapAPICommDef.h`  
**用途**：Tap 系列 API 的公共基础类型与结构体（字符串、数值、日期时间、日志级别、品种类型等）。  
**依赖**：无；使用 `#pragma pack(push, 1)` / `#pragma pack(pop)` 保证 1 字节对齐。

#### 预处理

- 头文件保护：`TAP_API_COMM_DEF_H`；`#pragma pack(push, 1)` 开始处、`#pragma pack(pop)` 结束处。

#### 类型定义（逐类说明）

**基本字符串（TAPISTR_*）**  
- `TAPISTR_10`～`TAPISTR_2000`：`typedef char TAPISTR_N[N+1]`，长度为 10/20/30/40/50/70/100/300/500/2000 的字符串（含 `\0`）。  
- `TAPIAUTHCODE`：`char[513]`，授权码。

**基本数值**  
- `TAPIINT32` / `TAPIUINT32`：int / unsigned int。  
- `TAPIINT64` / `TAPIUINT64`：long long / unsigned long long。  
- `TAPIUINT16` / `TAPIUINT8`：unsigned short / unsigned char。  
- `TAPIREAL64`：double。  
- `TAPICHAR`：char，单字符或枚举字符。

**是否标示**  
- `TAPIYNFLAG`：即 `TAPICHAR`；`APIYNFLAG_YES='Y'`，`APIYNFLAG_NO='N'`。

**日期时间**  
- `TAPIDTSTAMP`：时间戳，格式 `yyyy-MM-dd hh:nn:ss.xxx`，24 字符。  
- `TAPIDATETIME`：日期时间 20 字符；`TAPIDATE`：日期 11 字符；`TAPITIME`：时间 9 字符。

**日志级别**  
- `TAPILOGLEVEL`：`TAPICHAR`；常量 `APILOGLEVEL_NONE`/`ERROR`/`WARNING`/`DEBUG`。

**品种类型**  
- `TAPICommodityType`：单字符；常量包括 `TAPI_COMMODITY_TYPE_NONE`、`SPOT`、`FUTURES`、`OPTION`、各类套利、外汇、指数、股票等。

**看涨看跌**  
- `TAPICallOrPutFlagType`：`TAPICHAR`；`TAPI_CALLPUT_FLAG_CALL`/`PUT`/`NONE`。

#### 结构体（逐成员说明）

- **TapAPIApplicationInfo**：`AuthCode`（授权码）、`KeyOperationLogPath`（关键操作日志路径）。  
- **TapAPICommodity**：`ExchangeNo`、`CommodityType`、`CommodityNo`（品种编码）。  
- **TapAPIContract**：`Commodity`（品种）、`ContractNo1`/`StrikePrice1`/`CallOrPutFlag1`、`ContractNo2`/`StrikePrice2`/`CallOrPutFlag2`（合约/执行价/看涨看跌）。  
- **TapAPIExchangeInfo**：`ExchangeNo`、`ExchangeName`。  
- **TapAPIChangePasswordReq**：`OldPassword`、`NewPassword`。

---

### 2.2 TapAPIError.h

**路径**：`Tap/TapAPIError.h`  
**用途**：Tap API 内部与业务错误码常量定义。  
**依赖**：无。

**内容概要**：  
- 内部 API 错误：`TAPIERROR_SUCCEED=0`，`TAPIERROR_ConnectFail`、`LinkAuthFail`、`RepeatLogin`、`InputValueError` 等（约 -1～-22）。  
- 输入检查错误：`TAPIERROR_INPUTERROR_*`（如 TAPIYNFLAG、TAPILOGLEVEL、各类业务枚举错误，约 -10000～-12045）。  
- 断开原因：`TAPIERROR_DISCONNECT_*`（主动/被动、读写错误、超时、心跳失败等）。  
- 登录错误：`TAPIERROR_LOGIN_*`（用户、动态认证、密码、权限等）。  
- 管理/查询错误：用户信息、资金账号、合约等查询失败。  
- 交易错误：账号、合约、资金、订单类型、撤单、审核、改单等。  
- 行情错误：订阅数超限、权限、品种不存在等。  

所有项均为 `const int`，无类或结构体。

---

### 2.3 TapQuoteAPI.h

**路径**：`Tap/TapQuoteAPI.h`  
**用途**：Tap 行情 API 的回调接口与功能接口、以及 C 导出创建/销毁/配置函数。  
**依赖**：`TapQuoteAPIDataType.h`（含 `TapAPIApplicationInfo`、`TapAPIContract`、登录与行情结构体）。

**宏**：`_WIN32` 时 `TAP_CDECL` 为 `__cdecl`、`TAP_DLLEXPORT` 为 `__declspec(dllexport)`；否则为空。

#### 类 ITapQuoteAPINotify（行情回调，纯虚）

- **OnRspLogin**：登录响应（errorCode、TapAPIQuotLoginRspInfo）。  
- **OnAPIReady**：API 就绪，之后方可查询/订阅。  
- **OnDisconnect**：与服务器断开（reasonCode）。  
- **OnRspQryCommodity**：品种查询应答（sessionID、errorCode、isLast、TapAPIQuoteCommodityInfo）。  
- **OnRspQryContract**：合约查询应答。  
- **OnRspSubscribeQuote** / **OnRspUnSubscribeQuote**：订阅/退订行情应答。  
- **OnRtnQuote**：行情变化推送（TapAPIQuoteWhole）。

#### 类 ITapQuoteAPI（行情功能接口，纯虚）

- **SetAPINotify**：设置回调对象。  
- **SetHostAddress**：设置服务器 IP 与端口。  
- **Login**：登录（TapAPIQuoteLoginAuth）。  
- **Disconnect**：断开连接。  
- **QryCommodity**：查询品种。  
- **QryContract**：查询合约（可选条件）。  
- **SubscribeQuote** / **UnSubscribeQuote**：订阅/退订行情。

#### 全局 C 导出函数

- `CreateTapQuoteAPI(appInfo, iResult)`：创建行情 API。  
- `FreeTapQuoteAPI(apiObj)`：销毁 API。  
- `GetTapQuoteAPIVersion()`：版本字符串。  
- `SetTapQuoteAPIDataPath(path)`：数据/日志目录。  
- `SetTapQuoteAPILogLevel(level)`：日志级别。

---

### 2.4 TapQuoteAPIDataType.h

**路径**：`Tap/TapQuoteAPIDataType.h`  
**用途**：行情 API 使用的数据类型与结构体。  
**依赖**：`TapAPICommDef.h`；`#pragma pack(push, 1)`。

**类型**：`TAPIQPRICE`（行情价）、`TAPIQVOLUME`（量）、`TAPIQDIFF`（变化值）。

**结构体摘要**：  
- **TapAPIQuoteLoginAuth**：UserNo、ISModifyPassword、Password、NewPassword、QuoteTempPassword、ISDDA、DDASerialNo。  
- **TapAPIQuotLoginRspInfo**：UserNo、UserType、UserName、QuoteTempPassword、ReservedInfo、LastLoginIP/Prot/Time、TradeDate、LastSettleTime、StartTime、InitTime。  
- **TapAPIQuoteCommodityInfo**：Commodity、CommodityName、ContractSize、CommodityTickSize、CommodityDenominator 等。  
- **TapAPIQuoteContractInfo**：Contract、ContractType、QuoteUnderlyingContract、ContractName、ContractExpDate、LastTradeDate、FirstNoticeDate。  
- **TapAPIQuoteWhole**：合约、币种、交易状态、时间戳、昨收/昨结/昨持仓、开高低收、涨跌停、成交量/额、持仓、均价、买卖 20 档、隐含买卖、Delta、内外盘、换手率、市盈率、市值等（行情全文）。

---

### 2.5 TapTradeAPI.h

**路径**：`Tap/TapTradeAPI.h`  
**用途**：Tap 交易 API 的回调与功能接口、以及 C 导出函数。  
**依赖**：`TapTradeAPIDataType.h`。

#### 类 ITapTradeAPINotify（交易回调，纯虚）

- 连接与登录：**OnConnect**、**OnRspLogin**、**OnAPIReady**、**OnDisconnect**。  
- 用户：**OnRspChangePassword**、**OnRspSetReservedInfo**。  
- 账户与资金：**OnRspQryAccount**、**OnRspQryFund**、**OnRtnFund**。  
- 交易所与合约：**OnRspQryExchange**、**OnRspQryCommodity**、**OnRspQryContract**、**OnRtnContract**。  
- 委托与成交：**OnRtnOrder**、**OnRspOrderAction**、**OnRspQryOrder**、**OnRspQryOrderProcess**、**OnRspQryFill**、**OnRtnFill**。  
- 持仓与平仓：**OnRspQryPosition**、**OnRtnPosition**、**OnRspQryClose**、**OnRtnClose**、**OnRtnPositionProfit**。  
- 其他：**OnRspQryDeepQuote**、**OnRspQryExchangeStateInfo**、**OnRtnExchangeStateInfo**、**OnRtnReqQuoteNotice**、**OnRspUpperChannelInfo**、**OnRspAccountRentInfo**。

#### 类 ITapTradeAPI（交易功能接口，纯虚）

- **SetAPINotify**、**SetHostAddress**、**Login**、**Disconnect**。  
- **ChangePassword**、**HaveCertainRight**、**SetReservedInfo**。  
- **QryAccount**、**QryFund**、**QryExchange**、**QryCommodity**、**QryContract**。  
- **InsertOrder**、**CancelOrder**、**QryOrder**、**QryOrderProcess**、**QryFill**、**QryPosition**、**QryClose**。  
- **QryDeepQuote**、**QryExchangeStateInfo**、**QryUpperChannel**、**QryAccountRent**、**ActivateOrder**。

#### 全局 C 导出函数

- `CreateTapTradeAPI`、`FreeTapTradeAPI`、`GetTapTradeAPIVersion`、`SetTapTradeAPIDataPath`、`SetTapTradeAPILogLevel`。

---

### 2.6 TapTradeAPIDataType.h（摘要）

**路径**：`Tap/TapTradeAPIDataType.h`  
**行数**：约 1346 行。  
**用途**：交易 API 使用的枚举、typedef 与结构体（账号类型、委托类型、买卖/开平、订单状态、资金、委托、成交、持仓、平仓、费率等）。

**类型与常量**：如 `TAPIAccountType`（个人/机构）、`TAPIRightIDType`（权限编码）、`TAPIUserTypeType`（投资者/经纪人/交易员等）、`TAPIAccountState`、`TAPIOrderTypeType`（市价/限价/止损等）、`TAPIOrderSourceType`、`TAPITimeInForceType`、`TAPISideType`、`TAPIPositionEffectType` 等大量业务枚举；以及请求/响应/通知用结构体（如 TapAPITradeLoginAuth、TapAPIAccountInfo、TapAPIFundData、TapAPINewOrder、TapAPIOrderInfo、TapAPIFillInfo、TapAPIPositionInfo、TapAPICloseInfo 等）。  
完整列表见原文件；命名与 TapQuoteAPIDataType 风格一致，依赖 `TapAPICommDef.h`。

---

## 三、ITap 子目录

ITap 目录下部分文件与 Tap 目录**内容相同或高度相似**（如 TapAPICommDef.h、TapAPIError.h、TapQuoteAPI.h、TapQuoteAPIDataType.h），分析时可直接参考上述 Tap 对应小节。

**ITap 特有或扩展文件**：  
- **iTapAPICommDef.h**（278 行）：与 TapAPICommDef 类似，可能增加部分类型或常量。  
- **iTapAPIError.h**（581 行）：错误码更多，分类更细，与 TapAPIError 同一套体系。  
- **iTapTradeAPI.h**（1101 行）：交易回调与接口类，与 TapTradeAPI 设计一致，回调与请求更多（如更多查询与通知）。  
- **iTapTradeAPIDataType.h**（2030 行）：交易数据类型与结构体，为 TapTradeAPIDataType 的超集或另一版本。  

具体类名/方法名与 Tap 类似（如 ITapTradeAPINotify、ITapTradeAPI），仅命名空间或前缀为 iTap。若需逐行对照，可对比 ITap 与 Tap 同名字符串或符号。

---

## 四、Xtp 子目录

**路径与用途概览**：  
- **xtp_api_data_type.h**：基础数据类型与枚举（如 XTP_EXCHANGE_TYPE、行情/交易相关常量）。  
- **xtp_api_struct_common.h**：通用结构（如错误信息 XTPRI）。  
- **xtp_api_struct.h**：通过 include 聚合其它 struct 头文件（如 xtp_api_struct_common、xquote_api_struct、xoms_api_struct 等）。  
- **xtp_quote_api.h**：行情 API：回调类 `XTP::API::QuoteSpi`、接口类 `XTP::API::QuoteApi`（创建/登录/订阅/退订/查询等）。  
- **xtp_trader_api.h**：交易 API：回调类 `XTP::API::TraderSpi`、接口类 `XTP::API::TraderApi`（登录/报单/撤单/查询订单/成交/持仓/资金等）。  
- **xoms_api_struct.h** / **xoms_api_fund_struct.h**：OMS 订单与资金相关结构体。  
- **xquote_api_struct.h**：行情相关结构体（如行情数据、订单簿等）。

#### 类摘要（XTP 命名空间）

- **QuoteSpi**：OnDisconnected、OnError、OnSubMarketData/OnUnSubMarketData、OnDepthMarketData、OnSubOrderBook/OnUnSubOrderBook、OnOrderBook、OnSubTickByTick/OnUnSubTickByTick、OnTickByTick、全市场订阅/退订、OnQueryAllTickers、OnQueryTickersPriceInfo、期权全市场等。  
- **QuoteApi**：创建、登录、订阅/退订行情、订阅订单簿/逐笔、查询可交易合约与价格等。  
- **TraderSpi**：OnDisconnected、OnError、OnOrderEvent、OnTradeEvent、OnCancelOrderError、OnQueryOrder/Trade/Position/Asset、OnQueryStructuredFund、OnQueryFundTransfer、OnFundTransfer、OnQueryETF/ETFBasket、OnQueryIPOInfoList/IPOQuotaInfo、OnQueryOptionAuctionInfo 等。  
- **TraderApi**：登录、报单、撤单、查询订单/成交/持仓/资金、分级基金、资金划拨、ETF、新股申购、期权合约等。

Xtp 使用 `MD_API_EXPORT` / `TRADER_API_EXPORT` 与 CTP 类似的 Windows 导出宏；结构体类型名多为 XTP*、XTPRI、XTPST、XTPMD、XTPOB、XTPTBT 等，定义在 xtp_api_struct 所包含的各头文件中。

---

*文档按《APIs头文件分析工作计划》生成；Ctp 为逐文件详细分析，Tap 为完整分析，ITap/Xtp 为摘要与索引，便于定位到具体头文件与类。*
