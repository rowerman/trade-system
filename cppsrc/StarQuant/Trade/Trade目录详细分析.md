# Trade目录代码详细分析

## 目录
1. [calc.h / calc.cpp - 盈亏计算函数](#1-calch--calccpp)
2. [marginmanager.h / marginmanager.cpp - 保证金管理类](#2-marginmanagerh--marginmanagercpp)
3. [ordermanager.h / ordermanager.cpp - 订单管理器类](#3-ordermanagerh--ordermanagercpp)
4. [portfoliomanager.h / portfoliomanager.cpp - 投资组合管理器类](#4-portfoliomanagerh--portfoliomanagercpp)
5. [riskmanager.h / riskmanager.cpp - 风险管理器类](#5-riskmanagerh--riskmanagercpp)

---

## 1. calc.h / calc.cpp

### 1.1 文件概述
`calc.h` 和 `calc.cpp` 提供了盈亏计算相关的函数，位于 `StarQuant::Calc` 命名空间下。这些函数用于计算持仓的浮动盈亏（Open P&L）和已实现盈亏（Closed P&L）。

### 1.2 calc.h 详细分析

#### 1.2.1 头文件保护
```cpp
#ifndef CPPSRC_STARQUANT_TRADE_CALC_H
#define CPPSRC_STARQUANT_TRADE_CALC_H
```
- **第3行**: 头文件保护宏定义开始，防止重复包含
- **第4行**: 定义保护宏 `CPPSRC_STARQUANT_TRADE_CALC_H`

#### 1.2.2 包含的头文件
```cpp
#include <Common/datastruct.h>
```
- **第6行**: 包含通用数据结构头文件，用于访问 `Position` 和 `Fill` 结构体

#### 1.2.3 命名空间声明
```cpp
namespace StarQuant {
    namespace Calc {
```
- **第8行**: 外层命名空间 `StarQuant`，整个项目的命名空间
- **第9行**: 内层命名空间 `Calc`，专门用于计算相关的函数

#### 1.2.4 OpenPT 函数（重载版本1）
```cpp
double OpenPT(double LastTrade, double AvgPrice, bool Side);
```
- **函数名**: `OpenPT` - Open Profit per Tick，每单位浮动盈亏
- **返回值**: `double` - 每单位的浮动盈亏金额
- **参数**:
  - `LastTrade`: `double` - 最新成交价
  - `AvgPrice`: `double` - 持仓平均成本价
  - `Side`: `bool` - 持仓方向，true表示多头，false表示空头
- **功能**: 计算每单位的浮动盈亏，不考虑持仓数量

#### 1.2.5 OpenPT 函数（重载版本2）
```cpp
double OpenPT(double LastTrade, double AvgPrice, int32_t PosSize);
```
- **函数名**: `OpenPT` - Open Profit per Tick（重载版本）
- **返回值**: `double` - 每单位的浮动盈亏金额
- **参数**:
  - `LastTrade`: `double` - 最新成交价
  - `AvgPrice`: `double` - 持仓平均成本价
  - `PosSize`: `int32_t` - 持仓数量，正数表示多头，负数表示空头，0表示无持仓
- **功能**: 根据持仓数量自动判断方向，计算每单位的浮动盈亏

#### 1.2.6 OpenPL 函数
```cpp
double OpenPL(double LastTrade, double AvgPrice, int32_t PosSizeMultiplier);
```
- **函数名**: `OpenPL` - Open Profit and Loss，总浮动盈亏
- **返回值**: `double` - 总浮动盈亏金额
- **参数**:
  - `LastTrade`: `double` - 最新成交价
  - `AvgPrice`: `double` - 持仓平均成本价
  - `PosSizeMultiplier`: `int32_t` - 持仓数量（带方向），正数表示多头，负数表示空头
- **功能**: 计算整个持仓的总浮动盈亏，考虑持仓数量

#### 1.2.7 ClosePT 函数
```cpp
double ClosePT(const Position& existing, const Fill& adjust);
```
- **函数名**: `ClosePT` - Close Profit per Tick，每单位已实现盈亏
- **返回值**: `double` - 每单位的已实现盈亏金额
- **参数**:
  - `existing`: `const Position&` - 现有持仓的引用
  - `adjust`: `const Fill&` - 成交回报的引用，用于调整持仓
- **功能**: 计算平仓时每单位的已实现盈亏，不考虑平仓数量

#### 1.2.8 ClosePL 函数
```cpp
double ClosePL(const Position& existing, const Fill& adjust, int32_t multiplier = 1);
```
- **函数名**: `ClosePL` - Close Profit and Loss，总已实现盈亏
- **返回值**: `double` - 总已实现盈亏金额
- **参数**:
  - `existing`: `const Position&` - 现有持仓的引用
  - `adjust`: `const Fill&` - 成交回报的引用
  - `multiplier`: `int32_t` - 合约乘数，默认为1
- **功能**: 计算平仓的总已实现盈亏，考虑平仓数量和合约乘数

#### 1.2.9 命名空间和头文件保护结束
```cpp
    }  // namespace Calc
}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_CALC_H
```
- **第20行**: 关闭 `Calc` 命名空间
- **第21行**: 关闭 `StarQuant` 命名空间
- **第23行**: 头文件保护宏结束

### 1.3 calc.cpp 详细分析

#### 1.3.1 版权声明和头文件包含
```cpp
/*****************************************************************************
 * Copyright [2019] 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <Trade/calc.h>
#include <Common/datastruct.h>
#include <cmath>
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 包含自己的头文件 `calc.h`
- **第17行**: 包含数据结构头文件，确保 `Position` 和 `Fill` 类型可用
- **第18行**: 包含数学库 `<cmath>`，虽然当前代码未直接使用，但为未来扩展预留

#### 1.3.2 命名空间和 using 声明
```cpp
using namespace std;

namespace StarQuant {
    namespace Calc {
```
- **第21行**: 使用标准命名空间，简化代码
- **第23行**: 打开 `StarQuant` 命名空间
- **第24行**: 打开 `Calc` 命名空间

#### 1.3.3 OpenPT 函数实现（版本1）
```cpp
double OpenPT(double LastTrade, double AvgPrice, bool Side) {
    return Side ? LastTrade - AvgPrice : AvgPrice - LastTrade;
}
```
- **第26行**: 函数定义开始
- **第27行**: 使用三元运算符计算浮动盈亏
  - 如果 `Side` 为 `true`（多头）: 盈亏 = 最新价 - 成本价（价格上涨盈利）
  - 如果 `Side` 为 `false`（空头）: 盈亏 = 成本价 - 最新价（价格下跌盈利）
- **第28行**: 函数结束

**计算逻辑说明**:
- 多头持仓：价格上涨时盈利，所以是 `LastTrade - AvgPrice`
- 空头持仓：价格下跌时盈利，所以是 `AvgPrice - LastTrade`

#### 1.3.4 OpenPT 函数实现（版本2）
```cpp
double OpenPT(double LastTrade, double AvgPrice, int32_t PosSize) {
    return (PosSize == 0) ? 0 : OpenPT(LastTrade, AvgPrice, PosSize > 0);
}
```
- **第30行**: 函数定义开始
- **第31行**: 
  - 如果 `PosSize == 0`（无持仓），返回 0
  - 否则调用第一个版本的 `OpenPT`，通过 `PosSize > 0` 将持仓数量转换为布尔方向
    - `PosSize > 0` 为 `true` 表示多头
    - `PosSize < 0` 为 `false` 表示空头
- **第32行**: 函数结束

**设计说明**: 这个重载版本提供了更便捷的接口，可以直接传入持仓数量而不需要手动判断方向。

#### 1.3.5 OpenPL 函数实现
```cpp
double OpenPL(double LastTrade, double AvgPrice, int32_t PosSizeMultiplier) {
    return PosSizeMultiplier * (LastTrade - AvgPrice);
}
```
- **第34行**: 函数定义开始
- **第35行**: 计算总浮动盈亏
  - `(LastTrade - AvgPrice)` 是每单位的盈亏
  - `PosSizeMultiplier` 是持仓数量（带方向）
  - 两者相乘得到总盈亏
  - 注意：`PosSizeMultiplier` 的正负号已经包含了方向信息
    - 正数（多头）: `PosSizeMultiplier * (LastTrade - AvgPrice)`，价格上涨时为正
    - 负数（空头）: `PosSizeMultiplier * (LastTrade - AvgPrice)`，价格下跌时为正（因为乘数为负）
- **第36行**: 函数结束

**计算示例**:
- 多头持仓100股，成本价10元，最新价12元: `100 * (12 - 10) = 200` 元盈利
- 空头持仓100股，成本价10元，最新价8元: `-100 * (8 - 10) = 200` 元盈利

#### 1.3.6 ClosePT 函数实现
```cpp
double ClosePT(Position& existing, Fill& adjust) {
    if (existing.size_ == 0) return 0; // nothing to close
    if ((existing.size_ > 0) == (adjust.tradeSize_ > 0)) return 0; // if we're adding, nothing to close
    return (existing.size_ > 0) ? adjust.tradePrice_ - existing.avgPrice_ : existing.avgPrice_ - adjust.tradePrice_;   // if long, sell high = profit; if short, sell low = profit
}
```
- **第38行**: 函数定义开始，注意参数不是 `const` 引用（与头文件声明不一致，可能是实现细节）
- **第39行**: 检查现有持仓数量
  - 如果 `existing.size_ == 0`，没有持仓可平，返回 0
- **第40行**: 检查是否为开仓操作
  - `(existing.size_ > 0) == (adjust.tradeSize_ > 0)` 判断持仓方向和成交方向是否相同
  - 如果相同，说明是加仓操作（不是平仓），返回 0
  - 例如：持有多头（size_ > 0）且买入（tradeSize_ > 0），这是加仓
  - 例如：持有空头（size_ < 0）且卖出（tradeSize_ < 0），这也是加仓
- **第41行**: 计算平仓盈亏
  - 如果 `existing.size_ > 0`（多头持仓）:
    - 平仓时卖出，盈亏 = 卖出价 - 成本价
    - `adjust.tradePrice_ - existing.avgPrice_`
  - 如果 `existing.size_ < 0`（空头持仓）:
    - 平仓时买入，盈亏 = 成本价 - 买入价
    - `existing.avgPrice_ - adjust.tradePrice_`
- **第42行**: 函数结束

**平仓逻辑说明**:
- 多头平仓：卖出价格高于成本价时盈利
- 空头平仓：买入价格低于成本价时盈利

#### 1.3.7 ClosePL 函数实现
```cpp
double ClosePL(Position& existing, Fill& adjust, int32_t multiplier) {
    int32_t closedsize = std::abs(adjust.tradeSize_) > std::abs(existing.size_) ? std::abs(existing.size_) : std::abs(adjust.tradeSize_);   // choose the smaller one
    return ClosePT(existing, adjust) * closedsize * multiplier;
}
```
- **第44行**: 函数定义开始
- **第45行**: 计算实际平仓数量
  - `std::abs(adjust.tradeSize_)` 是成交数量的绝对值
  - `std::abs(existing.size_)` 是持仓数量的绝对值
  - 取两者中的较小值作为实际平仓数量
  - 原因：平仓数量不能超过持仓数量，也不能超过成交数量
- **第46行**: 计算总已实现盈亏
  - `ClosePT(existing, adjust)` 获取每单位的盈亏
  - `closedsize` 是实际平仓数量
  - `multiplier` 是合约乘数（期货等衍生品需要）
  - 三者相乘得到总盈亏
- **第47行**: 函数结束

**计算示例**:
- 持仓100股（多头），成本10元，平仓50股，价格12元，乘数1:
  - `closedsize = min(50, 100) = 50`
  - `ClosePT = 12 - 10 = 2` 元/股
  - `ClosePL = 2 * 50 * 1 = 100` 元

#### 1.3.8 命名空间结束
```cpp
    }  // namespace Calc
}  // namespace StarQuant
```
- **第48行**: 关闭 `Calc` 命名空间
- **第49行**: 关闭 `StarQuant` 命名空间

---

## 2. marginmanager.h / marginmanager.cpp

### 2.1 文件概述
`marginmanager.h` 和 `marginmanager.cpp` 定义了 `MarginManager` 类，用于管理保证金。目前该类为空实现，可能是预留接口或未完成的功能。

### 2.2 marginmanager.h 详细分析

#### 2.2.1 版权声明和头文件保护
```cpp
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

#ifndef CPPSRC_STARQUANT_TRADE_MARGINMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_MARGINMANAGER_H_
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 头文件保护宏定义开始
- **第17行**: 定义保护宏 `CPPSRC_STARQUANT_TRADE_MARGINMANAGER_H_`

#### 2.2.2 包含的头文件
```cpp
#include <string>
#include <sstream>
#include <map>
#include <regex>

using std::string;
```
- **第19行**: 包含字符串类 `<string>`
- **第20行**: 包含字符串流类 `<sstream>`，用于字符串格式化
- **第21行**: 包含映射容器 `<map>`，可能用于存储保证金信息
- **第22行**: 包含正则表达式库 `<regex>`，可能用于符号匹配
- **第24行**: 使用标准命名空间的 `string` 类型

#### 2.2.3 MarginManager 类声明
```cpp
namespace StarQuant {
class MarginManager {
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_MARGINMANAGER_H_
```
- **第26行**: 打开 `StarQuant` 命名空间
- **第27行**: `MarginManager` 类声明开始
- **第28行**: 类体为空，没有任何成员变量或函数
- **第29行**: 类声明结束
- **第31行**: 关闭 `StarQuant` 命名空间
- **第33行**: 头文件保护宏结束

**说明**: 这是一个空类，可能是：
1. 预留接口，待后续实现
2. 功能已移除但保留类结构
3. 在其他地方有特殊用途

### 2.3 marginmanager.cpp 详细分析

#### 2.3.1 版权声明和头文件包含
```cpp
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
#include <Trade/marginmanager.h>

namespace StarQuant {

}

```
- **第1-14行**: Apache License 2.0 版权声明
- **第15行**: 包含自己的头文件 `marginmanager.h`
- **第17行**: 打开 `StarQuant` 命名空间
- **第19行**: 命名空间为空，没有任何实现
- **第21行**: 关闭 `StarQuant` 命名空间

**说明**: 实现文件也是空的，与头文件一致。

---

## 3. ordermanager.h / ordermanager.cpp

### 3.1 文件概述
`ordermanager.h` 和 `ordermanager.cpp` 实现了 `OrderManager` 类，采用单例模式，负责管理所有订单的生命周期，包括订单跟踪、状态更新、查询等功能。

### 3.2 ordermanager.h 详细分析

#### 3.2.1 版权声明和头文件保护
```cpp
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

#ifndef CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 头文件保护宏定义开始
- **第17行**: 定义保护宏 `CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_`

#### 3.2.2 包含的头文件
```cpp
#include <Common/datastruct.h>
#include <Common/logger.h>
#include <string>
#include <map>
#include <mutex>
#include <atomic>

using namespace std;
```
- **第19行**: 包含数据结构头文件，使用 `Order`、`Fill` 等类型
- **第20行**: 包含日志头文件，使用 `SQLogger` 进行日志记录
- **第21行**: 包含字符串类
- **第22行**: 包含映射容器，用于存储订单
- **第23行**: 包含互斥锁，用于线程安全
- **第24行**: 包含原子类型，用于线程安全的计数（虽然当前未使用）
- **第26行**: 使用标准命名空间

#### 3.2.3 注释掉的全局变量声明
```cpp
    //extern int64_t m_serverOrderId;	  // unique order id on server side defined in ordermanager.cpp. Every broker has its own id;
    //extern std::mutex oid_mtx;  // mutex for increasing order id; defined in ordermanager.cpp
    //extern std::mutex orderStatus_mtx;  // mutex for changing order status; defined in ordermanager.cpp
```
- **第29行**: 注释掉的服务器端订单ID全局变量声明
- **第30行**: 注释掉的订单ID互斥锁声明
- **第31行**: 注释掉的订单状态互斥锁声明（实际在 `datastruct.h` 中定义）

**说明**: 这些变量可能已移至其他位置或不再使用。

#### 3.2.4 OrderManager 类声明

##### 3.2.4.1 构造函数和析构函数
```cpp
class OrderManager {
 public:
    OrderManager();
    ~OrderManager();  // release all the orders
```
- **第34行**: `OrderManager` 类声明开始
- **第35行**: `public` 访问控制符
- **第36行**: 默认构造函数声明
- **第37行**: 析构函数声明，注释说明会释放所有订单

##### 3.2.4.2 单例模式相关成员
```cpp
    static OrderManager* pinstance_;
    static mutex instancelock_;
    static OrderManager& instance();
```
- **第38行**: 静态成员变量 `pinstance_`，指向单例实例的指针
- **第39行**: 静态互斥锁 `instancelock_`，用于保护单例创建的线程安全
- **第40行**: 静态成员函数 `instance()`，获取单例实例的引用

**设计模式**: 采用双重检查锁定（Double-Checked Locking）的单例模式。

##### 3.2.4.3 成员变量
```cpp
    // std::atomic_int _count = { 0 };
    std::shared_ptr<SQLogger> logger;
    int32_t _count = 0;
    std::map<int64_t, std::shared_ptr<Order>> orders_;
    std::map<int64_t, int64_t> fills_;       // signed filled size
    std::map<int64_t, bool> cancels_;    // if cancelled
    mutex wlock;
```
- **第42行**: 注释掉的原子计数变量（可能计划使用但未实现）
- **第43行**: 日志记录器智能指针，用于记录操作日志
- **第44行**: `_count` 计数器，记录订单数量（非线程安全版本）
- **第45行**: `orders_` 映射表，键为服务器订单ID（`int64_t`），值为订单智能指针
- **第46行**: `fills_` 映射表，键为订单ID，值为已成交数量（带符号，正数表示买入，负数表示卖出）
- **第47行**: `cancels_` 映射表，键为订单ID，值为是否已取消的布尔值
- **第48行**: `wlock` 写锁，用于保护共享数据的写入操作

**数据结构说明**:
- 使用 `std::shared_ptr<Order>` 管理订单生命周期，避免内存泄漏
- `fills_` 使用带符号整数，可以表示买入（正）和卖出（负）的成交

##### 3.2.4.4 重置函数
```cpp
    void reset();
```
- **第49行**: `reset()` 函数，重置所有订单数据

##### 3.2.4.5 订单跟踪函数
```cpp
    void trackOrder(std::shared_ptr<Order> o);  // put order under track
    void gotOrder(int64_t oid);  // order acknowledged
    void gotFill(const Fill& fill);
    void gotCancel(int64_t oid);
```
- **第51行**: `trackOrder()` 将订单加入跟踪列表
- **第52行**: `gotOrder()` 订单被确认（acknowledged）
- **第53行**: `gotFill()` 收到成交回报
- **第54行**: `gotCancel()` 收到取消回报

##### 3.2.4.6 订单查询函数
```cpp
    std::shared_ptr<Order> retrieveOrderFromServerOrderId(int64_t oid);
    std::shared_ptr<Order> retrieveOrderFromSourceAndClientOrderId(int32_t source, int64_t oid);
    std::shared_ptr<Order> retrieveOrderFromOrderNo(const string& ono);
    std::shared_ptr<Order> retrieveOrderFromAccAndBrokerOrderId(const string& acc, int32_t oid);		
    std::shared_ptr<Order> retrieveOrderFromAccAndLocalNo(const string& acc, const string& ono);
    // std::shared_ptr<Order> retrieveOrderFromMatchNo(string fno);
    vector<std::shared_ptr<Order>> retrieveOrder(const string& fullsymbol);
    vector<std::shared_ptr<Order>> retrieveNonFilledOrderPtr();
    vector<std::shared_ptr<Order>> retrieveNonFilledOrderPtr(const string& fullsymbol);
    vector<int64_t> retrieveNonFilledOrderId();
    vector<int64_t> retrieveNonFilledOrderId(const string& fullsymbol);
```
- **第55行**: 根据服务器订单ID查询订单
- **第56行**: 根据客户端ID和客户端订单ID查询订单
- **第57行**: 根据订单号查询订单
- **第58行**: 根据账户和券商订单ID查询订单
- **第59行**: 根据账户和本地订单号查询订单
- **第60行**: 注释掉的根据成交编号查询函数
- **第61行**: 根据合约代码查询所有相关订单（返回向量）
- **第62行**: 查询所有未完全成交的订单（返回订单指针向量）
- **第63行**: 查询指定合约的未完全成交订单（返回订单指针向量）
- **第64行**: 查询所有未完全成交的订单ID（返回ID向量）
- **第65行**: 查询指定合约的未完全成交订单ID（返回ID向量）

##### 3.2.4.7 状态查询函数
```cpp
    bool isEmpty();
    bool isTracked(int64_t oid);
    bool isCompleted(int64_t oid);  // either filled or canceled
    bool hasPendingOrders();  // is all orders either filled or canceled?
```
- **第67行**: `isEmpty()` 检查订单列表是否为空
- **第68行**: `isTracked()` 检查订单是否在跟踪列表中
- **第69行**: `isCompleted()` 检查订单是否已完成（已成交或已取消）
- **第70行**: `hasPendingOrders()` 检查是否有待处理的订单

#### 3.2.5 类结束
```cpp
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_ORDERMANAGER_H_
```
- **第71行**: 类声明结束
- **第73行**: 关闭 `StarQuant` 命名空间
- **第75行**: 头文件保护宏结束

### 3.3 ordermanager.cpp 详细分析

#### 3.3.1 版权声明和头文件包含
```cpp
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

#include <Trade/ordermanager.h>
#include <Trade/portfoliomanager.h>
#include <Common/logger.h>
#include <Common/datastruct.h>
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 包含自己的头文件
- **第17行**: 包含 `PortfolioManager` 头文件，用于在成交时调整持仓
- **第18行**: 包含日志头文件
- **第19行**: 包含数据结构头文件，使用 `OrderStatus`、`Fill`、`Order` 等类型

#### 3.3.2 注释和静态成员初始化
```cpp
namespace StarQuant {
    // extern int64_t m_serverOrderId ;    // unique order id on server side defined in ordermanager.cpp. Every broker has its own id;
    // extern std::mutex oid_mtx;			 // mutex for increasing order id
    // extern std::mutex orderStatus_mtx;  // mutex for changing order status

OrderManager* OrderManager::pinstance_ = nullptr;
mutex OrderManager::instancelock_;
```
- **第20行**: 打开 `StarQuant` 命名空间
- **第21-23行**: 注释掉的全局变量说明
- **第25行**: 初始化静态成员 `pinstance_` 为 `nullptr`
- **第26行**: 初始化静态互斥锁 `instancelock_`（使用默认构造函数）

#### 3.3.3 构造函数实现
```cpp
OrderManager::OrderManager() : _count(0) {
    // set up stocks from config
    reset();
    logger = SQLogger::getLogger("SYS");
}
```
- **第28行**: 构造函数定义，使用初始化列表将 `_count` 初始化为 0
- **第29行**: 注释说明可能从配置设置股票信息
- **第30行**: 调用 `reset()` 初始化所有数据成员
- **第31行**: 获取系统日志记录器，名称为 "SYS"

#### 3.3.4 析构函数实现
```cpp
OrderManager::~OrderManager() {
    // release all the orders
}
```
- **第34行**: 析构函数定义
- **第35行**: 注释说明释放所有订单
- **第36行**: 实际为空实现，因为使用 `shared_ptr` 管理订单，会自动释放

#### 3.3.5 instance() 单例函数实现
```cpp
OrderManager& OrderManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new OrderManager();
        }
    }
    return *pinstance_;
}
```
- **第38行**: `instance()` 函数定义，返回单例引用
- **第39行**: 第一次检查，如果实例不存在
- **第40行**: 获取互斥锁，使用 `lock_guard` 自动管理锁的生命周期
- **第41行**: 第二次检查（双重检查锁定模式），防止多线程同时创建实例
- **第42行**: 创建新的 `OrderManager` 实例
- **第45行**: 返回实例的引用

**线程安全说明**: 使用双重检查锁定确保线程安全，避免不必要的锁竞争。

#### 3.3.6 reset() 函数实现
```cpp
void OrderManager::reset() {
    orders_.clear();
    fills_.clear();
    cancels_.clear();

    _count = 0;
}
```
- **第48行**: `reset()` 函数定义
- **第49行**: 清空订单映射表
- **第50行**: 清空成交映射表
- **第51行**: 清空取消映射表
- **第53行**: 重置计数器为 0

#### 3.3.7 trackOrder() 函数实现
```cpp
void OrderManager::trackOrder(std::shared_ptr<Order> o) {
    // if (o->orderSize_ == 0) {
    //  LOG_ERROR(logger, "Incorrect OrderSize.");
    //  return;
    // }

    auto iter = orders_.find(o->serverOrderID_);
    if (iter != orders_.end())  // order exists
        return;

    orders_[o->serverOrderID_] = o;  // add to map
    cancels_[o->serverOrderID_] = false;
    fills_[o->serverOrderID_] = 0;
    LOG_INFO(logger, "Order is put under track. ServerOrderId=" << o->serverOrderID_);
}
```
- **第56行**: `trackOrder()` 函数定义
- **第57-60行**: 注释掉的订单数量检查代码
- **第62行**: 查找订单是否已存在，使用 `serverOrderID_` 作为键
- **第63-64行**: 如果订单已存在，直接返回，避免重复添加
- **第66行**: 将订单添加到映射表中
- **第67行**: 初始化取消状态为 `false`
- **第68行**: 初始化成交数量为 0
- **第69行**: 记录日志，订单已加入跟踪

**设计说明**: 使用 `serverOrderID_` 作为唯一标识符，确保每个订单只被跟踪一次。

#### 3.3.8 gotOrder() 函数实现
```cpp
void OrderManager::gotOrder(int64_t oid) {
    if (!isTracked(oid)) {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << oid);
        return;
    }

    lock_guard<mutex> g(orderStatus_mtx);
    if ((orders_[oid]->orderStatus_ == OrderStatus::OS_NewBorn)
        || (orders_[oid]->orderStatus_ == OrderStatus::OS_Submitted)) {
        orders_[oid]->orderStatus_ = OrderStatus::OS_Acknowledged;
    }
}
```
- **第72行**: `gotOrder()` 函数定义，参数为订单ID
- **第73-76行**: 检查订单是否在跟踪列表中，如果不在则记录错误并返回
- **第78行**: 获取订单状态互斥锁（`orderStatus_mtx` 在 `datastruct.h` 中定义）
- **第79-80行**: 检查订单状态是否为 `OS_NewBorn`（新创建）或 `OS_Submitted`（已提交）
- **第81行**: 如果满足条件，将状态更新为 `OS_Acknowledged`（已确认）

**状态转换说明**: 订单从提交状态转换为确认状态，表示券商已收到订单。

#### 3.3.9 gotFill() 函数实现
```cpp
void OrderManager::gotFill(const Fill& fill) {
    if (!isTracked(fill.serverOrderID_)) {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << fill.serverOrderID_);
    } else {
        LOG_INFO(logger,"Order is filled. ServerOrderId="<<fill.serverOrderID_<<"price = "<<fill.tradePrice_);
        lock_guard<mutex> g(orderStatus_mtx);
        orders_[fill.serverOrderID_]->orderStatus_ = OrderStatus::OS_Filled;
        orders_[fill.serverOrderID_]->updateTime_ = ymdhmsf();
        // TODO: check for partial fill
        PortfolioManager::instance().Adjust(fill);
    }
}
```
- **第85行**: `gotFill()` 函数定义，参数为成交回报的常量引用
- **第86-88行**: 检查订单是否在跟踪列表中，如果不在则记录错误
- **第89行**: 如果订单在跟踪列表中，记录成交信息日志
- **第90行**: 获取订单状态互斥锁
- **第91行**: 将订单状态更新为 `OS_Filled`（已成交）
- **第92行**: 更新订单的更新时间，使用 `ymdhmsf()` 函数获取当前时间戳
- **第93行**: TODO 注释，需要检查部分成交的情况
- **第94行**: 调用 `PortfolioManager` 单例的 `Adjust()` 方法，根据成交调整持仓

**业务流程**: 成交回报会触发订单状态更新和持仓调整。

#### 3.3.10 gotCancel() 函数实现
```cpp
void OrderManager::gotCancel(int64_t oid) {
    if (isTracked(oid)) {
        lock_guard<mutex> g(orderStatus_mtx);
        orders_[oid]->orderStatus_ = OrderStatus::OS_Canceled;
        orders_[oid]->updateTime_ = ymdhmsf();
        cancels_[oid] = true;
    }
}
```
- **第98行**: `gotCancel()` 函数定义，参数为订单ID
- **第99行**: 检查订单是否在跟踪列表中
- **第100行**: 如果订单在跟踪列表中，获取订单状态互斥锁
- **第101行**: 将订单状态更新为 `OS_Canceled`（已取消）
- **第102行**: 更新订单的更新时间
- **第103行**: 将取消标志设置为 `true`

#### 3.3.11 retrieveOrderFromServerOrderId() 函数实现
```cpp
std::shared_ptr<Order> OrderManager::retrieveOrderFromServerOrderId(int64_t oid) {
    if (orders_.count(oid)) {
        return orders_[oid];
    }
    return nullptr;
}
```
- **第107行**: 函数定义，根据服务器订单ID查询订单
- **第108行**: 使用 `count()` 检查订单是否存在（返回0或1）
- **第109行**: 如果存在，返回订单的智能指针
- **第111行**: 如果不存在，返回 `nullptr`

#### 3.3.12 retrieveOrderFromSourceAndClientOrderId() 函数实现
```cpp
std::shared_ptr<Order> OrderManager::retrieveOrderFromSourceAndClientOrderId(int32_t source, int64_t oid) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->clientOrderID_ == oid) && (iterator->second->clientID_ == source)) {
            return iterator->second;
        }
    }

    return nullptr;
}
```
- **第113行**: 函数定义，根据客户端ID和客户端订单ID查询订单
- **第114行**: 遍历订单映射表
- **第115行**: 检查客户端订单ID和客户端ID是否匹配
- **第116行**: 如果匹配，返回订单智能指针
- **第119行**: 如果未找到，返回 `nullptr`

**查询逻辑**: 需要同时匹配客户端ID和客户端订单ID，因为不同客户端可能有相同的订单ID。

#### 3.3.13 retrieveOrderFromOrderNo() 函数实现
```cpp
std::shared_ptr<Order> OrderManager::retrieveOrderFromServerOrderId(int64_t oid) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->orderNo_ == ono)) {
            return iterator->second;
        }
    }

    return nullptr;
}
```
- **第123行**: 函数定义，根据订单号查询订单（注意函数名与实现不一致，可能是复制粘贴错误）
- **第124行**: 遍历订单映射表
- **第125行**: 检查订单号是否匹配
- **第126行**: 如果匹配，返回订单智能指针
- **第129行**: 如果未找到，返回 `nullptr`

#### 3.3.14 retrieveOrderFromAccAndBrokerOrderId() 函数实现
```cpp
std::shared_ptr<Order> OrderManager::retrieveOrderFromAccAndBrokerOrderId(const string& acc, int32_t oid) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->brokerOrderID_ == oid) && (iterator->second->account_ == acc)) {
            return iterator->second;
        }
    }
    return nullptr;
}
```
- **第133行**: 函数定义，根据账户和券商订单ID查询订单
- **第134行**: 遍历订单映射表
- **第135行**: 检查券商订单ID和账户是否匹配
- **第136行**: 如果匹配，返回订单智能指针
- **第139行**: 如果未找到，返回 `nullptr`

#### 3.3.15 retrieveOrderFromAccAndLocalNo() 函数实现
```cpp
std::shared_ptr<Order> OrderManager::retrieveOrderFromAccAndLocalNo(const string& acc, const string& ono) {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((iterator->second->localNo_ == ono) && (iterator->second->account_ == ono)) {
            return iterator->second;
        }
    }
    return nullptr;
}
```
- **第145行**: 函数定义，根据账户和本地订单号查询订单
- **第146行**: 遍历订单映射表
- **第147行**: 检查本地订单号和账户是否匹配（注意：代码中使用了 `ono` 两次，可能是bug，应该是 `acc`）
- **第148行**: 如果匹配，返回订单智能指针
- **第151行**: 如果未找到，返回 `nullptr`

**潜在Bug**: 第147行应该检查 `iterator->second->account_ == acc` 而不是 `== ono`。

#### 3.3.16 retrieveOrder() 函数实现
```cpp
vector<std::shared_ptr<Order>> OrderManager::retrieveOrder(const string& fullsymbol) {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (iterator->second->fullSymbol_ == fullsymbol) {
            v.push_back(iterator->second);
        }
    }

    return v;
}
```
- **第166行**: 函数定义，根据合约代码查询所有相关订单，返回向量
- **第167行**: 创建订单指针向量
- **第168行**: 遍历订单映射表
- **第169行**: 检查合约代码是否匹配
- **第170行**: 如果匹配，将订单添加到向量中
- **第173行**: 返回包含所有匹配订单的向量

#### 3.3.17 retrieveNonFilledOrderPtr() 函数实现
```cpp
vector<std::shared_ptr<Order>> OrderManager::retrieveNonFilledOrderPtr() {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            v.push_back(iterator->second);
        }
    }

    return v;
}
```
- **第177行**: 函数定义，查询所有未完全成交的订单，返回订单指针向量
- **第178行**: 创建订单指针向量
- **第179行**: 遍历订单映射表
- **第180行**: 使用 `isActiveOS()` 函数检查订单状态是否为活跃状态（未成交、未取消、未错误）
- **第181行**: 如果是活跃状态，将订单添加到向量中
- **第184行**: 返回包含所有活跃订单的向量

**说明**: `isActiveOS()` 函数在 `datastruct.cpp` 中定义，返回 `true` 表示订单仍在处理中。

#### 3.3.18 retrieveNonFilledOrderPtr() 重载版本实现
```cpp
vector<std::shared_ptr<Order>> OrderManager::retrieveNonFilledOrderPtr(const string& fullsymbol) {
    vector<std::shared_ptr<Order>> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ( isActiveOS(iterator->second->orderStatus_)  && (iterator->second->fullSymbol_ == fullsymbol)) {
            v.push_back(iterator->second);
        }
    }

    return v;
}
```
- **第188行**: 函数定义，查询指定合约的未完全成交订单，返回订单指针向量
- **第189行**: 创建订单指针向量
- **第190行**: 遍历订单映射表
- **第191行**: 同时检查订单状态是否为活跃状态且合约代码匹配
- **第192行**: 如果满足条件，将订单添加到向量中
- **第195行**: 返回包含匹配订单的向量

#### 3.3.19 retrieveNonFilledOrderId() 函数实现
```cpp
vector<int64_t> OrderManager::retrieveNonFilledOrderId() {
    vector<int64_t> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            v.push_back(iterator->first);
        }
    }

    return v;
}
```
- **第199行**: 函数定义，查询所有未完全成交的订单ID，返回ID向量
- **第200行**: 创建订单ID向量
- **第201行**: 遍历订单映射表
- **第202行**: 检查订单状态是否为活跃状态
- **第203行**: 如果是活跃状态，将订单ID（`iterator->first`）添加到向量中
- **第206行**: 返回包含所有活跃订单ID的向量

#### 3.3.20 retrieveNonFilledOrderId() 重载版本实现
```cpp
vector<int64_t> OrderManager::retrieveNonFilledOrderId(const string& fullsymbol) {
    vector<int64_t> v;
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if ((isActiveOS(iterator->second->orderStatus_)) && (iterator->second->fullSymbol_ == fullsymbol)) {
            v.push_back(iterator->first);
        }
    }

    return v;
}
```
- **第210行**: 函数定义，查询指定合约的未完全成交订单ID，返回ID向量
- **第211行**: 创建订单ID向量
- **第212行**: 遍历订单映射表
- **第213行**: 同时检查订单状态是否为活跃状态且合约代码匹配
- **第214行**: 如果满足条件，将订单ID添加到向量中
- **第217行**: 返回包含匹配订单ID的向量

#### 3.3.21 isEmpty() 函数实现
```cpp
bool OrderManager::isEmpty() {
    return orders_.empty();
}
```
- **第221行**: `isEmpty()` 函数定义
- **第222行**: 返回订单映射表是否为空

#### 3.3.22 isTracked() 函数实现
```cpp
bool OrderManager::isTracked(int64_t oid) {
    auto it = orders_.find(oid);
    return (it != orders_.end());
}
```
- **第225行**: `isTracked()` 函数定义，检查订单是否在跟踪列表中
- **第226行**: 使用 `find()` 查找订单
- **第227行**: 如果迭代器不等于 `end()`，说明找到了订单，返回 `true`；否则返回 `false`

#### 3.3.23 isCompleted() 函数实现
```cpp
bool OrderManager::isCompleted(int64_t oid) {
    if (isTracked(oid)) {
        return isActiveOS(orders_[oid]->orderStatus_);
    } else {
        LOG_ERROR(logger, "Order is not tracked. ServerOrderId= " << oid);
        return true;
    }
}
```
- **第231行**: `isCompleted()` 函数定义，检查订单是否已完成
- **第232行**: 检查订单是否在跟踪列表中
- **第233行**: 如果订单在跟踪列表中，使用 `isActiveOS()` 检查状态
  - `isActiveOS()` 返回 `false` 表示订单已完成（已成交或已取消）
  - 所以这里返回 `!isActiveOS()` 的结果，但代码逻辑有误
- **第234行**: 如果订单不在跟踪列表中
- **第235行**: 记录错误日志
- **第236行**: 返回 `true`（认为已完成，因为不在跟踪列表中）

**潜在Bug**: 第233行的逻辑有误。`isCompleted()` 应该返回 `!isActiveOS()`，但当前代码返回的是 `isActiveOS()`，逻辑相反。

#### 3.3.24 hasPendingOrders() 函数实现
```cpp
bool OrderManager::hasPendingOrders() {
    for (auto iterator = orders_.begin(); iterator != orders_.end(); ++iterator) {
        if (isActiveOS(iterator->second->orderStatus_)) {
            return true;
        }
    }
    return false;
}
```
- **第240行**: `hasPendingOrders()` 函数定义，检查是否有待处理的订单
- **第241行**: 遍历订单映射表
- **第242行**: 检查订单状态是否为活跃状态
- **第243行**: 如果找到活跃订单，立即返回 `true`
- **第246行**: 如果遍历完所有订单都没有找到活跃订单，返回 `false`

#### 3.3.25 命名空间结束
```cpp
}  // namespace StarQuant
```
- **第249行**: 关闭 `StarQuant` 命名空间

---

## 4. portfoliomanager.h / portfoliomanager.cpp

### 4.1 文件概述
`portfoliomanager.h` 和 `portfoliomanager.cpp` 实现了 `PortfolioManager` 类，采用单例模式，负责管理投资组合，包括持仓管理和账户信息管理。

### 4.2 portfoliomanager.h 详细分析

#### 4.2.1 版权声明和头文件保护
```cpp
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

#ifndef CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 头文件保护宏定义开始
- **第17行**: 定义保护宏 `CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_`

#### 4.2.2 包含的头文件
```cpp
#include <Common/datastruct.h>
#include <assert.h>
#include <string>
#include <numeric>
#include <mutex>
#include <regex>
#include <atomic>
#include <map>
#include <memory>

using namespace std;
```
- **第20行**: 包含数据结构头文件，使用 `Position`、`Fill`、`AccountInfo` 等类型
- **第21行**: 包含断言头文件 `<assert.h>`，用于调试
- **第22行**: 包含字符串类
- **第23行**: 包含数值算法头文件 `<numeric>`，可能用于累计计算
- **第24行**: 包含互斥锁，用于线程安全
- **第25行**: 包含正则表达式库，可能用于符号匹配
- **第26行**: 包含原子类型（虽然当前未使用）
- **第27行**: 包含映射容器
- **第28行**: 包含智能指针头文件 `<memory>`
- **第30行**: 使用标准命名空间

#### 4.2.3 PortfolioManager 类声明

##### 4.2.3.1 构造函数和析构函数
```cpp
namespace StarQuant {
class PortfolioManager {
 public:
    PortfolioManager();
    ~PortfolioManager();
```
- **第32行**: 打开 `StarQuant` 命名空间
- **第33行**: `PortfolioManager` 类声明开始
- **第34行**: `public` 访问控制符
- **第35行**: 默认构造函数声明
- **第36行**: 析构函数声明

##### 4.2.3.2 单例模式相关成员
```cpp
    static PortfolioManager* pinstance_;
    static mutex instancelock_;
    static PortfolioManager& instance();
```
- **第37行**: 静态成员变量 `pinstance_`，指向单例实例的指针
- **第38行**: 静态互斥锁 `instancelock_`，用于保护单例创建的线程安全
- **第39行**: 静态成员函数 `instance()`，获取单例实例的引用

##### 4.2.3.3 成员变量
```cpp
    // atomic<uint64_t> _count = { 0 };
    uint64_t _count = 0;
    AccountInfo account_;
    map<string, AccountInfo> accinfomap_;     // accname->acc
    map<string, std::shared_ptr<Position> > positions_;  // poskey ->pos
    double cash_;
```
- **第40行**: 注释掉的原子计数变量
- **第41行**: `_count` 计数器，记录持仓数量（非线程安全版本）
- **第42行**: `account_` 账户信息对象，存储当前账户信息
- **第43行**: `accinfomap_` 映射表，键为账户名称（`string`），值为账户信息（`AccountInfo`），用于管理多个账户
- **第44行**: `positions_` 映射表，键为持仓键（`string`，通常是账户+合约的组合），值为持仓智能指针
- **第45行**: `cash_` 现金余额，`double` 类型

**数据结构说明**:
- 使用 `std::shared_ptr<Position>` 管理持仓生命周期
- `accinfomap_` 支持多账户管理
- `positions_` 使用持仓键作为唯一标识

##### 4.2.3.4 成员函数
```cpp
    void reset();
    void rebuild();

    void Add(std::shared_ptr<Position> ppos);
    double Adjust(const Fill& fill);
    std::shared_ptr<Position> retrievePosition(const string& key);
```
- **第47行**: `reset()` 重置所有数据
- **第48行**: `rebuild()` 重建投资组合
- **第50行**: `Add()` 添加持仓
- **第51行**: `Adjust()` 根据成交调整持仓，返回调整后的盈亏
- **第52行**: `retrievePosition()` 根据持仓键查询持仓

#### 4.2.4 类结束
```cpp
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_PORTFOLIOMANAGER_H_
```
- **第53行**: 类声明结束
- **第55行**: 关闭 `StarQuant` 命名空间
- **第57行**: 头文件保护宏结束

### 4.3 portfoliomanager.cpp 详细分析

#### 4.3.1 版权声明和头文件包含
```cpp
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

#include <Trade/portfoliomanager.h>
#include <Common/datastruct.h>
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 包含自己的头文件
- **第17行**: 包含数据结构头文件

#### 4.3.2 静态成员初始化
```cpp
namespace StarQuant {
PortfolioManager* PortfolioManager::pinstance_ = nullptr;
mutex PortfolioManager::instancelock_;
```
- **第19行**: 打开 `StarQuant` 命名空间
- **第20行**: 初始化静态成员 `pinstance_` 为 `nullptr`
- **第21行**: 初始化静态互斥锁 `instancelock_`

#### 4.3.3 析构函数实现
```cpp
PortfolioManager::~PortfolioManager()
{
    // release all the positions
    /*for (auto&& p : positions_) {
        if (p.second != nullptr) delete p.second;
    }*/
}
```
- **第23行**: 析构函数定义
- **第24行**: 函数体开始
- **第25行**: 注释说明释放所有持仓
- **第26-28行**: 注释掉的释放代码，因为使用智能指针管理，不需要手动释放

#### 4.3.4 instance() 单例函数实现
```cpp
PortfolioManager& PortfolioManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new PortfolioManager();
        }
    }
    return *pinstance_;
}
```
- **第31行**: `instance()` 函数定义，返回单例引用
- **第32行**: 第一次检查，如果实例不存在
- **第33行**: 获取互斥锁
- **第34行**: 第二次检查（双重检查锁定模式）
- **第35行**: 创建新的 `PortfolioManager` 实例
- **第38行**: 返回实例的引用

#### 4.3.5 构造函数实现
```cpp
PortfolioManager::PortfolioManager() :_count(0) {
    rebuild();
}
```
- **第41行**: 构造函数定义，使用初始化列表将 `_count` 初始化为 0
- **第42行**: 调用 `rebuild()` 初始化投资组合

#### 4.3.6 reset() 函数实现
```cpp
void PortfolioManager::reset() {
    /*for (auto&& p : positions_) {
        if (p.second != nullptr) delete p.second;
    }*/

    positions_.clear();
    _count = 0;
}
```
- **第45行**: `reset()` 函数定义
- **第46-48行**: 注释掉的释放代码
- **第50行**: 清空持仓映射表
- **第51行**: 重置计数器为 0

**说明**: 没有清空 `accinfomap_` 和重置 `cash_`，可能是设计上的不完整。

#### 4.3.7 rebuild() 函数实现
```cpp
void PortfolioManager::rebuild() {
    reset();
}
```
- **第54行**: `rebuild()` 函数定义
- **第55行**: 调用 `reset()` 重置所有数据

**说明**: 当前实现只是简单地重置，未来可能会从数据库或配置文件重建持仓。

#### 4.3.8 Add() 函数实现
```cpp
void PortfolioManager::Add(std::shared_ptr<Position> pos) {
    if (pos)
        positions_[pos->key_] = pos;
}
```
- **第58行**: `Add()` 函数定义，添加持仓
- **第59行**: 检查持仓指针是否有效
- **第60行**: 如果有效，使用持仓的 `key_` 作为键，将持仓添加到映射表中

**设计说明**: 使用 `key_` 作为唯一标识符，如果已存在相同键的持仓，会被覆盖。

#### 4.3.9 Adjust() 函数实现
```cpp
double PortfolioManager::Adjust(const Fill& fill) {
    // auto it = positions_.find(fill.fullSymbol_);
    // if (it == positions_.end()) {
    // 	Position pos;
    // 	pos.fullSymbol_ = fill.fullSymbol_;
    // 	pos.size_ = 0;
    // 	pos.avgPrice_ = 0;
    // 	positions_.insert(std::pair<string, Position>(fill.fullSymbol_, pos));
    // }

    // return positions_[fill.fullSymbol_].Adjust(fill);TODO: add adjust
    return 1.0;
}
```
- **第63行**: `Adjust()` 函数定义，根据成交调整持仓
- **第64-71行**: 注释掉的实现代码，原本应该：
  - 查找持仓（使用 `fullSymbol_` 作为键）
  - 如果不存在，创建新的持仓
  - 调用持仓的 `Adjust()` 方法
- **第73行**: TODO 注释，需要实现调整逻辑
- **第74行**: 当前返回固定值 1.0，这是未完成的实现

**说明**: 这是一个未完成的函数，需要实现根据成交调整持仓的逻辑。

#### 4.3.10 retrievePosition() 函数实现
```cpp
std::shared_ptr<Position> PortfolioManager::retrievePosition(const string& key) {
    auto it = positions_.find(key);
    if (it != positions_.end()) {
        return it->second;
    }
    return nullptr;
}
```
- **第78行**: `retrievePosition()` 函数定义，根据持仓键查询持仓
- **第79行**: 使用 `find()` 查找持仓
- **第80行**: 如果找到（迭代器不等于 `end()`）
- **第81行**: 返回持仓的智能指针
- **第83行**: 如果未找到，返回 `nullptr`

#### 4.3.11 命名空间结束
```cpp
}  // namespace StarQuant
```
- **第87行**: 关闭 `StarQuant` 命名空间

---

## 5. riskmanager.h / riskmanager.cpp

### 5.1 文件概述
`riskmanager.h` 和 `riskmanager.cpp` 实现了 `RiskManager` 类，采用单例模式，负责风险管理，包括订单数量限制、资金限制、流速限制等风险控制。

### 5.2 riskmanager.h 详细分析

#### 5.2.1 版权声明和头文件保护
```cpp
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

#ifndef CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_
#define CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_
```
- **第1-14行**: Apache License 2.0 版权声明
- **第16行**: 头文件保护宏定义开始
- **第17行**: 定义保护宏 `CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_`

#### 5.2.2 包含的头文件
```cpp
#include <Common/datastruct.h>
#include <string>
#include <sstream>
#include <map>
#include <regex>
#include <mutex>
#include <atomic>
#include <memory>

using namespace std;
```
- **第19行**: 包含数据结构头文件，使用 `Order` 等类型
- **第20行**: 包含字符串类
- **第21行**: 包含字符串流类
- **第22行**: 包含映射容器
- **第23行**: 包含正则表达式库
- **第24行**: 包含互斥锁，用于线程安全
- **第25行**: 包含原子类型（虽然当前未使用）
- **第26行**: 包含智能指针头文件
- **第28行**: 使用标准命名空间

#### 5.2.3 RiskManager 类声明

##### 5.2.3.1 构造函数和析构函数
```cpp
namespace StarQuant {
class RiskManager {
 public:
    RiskManager();
    ~RiskManager();
```
- **第30行**: 打开 `StarQuant` 命名空间
- **第31行**: `RiskManager` 类声明开始
- **第32行**: `public` 访问控制符
- **第33行**: 默认构造函数声明
- **第34行**: 析构函数声明

##### 5.2.3.2 单例模式相关成员
```cpp
    static RiskManager* pinstance_;
    static mutex instancelock_;
    static RiskManager& instance();
```
- **第35行**: 静态成员变量 `pinstance_`，指向单例实例的指针
- **第36行**: 静态互斥锁 `instancelock_`，用于保护单例创建的线程安全
- **第37行**: 静态成员函数 `instance()`，获取单例实例的引用

##### 5.2.3.3 成员变量 - 状态标志
```cpp
    bool alive_;
```
- **第39行**: `alive_` 布尔标志，表示风险管理是否启用

##### 5.2.3.4 成员变量 - 单笔订单限制
```cpp
    // per order limit
    int32_t limitSizePerOrder_ = 100;
    double limitCashPerOrder_ = 100000;
```
- **第41行**: 注释说明这是单笔订单的限制
- **第42行**: `limitSizePerOrder_` 单笔订单数量限制，默认100
- **第43行**: `limitCashPerOrder_` 单笔订单资金限制，默认100000

##### 5.2.3.5 成员变量 - 每日总限制
```cpp
    // total limit everyday
    int32_t limitOrderCount_ = 100;
    int32_t limitCash_ = 100000;
    int32_t limitOrderSize_ = 100;
```
- **第45行**: 注释说明这是每日总限制
- **第46行**: `limitOrderCount_` 每日订单数量限制，默认100
- **第47行**: `limitCash_` 每日资金限制，默认100000
- **第48行**: `limitOrderSize_` 每日订单总数量限制，默认100

##### 5.2.3.6 成员变量 - 累计统计
```cpp
    int32_t totalOrderCount_ = 0;
    double totalCash_ = 0;
    int32_t totalOrderSize_ = 0;
```
- **第50行**: `totalOrderCount_` 累计订单数量
- **第51行**: `totalCash_` 累计资金使用量
- **第52行**: `totalOrderSize_` 累计订单总数量

##### 5.2.3.7 成员变量 - 流速限制
```cpp
    // flow limit
    int32_t limitOrderCountPerSec_ = 10;

    int32_t orderCountPerSec_ = 0;
```
- **第54行**: 注释说明这是流速限制
- **第55行**: `limitOrderCountPerSec_` 每秒订单数量限制，默认10
- **第57行**: `orderCountPerSec_` 当前秒内的订单数量

##### 5.2.3.8 成员函数
```cpp
    // check order
    bool passOrder(std::shared_ptr<Order>);

    // reset per day, sec ...
    void reset();
    void switchday();

    void resetflow();
```
- **第59行**: 注释说明这是订单检查函数
- **第60行**: `passOrder()` 检查订单是否通过风险控制，返回 `true` 表示通过
- **第62行**: 注释说明重置函数用于每日、每秒等周期重置
- **第63行**: `reset()` 重置风险参数（从配置加载）
- **第64行**: `switchday()` 切换交易日，重置每日累计统计
- **第66行**: `resetflow()` 重置流速统计（每秒重置）

#### 5.2.4 类结束
```cpp
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_TRADE_RISKMANAGER_H_
```
- **第67行**: 类声明结束
- **第69行**: 关闭 `StarQuant` 命名空间
- **第71行**: 头文件保护宏结束

### 5.3 riskmanager.cpp 详细分析

#### 5.3.1 版权声明和头文件包含
```cpp
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


#include <Trade/riskmanager.h>
#include <Common/config.h>
#include <fmt/format.h>
#include <memory>
```
- **第1-14行**: Apache License 2.0 版权声明
- **第17行**: 包含自己的头文件
- **第18行**: 包含配置头文件，使用 `CConfig` 读取风险参数
- **第19行**: 包含格式化库 `<fmt/format.h>`，用于格式化输出
- **第20行**: 包含智能指针头文件

#### 5.3.2 静态成员初始化
```cpp
namespace StarQuant {
RiskManager* RiskManager::pinstance_ = nullptr;
mutex RiskManager::instancelock_;
```
- **第22行**: 打开 `StarQuant` 命名空间
- **第23行**: 初始化静态成员 `pinstance_` 为 `nullptr`
- **第24行**: 初始化静态互斥锁 `instancelock_`

#### 5.3.3 构造函数实现
```cpp
RiskManager::RiskManager() : alive_(false) {
    // set up from config
    reset();
}
```
- **第26行**: 构造函数定义，使用初始化列表将 `alive_` 初始化为 `false`
- **第27行**: 注释说明从配置设置参数
- **第28行**: 调用 `reset()` 从配置加载风险参数

#### 5.3.4 析构函数实现
```cpp
RiskManager::~RiskManager() {
}
```
- **第31行**: 析构函数定义
- **第32行**: 空实现

#### 5.3.5 instance() 单例函数实现
```cpp
RiskManager& RiskManager::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new RiskManager();
        }
    }
    return *pinstance_;
}
```
- **第34行**: `instance()` 函数定义，返回单例引用
- **第35行**: 第一次检查，如果实例不存在
- **第36行**: 获取互斥锁
- **第37行**: 第二次检查（双重检查锁定模式）
- **第38行**: 创建新的 `RiskManager` 实例
- **第41行**: 返回实例的引用

#### 5.3.6 passOrder() 函数实现
```cpp
bool RiskManager::passOrder(std::shared_ptr<Order> o) {
    if (!alive_)
        return true;
    totalOrderCount_ += 1;
    totalOrderSize_ += abs(o->quantity_);
    orderCountPerSec_ += 1;
    bool ocok = (totalOrderCount_ <= limitOrderCount_);
    bool osok = (totalOrderSize_ <= limitOrderSize_);
    bool ospok = (abs(o->quantity_) <= limitSizePerOrder_);
    bool ocpsok = (orderCountPerSec_ <= limitOrderCountPerSec_);
    if (ocok && osok && ospok && ocpsok)
        return true;
    fmt:printf("totalcount:{},totalsize{},sizeperorder:{},countpersecond{}",
        ocok, osok, ospok, ocpsok);
    return false;
}
```
- **第46行**: `passOrder()` 函数定义，检查订单是否通过风险控制
- **第47-48行**: 如果风险管理未启用（`alive_` 为 `false`），直接返回 `true`（通过）
- **第49行**: 累计订单数量加1
- **第50行**: 累计订单总数量，加上订单数量的绝对值
- **第51行**: 当前秒内的订单数量加1
- **第52行**: 检查累计订单数量是否在限制内
- **第53行**: 检查累计订单总数量是否在限制内
- **第54行**: 检查单笔订单数量是否在限制内
- **第55行**: 检查每秒订单数量是否在限制内
- **第56行**: 如果所有检查都通过，返回 `true`
- **第57-58行**: 如果有检查未通过，使用 `fmt::printf` 打印检查结果（注意：代码中使用了 `fmt:` 而不是 `fmt::`，可能是笔误）
- **第59行**: 返回 `false`（未通过）

**风险检查逻辑**:
1. 如果风险管理未启用，直接通过
2. 更新累计统计
3. 检查四个限制条件：
   - 累计订单数量限制
   - 累计订单总数量限制
   - 单笔订单数量限制
   - 每秒订单数量限制
4. 所有条件都满足才通过

**潜在Bug**: 第57行的 `fmt:printf` 应该是 `fmt::printf`，而且参数顺序可能有问题。

#### 5.3.7 reset() 函数实现
```cpp
void RiskManager::reset() {
    alive_ = CConfig::instance().riskcheck;
    limitSizePerOrder_ = CConfig::instance().sizeperorderlimit;
    limitCashPerOrder_ = CConfig::instance().cashperorderlimit;
    limitOrderCount_ = CConfig::instance().ordercountlimit;
    limitCash_ = CConfig::instance().cashlimit;
    limitOrderSize_ = CConfig::instance().ordersizelimit;
    limitOrderCountPerSec_ = CConfig::instance().ordercountperseclimit;
}
```
- **第64行**: `reset()` 函数定义，从配置加载风险参数
- **第65行**: 从配置读取风险检查开关，设置 `alive_`
- **第66行**: 从配置读取单笔订单数量限制
- **第67行**: 从配置读取单笔订单资金限制
- **第68行**: 从配置读取每日订单数量限制
- **第69行**: 从配置读取每日资金限制
- **第70行**: 从配置读取每日订单总数量限制
- **第71行**: 从配置读取每秒订单数量限制

**说明**: 所有风险参数都从配置系统读取，便于动态调整。

#### 5.3.8 resetflow() 函数实现
```cpp
void RiskManager::resetflow() {
    orderCountPerSec_ = 0;
}
```
- **第74行**: `resetflow()` 函数定义，重置流速统计
- **第75行**: 将每秒订单数量重置为 0

**使用场景**: 应该在每秒开始时调用，重置流速计数器。

#### 5.3.9 switchday() 函数实现
```cpp
void RiskManager::switchday() {
    totalOrderCount_ = 0;
    totalCash_ = 0.0;
    totalOrderSize_ = 0;
}
```
- **第78行**: `switchday()` 函数定义，切换交易日
- **第79行**: 重置累计订单数量为 0
- **第80行**: 重置累计资金使用量为 0.0
- **第81行**: 重置累计订单总数量为 0

**使用场景**: 应该在每个交易日开始时调用，重置每日累计统计。

#### 5.3.10 命名空间结束
```cpp
}   // namespace StarQuant
```
- **第84行**: 关闭 `StarQuant` 命名空间

---

## 总结

### 设计模式总结
1. **单例模式**: `OrderManager`、`PortfolioManager`、`RiskManager` 都采用单例模式，使用双重检查锁定确保线程安全
2. **智能指针**: 广泛使用 `std::shared_ptr` 管理对象生命周期，避免内存泄漏
3. **映射表存储**: 使用 `std::map` 存储订单、持仓等数据，便于快速查找

### 线程安全
- 使用 `mutex` 保护共享数据
- 单例创建使用双重检查锁定
- 订单状态更新使用 `orderStatus_mtx` 互斥锁

### 未完成的功能
1. `MarginManager` 类为空实现
2. `PortfolioManager::Adjust()` 函数未完成实现
3. `RiskManager::passOrder()` 中的格式化输出可能有bug

### 潜在问题
1. `OrderManager::isCompleted()` 逻辑可能有误
2. `OrderManager::retrieveOrderFromAccAndLocalNo()` 中使用了错误的变量名
3. `RiskManager::passOrder()` 中的 `fmt:printf` 应该是 `fmt::printf`

### 使用建议
1. 订单管理：使用 `OrderManager::instance()` 获取单例，跟踪订单生命周期
2. 持仓管理：使用 `PortfolioManager::instance()` 管理投资组合
3. 风险控制：使用 `RiskManager::instance().passOrder()` 在提交订单前进行风险检查
4. 盈亏计算：使用 `Calc` 命名空间下的函数计算盈亏

---

**文档生成时间**: 2026-02-06
**分析完成状态**: ✅ 已完成所有文件分析
