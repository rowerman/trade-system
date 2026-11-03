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

#ifndef CPPSRC_STARQUANT_DATA_DATAMANAGER_H_
#define CPPSRC_STARQUANT_DATA_DATAMANAGER_H_

#include <Common/datastruct.h>
#include <Data/tickwriter.h>
#include <string>
#include <sstream>
#include <map>
#include <regex>

using std::string;

namespace StarQuant
{
//////////////////////////////////////////////////////////////////////////
// DataManager - 数据管理单例
//
// 功能概述：
//   1. DataBoard 服务：维护最新全档 Tick 行情（orderBook_），供策略/服务查询
//   2. 数据记录：通过 TickWriter 将行情写入文件或 MongoDB
//   3. 合约管理：加载/保存合约配置（ctpcontract.yaml），维护 CTP 代码与 fullSymbol 映射
//
// 单例模式：通过 instance() 获取全局唯一实例，线程安全（双检锁）
//////////////////////////////////////////////////////////////////////////
class DataManager {
 public:
    static DataManager* pinstance_;   // 单例指针
    static mutex instancelock_;       // 单例创建时的互斥锁
    static DataManager& instance();   // 获取单例引用

    // ========== 数据记录 ==========
    TickWriter recorder_;            // Tick 写入器，用于文件/MongoDB 持久化

    // ========== 行情数据 ==========
    uint64_t count_ = 0;             // 计数器（预留）
    std::map<string, Tick> orderBook_;  // 最新行情簿，key 为 fullSymbol_

    // ========== 合约与符号映射 ==========
    bool contractUpdated_ = false;   // 合约是否已更新（预留）
    bool saveSecurityFile_ = false;  // 是否需保存合约到文件（预留）
    std::map<std::string, Security> securityDetails_;  // CTP 合约代码 -> Security 详情
    std::map<string, string> ctp2Full_;  // CTP 代码 -> fullSymbol（如 cu2401 -> SHFE F cu 2401）
    std::map<string, string> full2Ctp_;  // fullSymbol -> CTP 代码

    DataManager();
    ~DataManager();
    void reset();                    // 清空 orderBook、securityDetails 等
    void rebuild();                  // 重建数据结构（当前为空实现）
    void updateOrderBook(const Tick& k) { orderBook_[k.fullSymbol_] = k; }  // 用 Tick 更新行情
    void updateOrderBook(const Fill& fill);  // 用成交更新行情（仅更新 price/size）
    void saveSecurityToFile();       // 将 securityDetails 写入 etc/ctpcontract.yaml
    void loadSecurityFile();         // 从 etc/ctpcontract.yaml 加载合约配置
};
}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_DATA_DATAMANAGER_H_
