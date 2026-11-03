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

#include <Common/config.h>
#include <Common/util.h>
#include <Common/datastruct.h>

#include <yaml-cpp/yaml.h>   // YAML 解析，读取 config_server.yaml
#include <fmt/format.h>      // 格式化输出
#include <fstream>
#include <cctype>            // isalpha()、isdigit() 等字符分类
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>




namespace bpt = boost::property_tree;
namespace bpo = boost::program_options;
namespace fs = boost::filesystem;

namespace StarQuant {
    // 静态成员初始化
    CConfig* CConfig::pinstance_ = nullptr;
    mutex CConfig::instancelock_;

    /**
     * 私有构造函数：创建时自动调用 readConfig 加载配置
     */
    CConfig::CConfig() {
        readConfig();
    }

    /**
     * 获取配置单例（双重检查锁，保证多线程下只创建一次）
     */
    CConfig& CConfig::instance() {
        if (pinstance_ == nullptr) {
            std::lock_guard<mutex> g(instancelock_);
            if (pinstance_ == nullptr) {
                pinstance_ = new CConfig();
            }
        }
        return *pinstance_;
    }

    /**
     * 从 etc/config_server.yaml 加载全部配置
     * 包括：运行模式、目录、消息队列、网关、风控等
     */
    void CConfig::readConfig() {
        std::lock_guard<mutex> g(readlock_);  // 防止并发读配置
        try {
            // 配置文件路径：当前工作目录下的 etc/config_server.yaml
            string path = fs::current_path().string() + "/etc/config_server.yaml";
            YAML::Node config = YAML::LoadFile(path);

            // ========== 运行模式 ==========
            string configmode = config["mode"].as<std::string>();
            if (configmode == "record") {
                _mode = RUN_MODE::RECORD_MODE;  // 录播模式：记录行情到文件
            } else if (configmode == "replay") {
                _mode = RUN_MODE::REPLAY_MODE;
                _tickinterval = config["tickinterval"].as<int32_t>();   // tick 推送间隔（ms）
                _brokerdelay = config["brokerdelay"].as<int32_t>();    // 模拟经纪商延迟（ms）
                filetoreplay = config["filetoreplay"].as<std::string>();  // 回放数据文件路径
            }
            // 默认 trade 模式

            // ========== 目录配置 ==========
            _config_dir = boost::filesystem::current_path().string() + "/etc/";
            _log_dir = config["log_dir"].as<std::string>();
            _data_dir = config["data_dir"].as<std::string>();
            boost::filesystem::create_directory(logDir());   // 确保日志目录存在
            boost::filesystem::create_directory(dataDir());  // 确保数据目录存在
            logconfigfile_ = fs::current_path().string() + "/etc/config_log";

            // ========== 系统选项 ==========
            cpuaffinity = config["cpuaffinity"].as<bool>();   // 是否绑定 CPU
            autoconnect = config["autoconnect"].as<bool>();   // 启动时自动连接
            autoqry = config["autoqry"].as<bool>();           // 启动时自动查询

            // ========== 数据库 ==========
            _mongodbaddr = config["dbaddr"].as<std::string>();
            _mongodbname = config["dbname"].as<std::string>();

            // ========== 消息队列地址 ==========
            SERVERPUB_URL = config["serverpub_url"].as<std::string>();
            SERVERSUB_URL = config["serversub_url"].as<std::string>();
            SERVERPULL_URL = config["serverpull_url"].as<std::string>();

            // ========== 网关配置 ==========
            const std::vector<string> gws = config["gateway"].as<std::vector<string>>();
            for (auto s : gws) {
                struct Gateway gw;
                gw.id = s;
                gw.intid = config[s]["intid"].as<int32_t>();
                gw.api = config[s]["api"].as<std::string>();       // CTP/TAP/XTP 等
                gw.brokerid = config[s]["brokerid"].as<std::string>();
                auto mdips = config[s]["md_address"].as<std::vector<string>>();
                gw.md_address.assign(mdips.begin(), mdips.end());  // 行情服务器地址列表
                auto tdips = config[s]["td_address"].as<std::vector<string>>();
                gw.td_address.assign(tdips.begin(), tdips.end());  // 交易服务器地址列表
                gw.userid = config[s]["userid"].as<std::string>();
                gw.password = config[s]["password"].as<std::string>();
                gw.auth_code = config[s]["auth_code"].as<std::string>();      // CTP 等需填
                gw.productinfo = config[s]["user_prod_info"].as<std::string>();
                gw.appid = config[s]["appid"].as<std::string>();              // 部分 API 需填
                gw.publicstream = config[s]["publicstream"].as<std::string>();  // XTP 公有流
                gw.privatestream = config[s]["privatestream"].as<std::string>(); // XTP 私有流
                _gatewaymap[s] = gw;  // 以网关 ID 为 key 存入映射
            }

            // ========== 风控参数 ==========
            riskcheck = config["risk"]["check"].as<bool>();
            sizeperorderlimit = config["risk"]["sizeperorder"].as<int32_t>();   // 单笔数量上限
            cashperorderlimit = config["risk"]["cashperorder"].as<double>();    // 单笔金额上限
            ordercountlimit = config["risk"]["ordercount"].as<int32_t>();       // 总订单数上限
            cashlimit = config["risk"]["cash"].as<double>();                    // 总资金上限
            ordersizelimit = config["risk"]["ordersize"].as<int32_t>();         // 单合约持仓上限
            ordercountperseclimit = config["risk"]["ordercountpersec"].as<int32_t>();  // 每秒下单上限
        }
        catch (exception &e) {
            fmt::print("Read Config exception:{}.", e.what());
        }
        catch (...) {
            fmt::print("Read Config error!");
        }
    }

    string CConfig::configDir() {
        return _config_dir;
    }

    string CConfig::logDir() {
        return _log_dir;
    }

    string CConfig::dataDir() {
        return _data_dir;
    }

    /**
     * 完整证券代码转 CTP 合约代码
     * 输入格式： "品种 合约月 交易所 年月" 如 "IF 2403 CFFEX 202403"
     * 输出格式： CTP 合约代码如 "IF2403"
     * 依赖 sec2instrument 映射（需在合约列表中预先加载）
     */
    string CConfig::SecurityFullNameToCtpSymbol(const std::string& symbol) {
        vector<string> v = stringsplit(symbol, ' ');
        string tmp = v[0] + " " + v[1] + " " + v[2];   // 品种+合约月+交易所
        string ctbticker = sec2instrument[tmp] + v[3];  // 取 CTP 前缀 + 年月
        return ctbticker;
    }

    /**
     * CTP 合约代码转完整证券代码
     * 输入格式： "IF2403" 等
     * 输出格式： "IF 2403 CFFEX 202403"
     * 依赖 instrument2sec 映射，通过去掉数字得到合约前缀再查表
     */
    string CConfig::CtpSymbolToSecurityFullName(const std::string& symbol) {
        string num = symbol;
        string alpha = symbol;
        num.erase(std::remove_if(num.begin(), num.end(), &::isalpha), num.end());   // 只保留数字
        alpha.erase(std::remove_if(alpha.begin(), alpha.end(), &::isdigit), alpha.end());  // 只保留字母
        string tmp = instrument2sec[alpha];  // 合约前缀 -> 完整代码前缀
        string fullsymbol = tmp + " " + num;
        return fullsymbol;
    }








}
