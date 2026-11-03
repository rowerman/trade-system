# API 模块说明

## 1. 模块概述

`source/api/` 提供与**交易/行情接口**相关的常量定义，主要面向中国期货的 **CTP（上期技术）** 接口。这些常量用于在 Python 层与 CTP API 的枚举值保持一致，便于策略、引擎、交易模块正确填单和解析回报。

## 2. 目录结构

```
source/api/
├── __init__.py      # 空包标识
└── ctp_constant.py  # CTP 相关常量（THOST_FTDC_*）
```

## 3. 文件说明

### 3.1 `ctp_constant.py`

单文件，包含大量以 `THOST_FTDC_` 为前缀的字符串常量，对应 CTP 接口中的枚举/字符取值。按功能大致分为：

| 类别 | 示例常量 | 含义 |
|------|----------|------|
| 证件类型 | `THOST_FTDC_ICT_IDCard` | 身份证等证件类型 |
| 连接/流控 | `THOST_FTDC_TCS_Connected` | 交易连接状态 |
| 报单状态 | `THOST_FTDC_OST_AllTraded` | 全部成交等订单状态 |
| 有效期类型 | `THOST_FTDC_TC_GFD` | 当日有效等 |
| 成交量类型 | `THOST_FTDC_VC_AV`、`THOST_FTDC_VC_CV` | 任意数量、全部数量（FOK） |
| 买卖方向 | `THOST_FTDC_D_Buy`、`THOST_FTDC_D_Sell` | 买/卖 |
| 开平标志 | `THOST_FTDC_OF_Open`、`THOST_FTDC_OF_Close`、`THOST_FTDC_OF_CloseToday`、`THOST_FTDC_OF_CloseYesterday` | 开仓/平仓/平今/平昨 |
| 持仓类型 | `THOST_FTDC_PT_Net`、`THOST_FTDC_PD_Long`、`THOST_FTDC_PD_Short` | 净持仓、多头、空头 |
| 价格类型 | `THOST_FTDC_OPT_AnyPrice`、`THOST_FTDC_OPT_LimitPrice` | 市价、限价 |
| 投机套保 | `THOST_FTDC_HF_Speculation` | 投机 |
| 触发条件 | `THOST_FTDC_CC_Immediately` | 立即等 |

本模块**不包含**任何业务逻辑或网络通信，仅作为“常量表”被以下模块引用：

- **common.constant**：将 CTP 方向/开平等映射为内部枚举（如 `Direction`、`Offset`）。
- **strategy.strategy_base**：构造 `CtpOrderField` 时使用（如 `OrderPriceType`、`Direction`、`CombOffsetFlag`、`TimeCondition`、`VolumeCondition`、`ContingentCondition`）。
- **trade.portfolio_manager**：平仓时设置 `CombOffsetFlag`（平今/平昨等）。

## 4. 使用注意

- 新增或修改常量时需与当前使用的 CTP 接口文档一致，避免与 C++ 端或柜台解析不一致。
- 仅 `ctp_constant.py` 被项目引用，若将来接入 TAP、XTP 等，可在此目录增加对应 `*_constant.py`，再由 common/strategy 做统一封装。

## 5. 小结

API 模块是**接口常量层**：集中维护 CTP 的枚举与字符取值，供 common、strategy、trade 等模块在报单、持仓转换和状态解析时使用，保证与 CTP 协议一致。
