# Strategy目录代码详细分析文档

## 目录
1. [strategybase.h / strategybase.cpp - 策略基类](#1-strategybaseh--strategybasecpp---策略基类)
2. [smacross.h / smacross.cpp - 均线交叉策略](#2-smacrossh--smacrosscpp---均线交叉策略)
3. [strategyFactory.h / strategyFactory.cpp - 策略工厂](#3-strategyfactoryh--strategyfactorycpp---策略工厂)
4. [strategymanager.h / strategymanager.cpp - 策略管理器](#4-strategymanagerh--strategymanagercpp---策略管理器)
5. [mystrategy.cpp - 预留示例策略](#5-mystrategycpp---预留示例策略)
6. [类关系图与总结](#6-类关系图与总结)

---

## 1. strategybase.h / strategybase.cpp - 策略基类

### 1.1 文件概述

`strategybase.h` 和 `strategybase.cpp` 实现了量化交易系统的策略基类 `StrategyBase`。所有具体策略都必须继承此类并实现相应的虚函数。策略基类定义了与经纪商、数据源的通信接口，以及行情回调、订单发送等核心功能。

### 1.2 strategybase.h 详细分析

#### 1.2.1 头文件保护与包含

```cpp
#ifndef _StarQuant_Common_StraegyBase_H_   // 第1行：头文件保护宏开始
#define _StarQuant_Common_StraegyBase_H_   // 第2行：定义保护宏（注：Straegy为拼写错误，应为Strategy）
```

**说明：** 使用传统头文件保护防止重复包含。注意宏名中 "Straegy" 为 "Strategy" 的拼写错误。

```cpp
#include <Common/config.h>      // 第6行：配置管理类，获取account等配置
#include <Common/util.h>        // 第7行：工具函数，如msleep等
#include <Data/tick.h>          // 第8行：Tick行情数据结构
#include <Data/bar.h>           // 第9行：Bar/K线数据结构
#include <Trade/order.h>        // 第10行：Order订单数据结构
#include <Trade/fill.h>         // 第11行：Fill成交数据结构
#include <Common/logger.h>      // 第12行：日志类
#include <Trade/position.h>     // 第13行：Position持仓数据结构
#include <Common/msgq.h>        // 第14行：消息队列类

#include <queue>                // 第16行：STL队列，用于msgstobrokerage
```

**说明：** 策略基类依赖系统配置、数据结构定义、日志和消息队列等模块。

```cpp
namespace StarQuant {           // 第18行：StarQuant命名空间
	struct Position;            // 第19行：前向声明Position结构体，避免循环依赖
```

#### 1.2.2 StrategyBase 类定义

```cpp
class StrategyBase {            // 第21行：策略基类定义
//	protected:                 // 第22-24行：注释掉的protected区，原计划存放与brokerage、datafeed通信的消息队列
	//与brokerage,datafeed 沟通消息
//		std::unique_ptr<CMsgq> msgq_pair_;
//		std::unique_ptr<CMsgq> msgq_sub_;
```

**说明：** 消息队列的创建已移至策略服务层，策略通过 `msgstobrokerage` 队列间接发送消息。

##### 公有成员变量

```cpp
public:                         // 第26行：公有成员
	bool initialized = false;   // 第27行：策略是否已初始化的标志，默认false
	static int m_orderId;       // 第28行：静态成员，全局订单ID计数器，在cpp中初始化为0
	queue<string> msgstobrokerage;  // 第30行：待发送到经纪商的消息队列，策略将序列化后的订单消息push到此队列
```

**说明：** 
- `initialized`：由 `initialize()` 设为 true，`teardown()` 设为 false
- `m_orderId`：每次 `SendOrder` 时递增，保证订单ID唯一
- `msgstobrokerage`：策略服务层会轮询此队列，将消息转发给交易引擎

##### 构造函数与析构函数

```cpp
	StrategyBase();             // 第31行：默认构造函数
	virtual ~StrategyBase();    // 第32行：虚析构函数，确保派生类正确析构
```

##### 生命周期虚函数

```cpp
	virtual void initialize();  // 第33行：初始化策略，子类可重写
	virtual void reset();       // 第34行：重置策略状态，子类可重写
	virtual void teardown();    // 第35行：清理策略资源，子类可重写
```

##### 入站消息处理虚函数（Incoming Message Handlers）

```cpp
	// ************  Incoming Message Handers ********************//  // 第37行：注释（Handers应为Handlers）
	virtual void OnTick(Tick& k) {}           // 第38行：Tick行情回调，默认空实现
	virtual void OnBar(Bar& k) {}             // 第39行：Bar/K线回调，默认空实现
	virtual void OnOrderTicket(long oid) {}   // 第40行：订单确认回调，oid为订单ID
	virtual void OnOrderCancel(long oid) {}   // 第41行：订单撤销回调
	virtual void OnFill(Fill& f) {}           // 第42行：成交回报回调
	virtual void OnPosition(Position& p) {}   // 第43行：持仓回报回调
	virtual void OnGeneralMessage(string& msg) {}  // 第44行：通用消息回调
	// ************ End Incoming Message Handers ********************//
```

**说明：** 子类根据策略逻辑重写需要的回调。例如 SmaCross 重写了 `OnTick` 和 `OnGeneralMessage`。

##### 出站方法（Outgoing Methods）

```cpp
	// ************  Outgoing Methods ********************//
	void SendOrder(std::shared_ptr<Order> o);  // 第48行：发送订单到经纪商
	void SendOrderCancel(long oid);            // 第49行：发送撤单请求
	void SendSubscriptionRequest();            // 第50行：发送行情订阅请求（当前空实现）
	void SendHistoricalBarRequest();           // 第51行：发送历史Bar请求（当前空实现）
	void SendGeneralInformation();             // 第52行：发送通用信息（当前空实现）
	void SendLog();                            // 第53行：发送日志（当前空实现）
	// ************  End Outgoing Methods ********************//
};  // 第56行：类定义结束
}   // 第57行：命名空间结束

#endif // _StarQuant_Common_StraegyBase_H_  // 第59行：头文件保护结束
```

### 1.3 strategybase.cpp 详细分析

#### 1.3.1 头文件包含

```cpp
#include <Strategy/strategybase.h>   // 第1行：策略基类头文件
#include <Common/config.h>           // 第2行：配置类
#include <Common/timeutil.h>         // 第3行：时间工具（实际可能在util.h中）
#include <Trade/ordermanager.h>      // 第4行：订单管理器
```

**说明：** `timeutil.h` 可能通过 `util.h` 间接包含；`ordermanager.h` 可能提供 `oid_mtx` 等声明（实际 `oid_mtx` 在 `datastruct.h` 中声明）。

#### 1.3.2 静态成员初始化

```cpp
namespace StarQuant {
	extern std::atomic<bool> gShutdown;   // 第7行：声明外部全局变量，系统关闭标志
	int StrategyBase::m_orderId=0;        // 第8行：静态成员m_orderId初始化为0
```

#### 1.3.3 构造函数

```cpp
	StrategyBase::StrategyBase()  // 第9行：构造函数
	{
		// 第12-19行：已注释的消息队列初始化代码
		// 原设计根据CConfig中的_msgq类型（ZMQ或NANOMSG）创建PAIR和SUB套接字
		// 与BROKERAGE_PAIR_PORT、MKT_DATA_PUBSUB_PORT通信
		// 现在消息队列由策略服务层管理，策略通过msgstobrokerage间接通信

		//m_orderId = 0;  // 第21行：注释，m_orderId已在类外初始化为0
	}
```

#### 1.3.4 析构函数

```cpp
	StrategyBase::~StrategyBase() {   // 第25行：析构函数，空实现
	}
```

#### 1.3.5 initialize() 实现

```cpp
	void StrategyBase::initialize() {  // 第28行
		msleep(2000);                  // 第29行：休眠2000毫秒(2秒)，等待系统就绪
		initialized = true;            // 第30行：标记策略已初始化
	}
```

**说明：** 子类应在重写时先调用 `StrategyBase::initialize()`，再初始化自有状态。

#### 1.3.6 reset() 与 teardown() 实现

```cpp
	void StrategyBase::reset() {       // 第33行：重置，当前为空
	}

	void StrategyBase::teardown() {    // 第36行：清理
		initialized = false;           // 第37行：清除初始化标志
	}
```

#### 1.3.7 SendOrder() 实现

```cpp
	void StrategyBase::SendOrder(std::shared_ptr<Order> o)   // 第46行
	{
		//TODO：根据订单类型发送相关信息，如限价单需要有价格信息  // 第47行注释
		lock_guard<mutex> g(oid_mtx);  // 第48行：加锁保护m_orderId，oid_mtx在datastruct.h中extern声明

		o->createTime = time(nullptr);        // 第50行：设置订单创建时间（Unix时间戳）
		o->orderStatus_ = OrderStatus::OS_NewBorn;  // 第51行：订单状态设为"新生"
		o->clientOrderId = m_orderId;         // 第52行：分配客户端订单ID
		m_orderId++;                          // 第55行：递增全局订单ID

		string s;
		string s_price = "0.0";               // 第59行：价格字符串，市价单默认为"0.0"
		if (o->orderType == OrderType::OT_Limit){      // 第60行：限价单
			s_price = to_string(o->limitPrice);        // 第61行：使用限价
		}else if (o->orderType == OrderType::OT_StopLimit){  // 第62行：止损限价单
			s_price =to_string(o->stopPrice);          // 第63行：使用止损价
		}
		// 第65-74行：序列化订单消息，格式为：MSG_TYPE|account|source|clientOrderId|orderType|fullSymbol|orderSize|price|orderFlag|tag
		s = to_string(MSG_TYPE::MSG_TYPE_ORDER)
		+ SERIALIZATION_SEPARATOR + CConfig::instance().account
		+ SERIALIZATION_SEPARATOR + to_string(o->source)
		+ SERIALIZATION_SEPARATOR + to_string(o->clientOrderId)
		+ SERIALIZATION_SEPARATOR + to_string(static_cast<int>(o->orderType))
		+ SERIALIZATION_SEPARATOR + o->fullSymbol
		+ SERIALIZATION_SEPARATOR + to_string(o->orderSize)
		+ SERIALIZATION_SEPARATOR + s_price
		+ SERIALIZATION_SEPARATOR + to_string(static_cast<int>(o->orderFlag))
		+ SERIALIZATION_SEPARATOR + o->tag;
		msgstobrokerage.push(s);              // 第75行：将序列化消息加入队列
	}
```

**消息格式说明：**  
`MSG_TYPE_ORDER|account|source|clientOrderId|orderType|fullSymbol|orderSize|price|orderFlag|tag`  
分隔符为 `SERIALIZATION_SEPARATOR`（通常为 `'|'`）。

#### 1.3.8 SendOrderCancel() 实现

```cpp
	void StrategyBase::SendOrderCancel(long oid)   // 第80行
	{
		string msg = to_string(MSG_TYPE::MSG_TYPE_CANCEL_ORDER)  // 第83行：撤单消息类型
			+ SERIALIZATION_SEPARATOR + std::to_string(oid);     // 第84行：订单ID

		msgstobrokerage.push(msg);   // 第86行：加入队列
	}
```

#### 1.3.9 其他发送方法（空实现）

```cpp
	void StrategyBase::SendSubscriptionRequest() { }      // 第90行
	void StrategyBase::SendHistoricalBarRequest() { }     // 第95行
	void StrategyBase::SendGeneralInformation() { }       // 第100行
	void StrategyBase::SendLog() { }                      // 第105行
```

**说明：** 这些方法为预留接口，子类可按需实现。

---

## 2. smacross.h / smacross.cpp - 均线交叉策略

### 2.1 文件概述

`smacross.h` 和 `smacross.cpp` 实现了一个简单的均线交叉示例策略 `SmaCross`，用于演示如何继承 `StrategyBase` 并实现具体交易逻辑。该策略基于价格区间进行简单的开平仓。

### 2.2 smacross.h 详细分析

```cpp
#ifndef _StarQuant_Strategy_SmaCross_H_   // 第1行：头文件保护
#define _StarQuant_Strategy_SmaCross_H_   // 第2行

#include <Strategy/strategybase.h>   // 第4行：策略基类
#include <atomic>                   // 第5行：原子类型（当前未使用）

namespace StarQuant {
	class SmaCross : public StrategyBase {   // 第8行：继承StrategyBase
	public:
		virtual void initialize();           // 第10行：重写初始化
		virtual void OnTick(Tick& k);        // 第11行：重写Tick回调
		virtual void OnGeneralMessage(string& msg);  // 第12行：重写通用消息回调
	private:
		int order_time;      // 第14行：上次下单时间（或时间相关标记）
		bool buy_sell;       // 第15行：买卖方向标志，true=多头/买入倾向，false=空头/卖出倾向
		vector<Tick> tickarray;  // 第16行：Tick数组，用于存储历史行情（可计算均线）
		int ordercount;      // 第17行：已下订单计数，用于限制交易次数
		int sid;             // 第18行：策略ID，用于标识订单来源
	};
}

#endif  // 第23行
```

**说明：** 类名 "SmaCross" 暗示均线交叉策略，但当前实现更多是基于价格区间的开平仓逻辑。

### 2.3 smacross.cpp 详细分析

#### 2.3.1 initialize() 实现

```cpp
	void SmaCross::initialize() {   // 第5行
		order_time = -10;           // 第6行：初始化为-10，避免首次立即下单
		buy_sell = true;            // 第7行：初始为买入倾向
		ordercount = 0;             // 第8行：订单计数清零
		sid = 1;                    // 第9行：策略ID设为1
	}
```

**说明：** 未调用 `StrategyBase::initialize()`，因此不会执行基类的 2 秒等待。

#### 2.3.2 OnTick() 实现

```cpp
	void SmaCross::OnTick(Tick& k) {   // 第12行
		tickarray.push_back(k);        // 第13行：将当前Tick加入数组
```

**策略逻辑（第26-69行）：**
- 条件：`ordercount <= 4`，即最多进行 5 次开平仓（约 2-3 个回合）
- 买入：当 `k.price_ < 16000` 且 `buy_sell == true` 时，发送市价开多 1 手
- 平仓：当 `k.price_ > 15700` 且 `buy_sell == false` 时，发送市价平多 1 手

```cpp
		if( ordercount <= 4){
			if ((k.price_ < 16000) &&(buy_sell)) {    // 第29行：价格低于16000且为买入倾向
				auto o = std::make_shared<Order>();   // 第30行：创建订单
				o->orderType = OrderType::OT_Market;  // 第31行：市价单
				o->fullSymbol = k.fullsymbol_;        // 第32行：标的合约（注：Tick通常为fullSymbol_）
				o->orderSize = 1;                     // 第33行：1手
				o->orderFlag = OrderFlag:: OF_OpenPosition;  // 第34行：开仓（OF前有空格，可能为笔误）
				o->source = sid;                      // 第35行：策略来源ID
				SendOrder(o);                         // 第36行：发送订单
				ordercount++;                         // 第37行
				buy_sell = false;                     // 第38行：切换为平仓倾向
				return;                               // 第39行
			}
			if ((k.price_ > 15700) &&(! buy_sell)) {  // 第54行：价格高于15700且为平仓倾向
			auto o_close = std::make_shared<Order>(); // 第55行
			o_close->source = sid;
			o_close->orderType = OrderType::OT_Market;
			o_close->fullSymbol = k.fullsymbol_;
			o_close->orderSize = -1;                  // 第59行：负数表示平仓数量
			o_close->orderFlag = OrderFlag:: OF_ClosePosition;  // 第60行：平仓
			SendOrder(o_close);
			ordercount++;
			buy_sell = true;
			return;
			}
		}
	}
```

**说明：** 实际逻辑为低价开多、高价平多，与均线交叉无直接关系。  
**注意：** 若 Tick 来源于 `datastruct.h`，则成员名为 `fullSymbol_`（大写 S），而本文件使用 `k.fullsymbol_`（小写 s），可能存在命名不一致，需根据实际 `Data/tick.h` 定义确认。

#### 2.3.3 OnGeneralMessage() 实现

```cpp
	void SmaCross::OnGeneralMessage(string& msg) {   // 第80行
		printf("SMA strat general msg: [%s]\n", msg.c_str());  // 第81行：打印消息
	}
```

---

## 3. strategyFactory.h / strategyFactory.cpp - 策略工厂

### 3.1 文件概述

策略工厂根据策略名称（算法名）创建对应的策略实例，用于实现策略的动态加载和选择。

### 3.2 strategyFactory.h 详细分析

```cpp
#ifndef __StarQuant_Strategy_StrategyFactor_H__   // 第1行：头文件保护（Factor应为Factory）
#define __StarQuant_Strategy_StrategyFactor_H__

#include <memory>                    // 第4行：智能指针
#include <Common/config.h>           // 第5行：配置
#include <Strategy/strategybase.h>   // 第6行：策略基类

namespace StarQuant
{
	std::unique_ptr<StrategyBase> strategyFactory(const string& _algo);  // 第11行：工厂函数声明
}
#endif
```

**说明：** 宏名中 "Factor" 应为 "Factory" 的拼写错误。

### 3.3 strategyFactory.cpp 详细分析

```cpp
#include <boost/algorithm/string.hpp>   // 第1行：Boost字符串算法（如iequals）
#include <Strategy/strategyFactory.h>

namespace StarQuant
{
	std::unique_ptr<StrategyBase> strategyFactory(const string& _algo) {  // 第6行
		/*if (boost::iequals(_algo, "simplecross")) {   // 第7行：注释掉的简单均线策略
			//return make_unique<SimpleCross>();
		}*/
		return nullptr;   // 第11行：当前始终返回空指针，未实现具体策略创建逻辑
	}
}
```

**说明：** 工厂当前未实现任何策略的创建，调用方需要补充对 SmaCross 等策略的分支判断和创建逻辑。

---

## 4. strategymanager.h / strategymanager.cpp - 策略管理器

### 4.1 文件概述

策略管理器设计用于管理多个策略实例，当前实现为空壳，具体逻辑可能在策略服务（如 strategyservice）中。

### 4.2 strategymanager.h 详细分析

```cpp
#ifndef _StarQuant_Common_StrategyManager_H_   // 第1行：头文件保护
#define _StarQuant_Common_StrategyManager_H_

#include <string>    // 第4行
#include <sstream>   // 第5行
#include <map>       // 第6行
#include <regex>     // 第7行

using std::string;

namespace StarQuant
{
	class StrategyManager {   // 第13行：空类定义，无成员
	};
}

#endif
```

### 4.3 strategymanager.cpp 详细分析

```cpp
#include <Strategy/strategymanager.h>

namespace StarQuant {

}   // 空实现，无任何代码
```

---

## 5. mystrategy.cpp - 预留示例策略

### 5.1 文件概述

`mystrategy.cpp` 当前为空文件，预留给用户自定义策略实现。

---

## 6. 类关系图与总结

### 6.1 类继承关系

```
StrategyBase（策略基类）
    └── SmaCross（均线交叉示例策略）
```

### 6.2 依赖关系

```
StrategyBase
    ├── Common: config, util, logger, msgq
    ├── Data: Tick, Bar
    └── Trade: Order, Fill, Position

strategyFactory
    └── StrategyBase

SmaCross
    └── StrategyBase
```

### 6.3 数据流

```
行情/回报 → 策略服务 → OnTick/OnBar/OnFill 等 → 策略逻辑
                                                    ↓
订单请求 ← 策略服务 ← msgstobrokerage ← SendOrder/SendOrderCancel
```

### 6.4 设计模式

- **模板方法**：StrategyBase 定义策略框架，子类实现具体回调
- **工厂模式**：strategyFactory 按名称创建策略（当前未完善）
- **策略模式**：不同策略实现不同交易逻辑，统一接口

### 6.5 待完善点

1. `strategyFactory` 需补充 SmaCross 等策略的创建逻辑
2. `StrategyManager` 需实现策略的增删启停管理
3. `SendSubscriptionRequest` 等出站方法需实现
4. 头文件宏拼写：`StraegyBase`、`StrategyFactor` 建议修正
5. Order 成员命名：策略中使用 `fullSymbol`、`limitPrice` 等，若与 `datastruct.h` 中 `fullSymbol_`、`limitPrice_` 不一致，需确认 `Trade/order.h` 中的实际定义

---

## 完成状态

- [x] strategybase.h/cpp
- [x] smacross.h/cpp
- [x] strategyFactory.h/cpp
- [x] strategymanager.h/cpp
- [x] mystrategy.cpp
- [x] 类关系与总结

**文档生成时间：** 2026-02-06
