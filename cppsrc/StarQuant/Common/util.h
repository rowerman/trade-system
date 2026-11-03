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
#ifndef CPPSRC_STARQUANT_COMMON_UTIL_H_
#define CPPSRC_STARQUANT_COMMON_UTIL_H_

#include <inttypes.h>
#include <sstream>
#include <iterator>
#include <atomic>
#include <chrono>  // for msleep
#include <thread>  // for msleep
#include <ctime>
#include <mutex>
#include <set>
#include <vector>
#include <string>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/local_time/local_time_types.hpp>
#include <boost/date_time/local_time/local_time_io.hpp>

using std::string;
using std::vector;
using std::pair;
using std::mutex;
using std::set;
using std::locale;
using boost::posix_time::ptime;


namespace StarQuant {
// macros

#define PRINT_SHUTDOWN_MESSAGE \
printf("\n Thank you for using Star Quant. Goodbye! \n")  // 程序退出时打印的告别信息

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>  // Windows 平台：高精度计时等

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>   // POSIX 平台：POSIX 标志
#include <time.h>    // clock_gettime()、time()
#include <sys/time.h> // gethrtime()、gettimeofday()

#if defined(__MACH__) && defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>  // macOS：mach 高精度时间
#endif

#else
#error "Unable to define getRealTime( ) for an unknown OS."  // 未知系统，无法定义 getRealTime
#endif

#define __MILLI_MULTIPLE__ 1000        // 毫秒换算倍数（1 秒 = 1000 毫秒）
#define __MICRO_MULTIPLE__ 1000000     // 微秒换算倍数（1 秒 = 10^6 微秒）
#define __NANOO_MULTIPLE__ 1000000000  // 纳秒换算倍数（1 秒 = 10^9 纳秒）

#ifdef __linux__
#define LOCALTIME_S(x, y) localtime_r(y, x)  // Linux：线程安全的本地时间转换
#else
#define LOCALTIME_S(x, y) localtime_s(x, y)  // Windows：线程安全的本地时间转换
#endif
#define DATE_FORMAT "%Y-%m-%d"                    // 日期格式：2024-01-15
#define DATE_FORMAT_COMPACT "%Y%m%d"               // 紧凑日期格式：20240115
#define DATE_FORMAT_CLEAN  "%4d-%02d-%02d"         // 日期 printf 格式（年-月-日）
#define DATE_TIME_FORMAT "%Y-%m-%d %H:%M:%S"       // 日期时间格式：2024-01-15 14:30:00
#define DATE_TIME_FORMAT_CLEAN  "%4d-%02d-%02d %02d:%02d:%02d"  // 日期时间 printf 格式

#define TIMEZONE_STRING(s) #s           // 将宏参数转为字符串
#define NYC_TZ_OFFSET -04               // 纽约时区偏移（夏令时 UTC-4）
#define NYC_TZ_STR "UTC" TIMEZONE_STRING(NYC_TZ_OFFSET) ":00:00"  // 纽约时区字符串

#define EPSILON (1e-6)    // 浮点比较容差（用于判断相等）
#define DOUBLEMAX (1e16)  // double 有效最大值（约一亿亿，2018 年 A 股总市值不到 50 万亿）



// console ,control related
    // std::atomic<bool>* setconsolecontrolhandler(void);
    // int32_t check_gshutdown(bool force = true);

// except signal
#include <setjmp.h>
#include <string.h>

struct Except_frame {  // 异常/信号跳转帧（配合 setjmp/longjmp）
    jmp_buf env;       // 跳转环境缓冲区
    int32_t flag;      // 是否已设置跳转点（setjmp 返回值）
    void clear() {
        flag = 0;
        bzero(env, sizeof(env));
    }
    bool isDef() {
        return flag;
    }
    Except_frame() {
        clear();
    }
};
void errorDump();              // 发生错误时输出堆栈等信息
void recvSignal(int32_t sig);   // 信号处理入口（如 SIGSEGV、SIGINT）
void ConsoleControlHandler(int32_t sig);  // Windows 控制台事件处理（Ctrl+C 等）


// string related

vector<string> stringsplit(const string &s, char delim);  // 按分隔符拆分字符串
bool startwith(const string&, const string&);             // 判断是否以指定前缀开头
bool endwith(const std::string &str, const std::string &suffix);  // 判断是否以指定后缀结尾
string extractExchangeID(const std::string& fullsym);     // 从合约全称提取交易所 ID（如 IF2403.CFFEX -> CFFEX）
string UTF8ToGBK(const std::string & strUTF8);           // UTF-8 转 GBK 编码
string GBKToUTF8(const std::string & strGBK);             // GBK 转 UTF-8 编码

// 数值工具
double rounded(double x, int32_t n = 3);  // 四舍五入，保留 n 位小数（默认 3 位）

inline bool is_greater(double x, double y) {
    return std::isgreater(x - y, EPSILON);  // x > y（考虑浮点容差）
}

inline bool is_less(double x, double y) {
    return std::isless(x - y, EPSILON * -1);  // x < y（考虑浮点容差）
}

inline bool is_equal(double x, double y) {
    return std::abs(x - y) <= EPSILON * std::abs(x);  // x ≈ y（相对误差容差）
}

inline bool is_greater_equal(double x, double y) {
    return is_greater(x, y) || is_equal(x, y);  // x >= y
}

inline bool is_less_equal(double x, double y) {
    return is_less(x, y) || is_equal(x, y);  // x <= y
}

inline bool is_zero(double x) {
    return is_equal(x, 0.0);  // 判断是否近似为零
}

inline bool is_too_large(double x) {
    return is_greater(x, DOUBLEMAX);  // 判断是否超出有效范围
}


// time related

uint64_t getMicroTime();           // 获取当前微秒级时间戳
int32_t getMilliSeconds();        // 获取当前毫秒数（当日 0 点起）
string ymd();                     // 当前日期：YYYY-MM-DD
string ymdcompact();              // 当前日期紧凑格式：YYYYMMDD
string ymdhms();                  // 当前日期时间：YYYY-MM-DD HH:MM:SS
string ymdhmsf();                 // 当前日期时间含毫秒：YYYY-MM-DD HH:MM:SS.fff
string ymdhmsf6();                // 当前日期时间含微秒：YYYY-MM-DD HH:MM:SS.ffffff
string hmsf();                    // 当前时分秒毫秒：HH:MM:SS.fff
int32_t hmsf2inttime(string hmsf);  // 时分秒字符串转整型时间（HHMMSS）

void msleep(uint64_t _ms);        // 毫秒级休眠
string nowMS();                   // 当前时间毫秒字符串

string ptime2str(const ptime& pt);   // boost::ptime 转字符串
time_t str2time_t(const string& s);  // 时间字符串转 time_t
string time_t2str(time_t tt);       // time_t 转字符串
time_t ptime2time(ptime t);         // boost::ptime 转 time_t

int32_t tointdate();                // 当前日期整型：YYYYMMDD
int32_t tointtime();                // 当前时间整型：HHMMSS
int32_t tointdate(time_t time);     // 指定 time_t 的日期整型
int32_t tointtime(time_t time);     // 指定 time_t 的时间整型
int32_t inttimetointtimespan(int32_t time);   // 整型时间（HHMMSS）转秒数
int32_t inttimespantointtime(int32_t timespan);  // 秒数转整型时间（HHMMSS）
int32_t inttimeadd(int32_t firsttime, int32_t timespaninseconds);  // 整型时间加秒数
int32_t inttimediff(int32_t firsttime, int32_t latertime);  // 两整型时间差（秒）
int64_t string2unixtimems(const string& s);  // 时间字符串转 Unix 毫秒时间戳


}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_COMMON_UTIL_H_
