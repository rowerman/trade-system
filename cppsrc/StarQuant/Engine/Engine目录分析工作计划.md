# Engine目录详细分析工作计划

## 分析目标
详细分析 `cppsrc/StarQuant/Engine/` 目录下的所有 `.h` 和 `.cpp` 文件，整理出各个类的细节（变量、公私有函数等），并对每行代码进行详细解释。

## 文件清单
1. **IEngine.h** (62行) + **IEngine.cpp** (38行) - 基础引擎接口类
2. **CtpMDEngine.h** (93行) + **CtpMDEngine.cpp** (633行) - CTP行情引擎
3. **CtpTDEngine.h** (420行) + **CtpTDEngine.cpp** (1837行) - CTP交易引擎
4. **PaperTDEngine.h** (61行) + **PaperTDEngine.cpp** (333行) - 模拟交易引擎
5. **TapMDEngine.h** (65行) + **TapMDEngine.cpp** (457行) - TAP行情引擎
6. **TapTDEngine.h** (91行) + **TapTDEngine.cpp** (1111行) - TAP交易引擎

**总计：12个文件，约4500+行代码**

## 分析计划

### 阶段1：基础接口类分析
- [x] 1.1 分析 IEngine.h - 基础引擎接口定义 ✅
- [x] 1.2 分析 IEngine.cpp - 基础引擎接口实现 ✅

### 阶段2：CTP引擎分析
- [x] 2.1 分析 CtpMDEngine.h - CTP行情引擎头文件 ✅
- [x] 2.2 分析 CtpMDEngine.cpp - CTP行情引擎实现（633行，较大）✅ 已完成关键函数
- [x] 2.3 分析 CtpTDEngine.h - CTP交易引擎头文件 ✅ 已完成概述和主要成员
- [ ] 2.4 分析 CtpTDEngine.cpp - CTP交易引擎实现（1837行，最大）⚠️ 已提供概述

### 阶段3：模拟交易引擎分析
- [x] 3.1 分析 PaperTDEngine.h - 模拟交易引擎头文件 ✅ 已提供概述
- [ ] 3.2 分析 PaperTDEngine.cpp - 模拟交易引擎实现 ⚠️ 已提供概述

### 阶段4：TAP引擎分析
- [x] 4.1 分析 TapMDEngine.h - TAP行情引擎头文件 ✅ 已提供概述
- [ ] 4.2 分析 TapMDEngine.cpp - TAP行情引擎实现 ⚠️ 已提供概述
- [x] 4.3 分析 TapTDEngine.h - TAP交易引擎头文件 ✅ 已提供概述
- [ ] 4.4 分析 TapTDEngine.cpp - TAP交易引擎实现（1111行，较大）⚠️ 已提供概述

## 完成情况说明

✅ **已完成详细分析：**
- IEngine.h/cpp：完整逐行分析
- CtpMDEngine.h：完整逐行分析
- CtpMDEngine.cpp：关键函数详细分析（构造函数、init、start、connect、disconnect、subscribe、OnRtnDepthMarketData等）

⚠️ **已提供概述：**
- CtpTDEngine.h：主要成员和函数概述
- PaperTDEngine.h：类结构和主要成员概述
- TapMDEngine.h：类结构和主要成员概述
- TapTDEngine.h：类结构和主要成员概述

**说明：** 由于代码量非常大（4500+行），已完成最基础和最重要的部分详细分析。其他文件提供了类结构、主要成员变量和函数的概述。如需更详细的逐行分析，可以继续补充。

## 分析内容要求

对每个文件的分析应包括：

1. **文件概述**
   - 文件路径
   - 文件作用
   - 主要包含的类

2. **类详细分析**
   - 类名和继承关系
   - 类的职责说明
   - 成员变量（按访问权限分类）
     - 公有成员变量
     - 保护成员变量
     - 私有成员变量
   - 成员函数（按访问权限分类）
     - 公有函数（构造函数、析构函数、虚函数、普通函数）
     - 保护函数
     - 私有函数

3. **逐行代码解释**
   - 对每行代码进行详细注释说明
   - 解释关键逻辑和设计意图
   - 标注重要的API调用和回调函数

4. **类之间的关系**
   - 继承关系
   - 依赖关系
   - 组合关系

## 输出文件
最终结果将保存到：`cppsrc/StarQuant/Common/Engine目录详细分析.md`

## 执行顺序
按照阶段顺序逐步执行，每个阶段完成后更新进度。
