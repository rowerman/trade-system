# StarQuant Python 源码模块说明索引

本目录（`source/`）包含 StarQuant 项目 Python 部分的全部代码，采用事件驱动的量化交易架构。各子模块说明文档如下：

| 子模块 | 说明文档 | 主要职责 |
|--------|----------|----------|
| **api** | [API模块说明.md](API模块说明.md) | CTP 等交易接口常量定义 |
| **common** | [Common模块说明.md](Common模块说明.md) | 配置、常量、数据结构、消息队列客户端、工具函数 |
| **data** | [Data模块说明.md](Data模块说明.md) | 数据库抽象、Bar/Tick 存储与加载、RQData、K线生成与指标 |
| **engine** | [Engine模块说明.md](Engine模块说明.md) | 事件引擎、策略引擎、回测引擎、录播引擎 |
| **gui** | [GUI模块说明.md](GUI模块说明.md) | PyQt5 图形界面、行情/订单/持仓/回测等窗口 |
| **strategy** | [Strategy模块说明.md](Strategy模块说明.md) | 策略基类、下单封装、信号与组合策略 |
| **trade** | [Trade模块说明.md](Trade模块说明.md) | 持仓转换、资金管理、风险管理 |

## 模块依赖关系概览

```
gui → common, engine, trade
engine → common, data, strategy, trade
strategy → common, api
trade → common, api, engine
data → common
api → (无项目内依赖)
common → api (仅 constant 引用 ctp_constant)
```

## 使用说明

- 实盘/模拟：通过 GUI 启动，由 **engine** 中的 StrategyEngine/RecorderEngine 与 C++ 端通过 nanomsg 通信。
- 回测：使用 **engine.backtest_engine** 的 `BacktestingEngine`，搭配 **strategy** 中继承 `StrategyBase` 的策略类。
- 数据：历史 K 线/Tick 由 **data** 模块从数据库或 RQData 加载；录播由 **engine.recorder_engine** 写入 **data** 的 database_manager。

详细功能与逻辑请参阅各子模块对应的说明文档。
