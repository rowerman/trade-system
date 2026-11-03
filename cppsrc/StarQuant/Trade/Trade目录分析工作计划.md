# Trade目录代码分析工作计划

## 目标
详细分析 `cppsrc/StarQuant/Trade/` 目录下的所有 `.h` 和 `.cpp` 文件，整理出各个类的细节（变量、公私有函数等），并对每行代码进行详细解释。

## 文件列表
1. **calc.h / calc.cpp** - 盈亏计算函数（命名空间 Calc）
2. **marginmanager.h / marginmanager.cpp** - 保证金管理类
3. **ordermanager.h / ordermanager.cpp** - 订单管理器类（单例模式）
4. **portfoliomanager.h / portfoliomanager.cpp** - 投资组合管理器类（单例模式）
5. **riskmanager.h / riskmanager.cpp** - 风险管理器类（单例模式）

## 分析计划

### 阶段1: calc.h / calc.cpp
- [ ] 分析 Calc 命名空间下的所有函数
- [ ] 详细解释每个函数的参数和返回值
- [ ] 逐行代码解释
- [ ] 分析函数实现逻辑

### 阶段2: marginmanager.h / marginmanager.cpp
- [ ] 分析 MarginManager 类的结构
- [ ] 详细解释每个成员变量
- [ ] 详细解释每个成员函数
- [ ] 逐行代码解释

### 阶段3: ordermanager.h / ordermanager.cpp
- [ ] 分析 OrderManager 类（单例模式）
- [ ] 详细解释每个成员变量
- [ ] 详细解释每个成员函数
- [ ] 逐行代码解释
- [ ] 分析订单跟踪和状态管理逻辑

### 阶段4: portfoliomanager.h / portfoliomanager.cpp
- [ ] 分析 PortfolioManager 类（单例模式）
- [ ] 详细解释每个成员变量
- [ ] 详细解释每个成员函数
- [ ] 逐行代码解释
- [ ] 分析持仓和账户管理逻辑

### 阶段5: riskmanager.h / riskmanager.cpp
- [ ] 分析 RiskManager 类（单例模式）
- [ ] 详细解释每个成员变量
- [ ] 详细解释每个成员函数
- [ ] 逐行代码解释
- [ ] 分析风险控制逻辑

### 阶段6: 生成最终文档
- [ ] 整合所有分析结果
- [ ] 生成完整的 Markdown 文档
- [ ] 添加目录索引
- [ ] 添加类关系图说明
- [ ] 添加使用示例

## 开始时间
2026-02-06

## 完成状态
✅ 所有阶段已完成

## 生成的文档
- `Trade目录详细分析.md` - 包含所有文件的详细分析，约5000+行
- 文档包含：
  - 每个文件的概述
  - 头文件包含说明
  - 所有类/函数的详细分析
  - 每个成员变量和函数的详细说明
  - 逐行代码解释
  - 使用示例说明
  - 设计模式总结
  - 潜在问题说明

## 文档结构
最终文档包含：
1. 目录索引
2. 每个文件的详细分析
   - 文件概述
   - 包含的头文件说明
   - 类/函数详细说明
   - 成员变量详细说明
   - 成员函数详细说明
   - 逐行代码解释
3. 设计模式总结
4. 使用建议
5. 潜在问题说明
