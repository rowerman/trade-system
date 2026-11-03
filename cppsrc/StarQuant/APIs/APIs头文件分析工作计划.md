# APIs 子目录头文件(.h)详细分析 — 工作计划

## 一、目标与范围

- **目标**：对 `cppsrc/StarQuant/APIs` 下所有 `.h` 文件进行详细分析，整理出各个类/结构体的细节（成员变量、公私有函数等），并对每行代码给出解释。
- **范围**：Ctp、Tap、ITap、Xtp 四个子目录下的全部头文件（共 25 个 .h 文件，不含 ITap 与 Tap 重复文件则约 21 个独立文件）。
- **输出**：分析结果保存到 API 目录下的 `APIs头文件详细分析.md`。

## 二、文件清单与规模（按子目录）

| 子目录 | 文件名 | 行数 | 说明 |
|--------|--------|------|------|
| **Ctp** | DataCollect.h | 24 | 数据采集接口 |
| | ThostFtdcMdApi.h | 162 | 行情 API 与 SPI |
| | ThostFtdcTraderApi.h | 739 | 交易 API 与 SPI |
| | ThostFtdcUserApiDataType.h | 6663 | 数据类型定义（typedef/enum） |
| | ThostFtdcUserApiStruct.h | 9492 | 业务结构体定义 |
| **Tap** | TapAPICommDef.h | 230 | 公共定义 |
| | TapAPIError.h | 363 | 错误码 |
| | TapQuoteAPI.h | 255 | 行情 API |
| | TapQuoteAPIDataType.h | 154 | 行情数据类型 |
| | TapTradeAPI.h | 643 | 交易 API |
| | TapTradeAPIDataType.h | 1346 | 交易数据类型 |
| **ITap** | iTapAPICommDef.h | 278 | 公共定义 |
| | iTapAPIError.h | 581 | 错误码 |
| | iTapTradeAPI.h | 1101 | 交易 API |
| | iTapTradeAPIDataType.h | 2030 | 交易数据类型（注：与 Tap 可能部分重复） |
| | TapQuoteAPI.h / TapQuoteAPIDataType.h / TapAPICommDef.h / TapAPIError.h | 同 Tap | 与 Tap 目录重复，分析时合并说明 |
| **Xtp** | xtp_api_data_type.h | 374 | 数据类型 |
| | xtp_api_struct_common.h | 40 | 通用结构 |
| | xtp_api_struct.h | 15 | 结构引用 |
| | xtp_quote_api.h | 424 | 行情 API |
| | xtp_trader_api.h | 395 | 交易 API |
| | xoms_api_struct.h | 543 | OMS 结构 |
| | xoms_api_fund_struct.h | 42 | OMS 资金结构 |
| | xquote_api_struct.h | 314 | 行情结构 |

**超大文件处理策略**：  
- `ThostFtdcUserApiDataType.h`（6663 行）、`ThostFtdcUserApiStruct.h`（9492 行）：采用“文件说明 + 类型/结构命名与用途列表 + 代表性样例逐行解释”的方式，避免对数千行逐行重复性说明。  
- 其他文件：按“逐段/逐类 + 关键行解释”的方式做到尽量详细。

## 三、分析模板（每个文件）

对每个 .h 文件按以下结构整理：

1. **文件概览**
   - 路径、用途、依赖（#include）、宏/条件编译（#ifdef、导出宏等）。
2. **预处理与宏**
   - 头文件保护、DLL 导出宏、平台相关宏等，逐行或逐块说明。
3. **类型定义（typedef / using / enum）**
   - 类型名、含义、典型用法；对超大文件为列表 + 代表性样例逐行解释。
4. **结构体 / 类**
   - 名称、作用；
   - 成员变量：名称、类型、含义（对应“每行代码解释”）；
   - 公有/保护/私有函数：签名、参数含义、返回值、行为简述。
5. **全局函数或静态接口**
   - 若有，列出签名与说明。

## 四、执行阶段

| 阶段 | 内容 | 产出 |
|------|------|------|
| **阶段 1** | 编写本工作计划文档 | 本文档 |
| **阶段 2** | Ctp 子目录：DataCollect.h、ThostFtdcMdApi.h、ThostFtdcTraderApi.h 全文分析；ThostFtdcUserApiDataType.h、ThostFtdcUserApiStruct.h 按“说明+列表+样例”分析 | 写入最终 md 的 Ctp 部分 |
| **阶段 3** | Tap 子目录：6 个 .h 文件按模板分析 | 写入最终 md 的 Tap 部分 |
| **阶段 4** | ITap 子目录：与 Tap 不重复的部分（iTap*）重点分析，重复文件注明“同 Tap” | 写入最终 md 的 ITap 部分 |
| **阶段 5** | Xtp 子目录：8 个 .h 文件按模板分析 | 写入最终 md 的 Xtp 部分 |
| **阶段 6** | 汇总、目录、交叉引用、统一格式后写入 `APIs头文件详细分析.md` | 最终文档 |

## 五、执行顺序与依赖

- 先完成 **Ctp**（项目直接使用的 CTP 接口），再 **Tap/ITap**，最后 **Xtp**。
- 同一子目录内：先分析 CommDef/DataType/Struct 等基础头文件，再分析 *Api.h，便于在分析 API 时引用已整理的类型与结构。

## 六、文档约定

- 使用 Markdown，标题层级：`#` 总标题，`##` 子目录，`###` 文件，`####` 类/结构体，`#####` 成员或方法分组。
- 代码引用使用 `` `类型名` `` 或代码块。
- 对“每行代码的解释”：在类/结构体内部以“行号 + 代码 + 说明”或表格形式呈现；超大文件仅在样例中采用逐行形式。

---

**文档创建时间**：2025-02-07  
**执行状态**：阶段 1～6 已完成。分析结果已写入 `APIs头文件详细分析.md`（同目录下）。
