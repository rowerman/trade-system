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

#ifndef CPPSRC_STARQUANT_COMMON_LOGGER_H_
#define CPPSRC_STARQUANT_COMMON_LOGGER_H_

#include <Common/util.h>
#include <stdarg.h>   // va_list、va_start 等可变参数
#include <stdio.h>
#include <time.h>
#include <log4cplus/logger.h>      // log4cplus 日志库
#include <log4cplus/loggingmacros.h>
#include <string>
#include <memory>
#include <mutex>

// ========== 日志宏（简化 log4cplus 调用，传入 SQLogger 指针） ==========
#define LOG_FATAL(sqLogger, content) LOG4CPLUS_FATAL(sqLogger->getLogger(), content)
#define LOG_ERROR(sqLogger, content) LOG4CPLUS_ERROR(sqLogger->getLogger(), content)
#define LOG_INFO(sqLogger, content)  LOG4CPLUS_INFO(sqLogger->getLogger(), content)
#define LOG_DEBUG(sqLogger, content) LOG4CPLUS_DEBUG(sqLogger->getLogger(), content)

using std::string;
using std::mutex;

namespace StarQuant {

// ========== 打印宏（轻量级，不依赖 log4cplus） ==========
#define PRINT_TO_FILE logger::instance().Printf2File  // 仅写入日志文件
#define PRINT_TO__CONSOLE(...) do { \
    printf("%s ", ymdhmsf().c_str()); \
    printf(__VA_ARGS__); \
} while (0)  // 仅输出到控制台，带时间戳
#define PRINT_TO_FILE_AND_CONSOLE(...) do { \
    logger::instance().Printf2File(__VA_ARGS__); \
    printf("%s ", ymdhmsf().c_str()); \
    printf(__VA_ARGS__); \
} while (0)  // 同时写入文件和控制台

/**
 * 轻量级文件日志（单例）
 * 直接写入 txt 文件，按日期命名（starquant-YYYY-MM-DD.txt 或 starequant-replay-...）
 */
class logger {
    static logger* pinstance_;
    static mutex instancelock_;

    FILE* logfile = nullptr;  // 日志文件句柄
    logger();
    ~logger();

 public:
    static logger& instance();

    /** 初始化：根据运行模式创建日志文件（回放模式用 starequant-replay-*.txt） */
    void Initialize();

    /** 按 printf 格式写入日志文件，自动加时间戳，线程安全 */
    void Printf2File(const char *format, ...);
};

/**
 * 基于 log4cplus 的日志器（支持按名称、级别、配置）
 * 通过 config_log 配置输出格式、级别、文件等
 */
class SQLogger {
 protected:
    log4cplus::Logger logger;  // log4cplus 底层 logger
    SQLogger() {}
    explicit SQLogger(string name);  // 按名称创建，如 "Engine"、"Strategy"

 public:
    inline log4cplus::Logger& getLogger() {
        return logger;  // 供 LOG_FATAL 等宏使用
    }
    inline void fatal(const char* content) {
        LOG4CPLUS_FATAL(logger, content);
    }
    inline void error(const char* content) {
        LOG4CPLUS_ERROR(logger, content);
    }
    inline void info(const char* content) {
        LOG4CPLUS_INFO(logger, content);
    }
    inline void debug(const char* content) {
        LOG4CPLUS_DEBUG(logger, content);
    }

    /** 返回配置目录（用于解析 config_log 路径） */
    static string getConfigFolder();

    /**
     * 加载 log4cplus 日志配置
     * @param configureName 配置文件路径（如 etc/config_log）
     * @return true 仅当首次配置成功；已配置过则返回 false
     */
    static bool doConfigure(string configureName);

    /** 按名称获取 logger 实例（首次调用时自动 doConfigure） */
    static std::shared_ptr<SQLogger> getLogger(string name);
};

}  // namespace StarQuant
#endif  // CPPSRC_STARQUANT_COMMON_LOGGER_H_
