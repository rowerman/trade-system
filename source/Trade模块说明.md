# Trade 模块说明

## 1. 模块概述

`source/trade/` 提供**持仓管理与风险管理**：OffsetConverter 根据合约是否为“净持仓”模式，将策略的“平仓”请求转换为交易所要求的“平今/平昨”组合（如 SHFE）；PositionHolding 维护每个 account+full_symbol 的多空、今昨、冻结及成本价，并在订单与成交回报驱动下更新。RiskManagerBase 与 PassThroughRiskManager 提供订单级风控接口（当前为透传）。

## 2. 目录结构

```
source/trade/
├── __init__.py
├── portfolio_manager.py  # OffsetConverter、PositionHolding
├── risk_manager_base.py # RiskManagerBase 抽象类
└── risk_manager.py      # PassThroughRiskManager 实现
```

## 3. 各文件功能与逻辑

### 3.1 `portfolio_manager.py`

- **OffsetConverter**  
  - 持有 main_engine（StrategyEngine 或具备 get_contract 的引擎）和 holdings：key 为 "acc.full_symbol"，value 为 PositionHolding。  
  - **is_convert_required(full_symbol)**：查合约的 net_position；若为 False（即“逐笔持仓”，如 SHFE），则需要做平仓转换；若为 True（净持仓）则不需要。  
  - **get_position_holding(acc, full_symbol)**：若尚无该 key 的 PositionHolding，则用 main_engine.get_contract(full_symbol) 创建 PositionHolding(acc, contract) 并放入 holdings。  
  - **update_position(position)**、**update_trade(trade)**、**update_order(order)**、**update_order_request(req)**：仅当 is_convert_required 为 True 时，取对应 PositionHolding 并调用其 update_*，用于同步柜台回报或回测成交，更新多空今昨与冻结。  
  - **convert_order_request(req, lock)**：若不需要转换则返回 [req]；若需要则调用 holding.convert_order_request_shfe(req)（非锁仓）或 convert_order_request_lock(req)（锁仓）。StrategyEngine 在下单前调用此方法，将一条“平仓”请求可能拆成“平今 + 平昨”两条请求发给柜台。

- **PositionHolding**  
  - 维护单 account+单合约的多空、今昨、冻结及成本价。  
  - 字段：long_pos、long_yd、long_td、long_price；short_pos、short_yd、short_td、short_price；long_pos_frozen、long_yd_frozen、long_td_frozen；short_pos_frozen、short_yd_frozen、short_td_frozen；active_orders（server_order_id → OrderData）；active_requests（reqid → OrderRequest）；last_price。  
  - **update_position(position)**：按 direction 更新多或空一侧的 volume、yd_volume、td、price。  
  - **update_order(order)**：用 order 的 clientID+client_order_id 作 reqid，若订单已结束则从 active_requests 移除；若订单仍活跃则加入 active_orders，否则移除；然后 calculate_frozen()。  
  - **update_order_request(req)**：将 req 放入 active_requests，再 calculate_frozen()。  
  - **update_trade(trade)**：根据 direction、offset（OPEN、CLOSETODAY、CLOSEYESTERDAY、CLOSE）更新 long_td/long_yd、short_td/short_yd；SHFE 下 CLOSE 按先平昨处理；并更新 long_price/short_price 为持仓成本。  
  - **calculate_frozen()**：遍历 active_orders 与 active_requests，按 offset、direction 累加平今/平昨/平仓的未成交量到对应的 *_frozen，保证 td_frozen 不超过 td、不足部分算到 yd_frozen。  
  - **convert_order_request_shfe(req)**：仅对平仓（offset!=OPEN）做转换。根据 direction 取可用多/空持仓与今仓；若请求量大于可用则直接返回 [req]；若全部用今仓可满足则返回一条 CombOffsetFlag=CloseToday 的请求；否则拆成“平今一部分 + 平昨剩余”两条请求（CTP 侧改 orderfield.CombOffsetFlag、VolumeTotalOriginal）。  
  - **convert_order_request_lock(req)**：锁仓逻辑：若有多/空今仓则改为“开仓”请求返回；若无今仓则根据昨仓可用与请求量，生成“平昨”和“开仓”组合，用于先平昨再开反向。

策略引擎在发单前对需要转换的合约调用 OffsetConverter.convert_order_request，得到一或多条 OrderRequest 再依次发送；回报的 Position、Order、Trade 会回写 OffsetConverter 与 PositionHolding，保证下次转换时数据一致。

### 3.2 `risk_manager_base.py`

- **RiskManagerBase**：抽象基类，仅定义 **order_in_compliance(original_order)**，返回通过风控的订单（或修改后的订单）。引擎在下单前调用，若需拦截可返回 None 或修改后订单。

### 3.3 `risk_manager.py`

- **PassThroughRiskManager(RiskManagerBase)**  
  - order_in_compliance(original_order)：直接返回 original_order，不做限制。  
  - passorder()、passquery()：内部有简单的“每秒订单数/查询数”计数，超过阈值时打印提示并返回 False，可用于限流（若引擎侧调用）；当前未在 strategy_engine 中强制使用。

## 4. 与其它模块的关系

- **engine**：StrategyEngine 持有 OffsetConverter，在 send_order 前做 convert_order_request；BacktestingEngine 直接使用 PositionHolding 维护持仓并 update_trade，无 OffsetConverter（回测可假设净持仓或按合约规则在引擎内处理）。  
- **common**：使用 Direction、Offset、Exchange、OrderRequest、OrderData、TradeData、PositionData、ContractData 等；api.ctp_constant 用于设置 CombOffsetFlag（平今/平昨）。

## 5. 小结

Trade 模块提供：**按交易所规则将“平仓”拆成“平今/平昨”的 OffsetConverter**、**维护多空今昨与冻结的 PositionHolding**、以及**可扩展的风控接口 RiskManagerBase**（当前实现为透传）。理解本模块即可掌握实盘下单前的开平转换逻辑与持仓状态的维护方式。
