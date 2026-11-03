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

#include <Common/logger.h>
#include <Common/util.h>
#include <Common/config.h>

#include <log4cplus/initializer.h>
#include <log4cplus/configurator.h>   // PropertyConfigurator
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loglevel.h>
#ifdef _WIN32
#include <nanomsg/src/nn.h>
#include <nanomsg/src/pubsub.h>
#else
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#endif
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace fs = boost::filesystem;
using std::lock_guard;
using namespace log4cplus;

namespace StarQuant {

// ========== logger 类（轻量级文件日志） ==========

logger* logger::pinstance_ = nullptr;
mutex logger::instancelock_;

/**
 * 构造函数：初始化 logfile 后立即调用 Initialize 创建日志文件
 */
logger::logger() : logfile(nullptr) {
    Initialize();
}

/**
 * 析构函数：关闭日志文件句柄
 */
logger::~logger() {
    fclose(logfile);
}

/**
 * 获取 logger 单例（双重检查锁）
 */
logger& logger::instance() {
    if (pinstance_ == nullptr) {
        lock_guard<mutex> g(instancelock_);
        if (pinstance_ == nullptr) {
            pinstance_ = new logger();
        }
    }
    return *pinstance_;
}

/**
 * 根据运行模式创建日志文件
 * 回放模式：starequant-replay-YYYY-MM-DD.txt
 * 其他模式：starquant-YYYY-MM-DD.txt
 */
void logger::Initialize() {
    string fname;

    if (CConfig::instance()._mode == RUN_MODE::REPLAY_MODE) {
        fname = CConfig::instance().logDir() + "/starequant-replay-" + ymd() + ".txt";
    } else {
        fname = CConfig::instance().logDir() + "/starquant-" + ymd() + ".txt";
    }

    logfile = fopen(fname.c_str(), "w");
    setvbuf(logfile, nullptr, _IONBF, 0);  // 无缓冲，立即落盘（便于调试崩溃）
}

/**
 * 按 printf 格式写入日志文件，自动加时间戳
 * 格式：YYYY-MM-DD HH:MM:SS.fff 用户内容
 */
void logger::Printf2File(const char *format, ...) {
    lock_guard<mutex> g(instancelock_);  // 多线程安全

    static char buf[1024 * 2];
    string tmp = nowMS();
    size_t sz = tmp.size();
    strcpy(buf, tmp.c_str());
    buf[sz] = ' ';  // 时间戳与内容之间加空格

    va_list args;
    va_start(args, format);
    vsnprintf(buf + sz + 1, 1024 * 2 - sz - 1, format, args);  // 格式化到 buf 后半部分
    size_t buflen = strlen(buf);
    fwrite(buf, sizeof(char), buflen, logfile);
    va_end(args);
}

// ========== SQLogger 类（log4cplus 封装） ==========

static bool configured = false;  // 全局标记：是否已加载 log4cplus 配置

/**
 * 加载 log4cplus 配置文件（仅首次调用生效）
 * @param configureName 配置文件路径（如 etc/config_log）
 * @return true 首次配置成功；已配置过返回 false
 */
bool SQLogger::doConfigure(string configureName) {
    if (!configured) {
        log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(configureName));
        configured = true;
        return true;
    } else {
        return false;
    }
}

/**
 * 按名称获取 SQLogger 实例
 * 构造时自动调用 doConfigure 加载 config_log
 */
std::shared_ptr<SQLogger> SQLogger::getLogger(string name) {
    return std::shared_ptr<SQLogger>(new SQLogger(name));
}

/**
 * 构造函数：加载配置并获取 log4cplus Logger
 * @param name logger 名称，用于区分不同模块（如 Engine、Strategy）
 */
SQLogger::SQLogger(string name) {
    doConfigure(CConfig::instance().logconfigfile_);  // 从 config 读取配置路径
    logger = log4cplus::Logger::getInstance(name);
}

string SQLogger::getConfigFolder() {
    return CConfig::instance().configDir();
}

}  // namespace StarQuant
