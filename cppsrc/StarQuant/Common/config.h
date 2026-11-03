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
#ifndef CPPSRC_STARQUANT_COMMON_CONFIG_H_
#define CPPSRC_STARQUANT_COMMON_CONFIG_H_

#include <Common/datastruct.h>
#include <inttypes.h>       // 固定宽度整数类型（int32_t、int64_t 等）
#include <cmath>            // 数学函数
#include <mutex>            // 互斥锁，用于线程同步
#include <atomic>           // 原子操作，用于无锁并发
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <list>
#include <deque>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>  // 跨平台文件/目录操作
// #define CEREAL_RAPIDJSON_NAMESPACE creal_rapidjson
// #include <cereal/types/unordered_map.hpp>
// #include <cereal/types/memory.hpp>
// #include <cereal/archives/json.hpp>
// #include <cereal/types/vector.hpp>
// #include <cereal/types/string.hpp>
// #include <cereal/types/map.hpp>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // 排除不常用 Windows API，加快编译
#include <Windows.h>         // Windows 系统 API
#include <WinSock2.h>       // Windows 套接字（消息队列网络通信）
#else
#include <unistd.h>         // Unix/Linux 系统调用（如 getpid、sleep）
#endif

using std::vector;
using std::string;
using std::set;
using std::mutex;
using std::map;
using std::atomic_int;



namespace StarQuant {

/**
 * 全局配置类（单例）
 * 从 config_server.yaml 读取运行模式、网关、消息队列、风控等配置
 */
class CConfig {
    static CConfig* pinstance_;      // 单例实例指针
    static mutex instancelock_;      // 双重检查锁，保证实例创建线程安全

    CConfig();                       // 私有构造函数，禁止外部直接构造

 public:
    RUN_MODE _mode = RUN_MODE::TRADE_MODE;   // 运行模式：交易/录播/回放
    BROKERS _broker = BROKERS::PAPER;       // 经纪商类型（默认模拟盘）
    MSGQ _msgq = MSGQ::NANOMSG;              // 消息队列实现（nanomsg/zmq/kafka）
    int32_t _tickinterval = 0;               // 回放模式下的 tick 推送间隔（毫秒）
    int32_t _brokerdelay = 0;                // 回放模式下模拟经纪商延迟（毫秒）
    /** 获取配置单例引用（双重检查锁实现线程安全） */
    static CConfig& instance();
    map<string, Gateway> _gatewaymap;        // 网关 ID -> Gateway 配置映射
    mutex readlock_;                         // 读配置时的互斥锁
    /** 从 etc/config_server.yaml 加载配置（模式、网关、风控、消息队列等） */
    void readConfig();

    string _config_dir;                      // 配置文件目录（如 ./etc/）
    string _log_dir;                         // 日志文件目录
    string _data_dir;                        // 数据文件目录（录播/回放）
    string configDir();                     // 返回配置目录路径（_config_dir）
    string logDir();                        // 返回日志目录路径（_log_dir）
    string dataDir();                       // 返回数据目录路径（_data_dir）
    string logconfigfile_;                   // 日志配置文件路径（如 ./etc/config_log）

    /************* 合约/证券列表 ****************/
    vector<string> securities;               // 完整证券代码列表（如 "IF 2403 CFFEX 202403"）
    map<string, string> instrument2sec;      // 合约前缀 -> 完整代码（如 "IF" -> "IF 2403 CFFEX"）
    map<string, string> sec2instrument;      // 完整代码 -> CTP 合约前缀（如 "IF 2403 CFFEX" -> "IF")
    /** 完整证券代码转 CTP 合约代码（如 "IF 2403 CFFEX 202403" -> "IF2403"） */
    string SecurityFullNameToCtpSymbol(const std::string& symbol);
    /** CTP 合约代码转完整证券代码（如 "IF2403" -> "IF 2403 CFFEX 202403"） */
    string CtpSymbolToSecurityFullName(const std::string& symbol);
    /******** End of Securities List ************/

    /******************* 数据库 ***************/
    string _mongodbaddr = "mongodb://localhost:27017";  // MongoDB 连接地址
    string _mongodbname = "";                           // MongoDB 数据库名
    string filetoreplay;                                // 回放模式下的数据文件路径
    /************* End of Database **********************/

    /*************** 消息队列地址 ******************/
    string SERVERPUB_URL = "tcp://localhost:55555";   // 服务端发布地址（PUB，广播给客户端）
    string SERVERSUB_URL = "tcp://localhost:55556";   // 服务端订阅地址（SUB，接收引擎请求）
    string SERVERPULL_URL = "tcp://localhost:55557";  // 服务端拉取地址（PULL，接收客户端请求）
    bool cpuaffinity = false;                          // 是否绑定 CPU 核心（减少上下文切换）
    /******************** Message Queue **********/

    /************************ 自动任务 *****************/
    bool autoconnect = true;   // 启动时是否自动连接行情/交易
    bool autoqry = false;      // 启动时是否自动查询账户/持仓

    /********* 风控参数 **************/
    bool riskcheck = false;              // 是否启用风控检查
    int32_t sizeperorderlimit = 0;       // 单笔订单数量上限
    double cashperorderlimit = 0.0;     // 单笔订单金额上限
    int32_t ordercountlimit = 0;        // 总订单数量上限
    double cashlimit = 0.0;             // 总资金/持仓金额上限
    int32_t ordersizelimit = 0;         // 单合约持仓数量上限
    int32_t ordercountperseclimit = 0;   // 每秒下单数量上限
    /*********** End Risk setting *********/
};
}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_COMMON_CONFIG_H_
