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

#include <Common/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <stddef.h>
#include <mutex>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <boost/locale.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::posix_time;
using namespace boost::local_time;
using namespace std;

namespace StarQuant {

// signal handler

Except_frame g_except_stack = Except_frame();  // 全局异常跳转栈

// 输出当前调用栈（用于调试/崩溃分析，依赖 backtrace）
void errorDump() {
    void* array[30];
    size_t size;
    char** strings;
    size_t i;

    size = backtrace(array, 30);
    strings = backtrace_symbols(array, size);
    if (NULL == strings) {
        perror("backtrace_symbols");
    }

    printf("Obtained %zd stack frames.\n", size);

    for (i = 0 ; i < size; i++) {
        std::cout << array[i] << std::endl;
        printf("%s\n", strings[i]);
    }

    free(strings);
    strings = NULL;
}

// 信号处理入口：打印信号、输出堆栈后跳转至 setjmp 保存点
void recvSignal(int32_t sig) {
    printf("StarQuant received signal %d !\n", sig);
    errorDump();
    siglongjmp(g_except_stack.env, 1);
}

// console related
std::atomic<bool> gShutdown{ false };

#if defined(_WIN64) || defined(_WIN32)
// 优化编译过程，通过减少不必要的头文件加载
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
    // Windows 控制台事件回调：Ctrl+C、关闭窗口等触发优雅退出
    BOOL CtrlHandler(DWORD fdwCtrlType) {
        switch (fdwCtrlType) {
            case CTRL_C_EVENT:
            case CTRL_CLOSE_EVENT:
                PRINT_SHUTDOWN_MESSAGE;
                gShutdown = true;
                return(TRUE);

            case CTRL_BREAK_EVENT:
            case CTRL_LOGOFF_EVENT:
            case CTRL_SHUTDOWN_EVENT:
                PRINT_SHUTDOWN_MESSAGE;
                gShutdown = true;
                return FALSE;

            default:
                return FALSE;
        }
    }

    // 注册控制台事件处理，返回 gShutdown 指针供主循环检查
    std::atomic<bool>* setconsolecontrolhandler(void) {
        bool b = SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
        if (!b) {
            printf("\nERROR: Could not set control handler");
        }
        return &gShutdown;
    }

#elif defined(__linux__)
    // Linux 信号处理：Ctrl+C 等触发优雅退出
    void ConsoleControlHandler(int32_t sig) {
        gShutdown = true;
        PRINT_SHUTDOWN_MESSAGE;
    }

// std::atomic<bool>* setconsolecontrolhandler(void) {
// signal(SIGINT, ConsoleControlHandler);
// signal(SIGPWR, ConsoleControlHandler);
// return &gShutdown;
// }

#endif
// int32_t check_gshutdown(bool force) {
// atomic_bool* g = setconsolecontrolhandler();
// while (!*g) {
// msleep(1 * 1000);
// }
// // ctrl-c
// if (force) {
// throw runtime_error("Throw an exception to trigger shutdown");
// }
// return 0;
// }



// ========== 字符串相关 ==========

// 从合约全称提取首段（按空格分隔，如 "IF2403 CFFEX" -> "IF2403"）
string extractExchangeID(const string& fullsym) {
    string ex;
    stringstream ss(fullsym);
    getline(ss, ex, ' ');
    return ex;
}

// 按分隔符拆分字符串，末尾空字符会被移除
vector<string> stringsplit(const string &s, char delim) {
    vector<string> elems;

    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    string& last = elems.back();
    if (last.back() == '\0')
        last.pop_back();
    return elems;
}

// 判断字符串 x 是否以 y 为前缀
bool startwith(const string& x, const string& y) {
    return x.find(y) == 0;
}




// 判断字符串是否以指定后缀结尾
bool endwith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}



// UTF-8 转 GB18030（用于 Windows 中文环境）
string UTF8ToGBK(const std::string & strUTF8) {
    string stroutGBK = "";
    stroutGBK = boost::locale::conv::between(strUTF8, "GB18030", "UTF-8");
    return stroutGBK;
}

// GB18030 转 UTF-8
string GBKToUTF8(const std::string & strGBK) {
    string strOutUTF8 = "";
    strOutUTF8 = boost::locale::conv::between(strGBK, "UTF-8", "GB18030");
    return strOutUTF8;
}


// ========== 数值相关 ==========

// 四舍五入保留 n 位小数（通过 sprintf + strtod 实现）
double rounded(double x, int32_t n) {
    char out[64];
    double xrounded;
    sprintf(out, "%.*f", n, x);
    xrounded = strtod(out, 0);
    return xrounded;
}

// ========== 时间相关 ==========

// 获取当前微秒级时间戳（跨平台：Windows/Linux/macOS）
uint64_t getMicroTime() {
    uint64_t t = 0;

#if defined(_WIN32) || defined(_WIN64)
        FILETIME tm;
#if defined(NTDDI_WIN8) && NTDDI_VERSION >= NTDDI_WIN8
        /* Windows 8, Windows Server 2012 and later. ---------------- */
        GetSystemTimePreciseAsFileTime(&tm);
#else
        /* Windows 2000 and later. ---------------------------------- */
        GetSystemTimeAsFileTime(&tm);
#endif
        t = ((uint64_t)tm.dwHighDateTime << 32) | (uint64_t)tm.dwLowDateTime;
        return t;

#elif (defined(__hpux) || defined(hpux)) || ((defined(__sun__) || defined(__sun) || defined(sun)) && (defined(__SVR4) || defined(__svr4__)))
        /* HP-UX, Solaris. ------------------------------------------ */
        return (double)gethrtime() / 1000000000.0;

#elif defined(__MACH__) && defined(__APPLE__)
        /* OSX. ----------------------------------------------------- */
        static double timeConvert = 0.0;
        if (timeConvert == 0.0) {
            mach_timebase_info_data_t timeBase;
            (void)mach_timebase_info(&timeBase);
            timeConvert = (double)timeBase.numer /
                (double)timeBase.denom /
                1000000000.0;
        }
        return (double)mach_absolute_time() * timeConvert;

#elif defined(_POSIX_VERSION)
        /* POSIX. --------------------------------------------------- */
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
        {
            struct timespec ts;
#if defined(CLOCK_MONOTONIC_PRECISE)
            /* BSD. --------------------------------------------- */
            const clockid_t id = CLOCK_MONOTONIC_PRECISE;
#elif defined(CLOCK_MONOTONIC_RAW)
            /* Linux. ------------------------------------------- */
            const clockid_t id = CLOCK_MONOTONIC_RAW;
#elif defined(CLOCK_HIGHRES)
            /* Solaris. ----------------------------------------- */
            const clockid_t id = CLOCK_HIGHRES;
#elif defined(CLOCK_MONOTONIC)
            /* AIX, BSD, Linux, POSIX, Solaris. ----------------- */
            const clockid_t id = CLOCK_MONOTONIC;
#elif defined(CLOCK_REALTIME)
            /* AIX, BSD, HP-UX, Linux, POSIX. ------------------- */
            const clockid_t id = CLOCK_REALTIME;
#else
            const clockid_t id = (clockid_t)-1; /* Unknown. */
#endif /* CLOCK_* */
            if (id != (clockid_t)-1 && clock_gettime(id, &ts) != -1) {
                t = ts.tv_sec*__NANOO_MULTIPLE__ + ts.tv_nsec;
                return t / 1000;
            }
            /* Fall thru. */
        }
#endif /* _POSIX_TIMERS */

        /* AIX, BSD, Cygwin, HP-UX, Linux, OSX, POSIX, Solaris. ----- */
        struct timeval tm;
        gettimeofday(&tm, nullptr);
        t = __MICRO_MULTIPLE__*tm.tv_sec + tm.tv_usec;
        return t;
#else
        return 0;  /* Failed. */
#endif
}

// 获取当日 0 点起的毫秒数
int32_t getMilliSeconds() {
    struct timeval tm;
    int32_t milli = 0;
    gettimeofday(&tm, nullptr);
    milli = int32_t(tm.tv_usec / 1000);
    return milli;
}

// 当前日期：YYYY-MM-DD
string ymd() {
    char buf[128] = { 0 };
    const size_t sz = sizeof("0000-00-00");
    {
        time_t timer;
        struct tm tm_info;
        time(&timer);
        localtime_r(&timer, &tm_info);
        strftime(buf, sz, DATE_FORMAT, &tm_info);
    }
    return string(buf);
}

// 当前日期紧凑格式：YYYYMMDD
string ymdcompact() {
    char buf[128] = { 0 };
    const size_t sz = sizeof("00000000");
    {
        time_t timer;
        struct tm tm_info;
        time(&timer);
        localtime_r(&timer, &tm_info);
        strftime(buf, sz, DATE_FORMAT_COMPACT, &tm_info);
    }
    return string(buf);
}

// 当前日期时间：YYYY-MM-DD HH:MM:SS
string ymdhms() {
    char buf[128] = { 0 };
    const size_t sz = sizeof("0000-00-00 00-00-00");
    {
        time_t timer;
        time(&timer);
        struct tm tm_info;
        localtime_r(&timer, &tm_info);
        strftime(buf, sz, DATE_TIME_FORMAT, &tm_info);
    }
    return string(buf);
}

// 当前日期时间含毫秒：YYYY-MM-DD HH:MM:SS.fff
string ymdhmsf() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
    time_t tt = std::chrono::system_clock::to_time_t(now);

    // tm t = *gmtime(&tt);
    struct tm t;
    localtime_r(&tt, &t);

    char buf[64];
    std::sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u.%03u", t.tm_year + 1900,
        t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
        static_cast<unsigned>(tp / std::chrono::milliseconds(1)));

    return string(buf);
}

// 当前日期时间含微秒：YYYY-MM-DD HH:MM:SS.ffffff
string ymdhmsf6() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
    time_t tt = std::chrono::system_clock::to_time_t(now);

    struct tm t;
    localtime_r(&tt, &t);

    char buf[64];
    std::sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u.%06u", t.tm_year + 1900,
        t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
        static_cast<unsigned>(tp / std::chrono::microseconds(1)));

    return string(buf);
}

// 当前时分秒毫秒：HH:MM:SS.fff
string hmsf() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
    time_t tt = std::chrono::system_clock::to_time_t(now);

    struct tm t;
    localtime_r(&tt, &t);

    char buf[64];
    std::sprintf(buf, "%02u:%02u:%02u.%03u", t.tm_hour, t.tm_min, t.tm_sec,
        static_cast<unsigned>(tp / std::chrono::milliseconds(1)));

    return string(buf);
}

// 时分秒字符串（HH:MM:SS）转整型时间 HHMMSS
int32_t hmsf2inttime(string hmsf) {
    return std::stoi(hmsf.substr(0, 2)) * 10000 + std::stoi(hmsf.substr(3, 2)) * 100 + std::stoi(hmsf.substr(6, 2));
}

// 毫秒级休眠
void msleep(uint64_t _ms) {
    if (_ms == 0) { return; }
    this_thread::sleep_for(chrono::milliseconds(_ms));
}

// 当前时间字符串（含毫秒）：YYYY-MM-DD HH:MM:SS.fff
string nowMS() {
    char buf[128] = {};
#ifdef __linux__
        struct timespec ts = { 0, 0 };
        struct tm tm = {};
        char timbuf[64] = {};
        clock_gettime(CLOCK_REALTIME, &ts);
        time_t tim = ts.tv_sec;
        localtime_r(&tim, &tm);
        strftime(timbuf, sizeof(timbuf), "%F %T", &tm);
        snprintf(buf, 128, "%s.%03d", timbuf, (int32_t)(ts.tv_nsec / 1000000));
#else
        SYSTEMTIME SystemTime;
        GetLocalTime(&SystemTime);
        sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
            SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
            SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
#endif
        return buf;
    }

    // boost::ptime 转 time_t（按纽约时区偏移）
    time_t ptime2time(ptime t) {
        static ptime epoch(boost::gregorian::date(1970, 1, 1));
        time_duration::sec_type x = (t - epoch).total_seconds() - 3600 * NYC_TZ_OFFSET;
        // hours(4).total_seconds() = 3600 * 4
        // ... check overflow here ...
        return time_t(x);
    }

    // boost::ptime 转日期时间字符串（纽约时区）
    string ptime2str(const ptime& pt) {
        time_zone_ptr tz_cet(new boost::local_time::posix_time_zone(NYC_TZ_STR));
        local_date_time dt_with_zone(pt, tz_cet); // glocale::instance()._ny_tzone);
#if 1
        tm _t = to_tm(dt_with_zone);
        char buf[32] = { 0 };
        strftime(buf, 32, DATE_TIME_FORMAT, &_t);
        return buf;
#else
// using stringstream only for logging
        stringstream strm;
        strm.imbue(*glocale::instance()._s_loc);
        strm << dt_with_zone;
        // strm << pt;
        return strm.str();
#endif
    }


// 时间字符串（如 "2024-01-15 14:30:00"）转 time_t
time_t str2time_t(const string& s) {
    ptime pt(time_from_string(s));
    return ptime2time(pt);
}

// time_t 转简单时间字符串
string time_t2str(time_t tt) {
    ptime pt = from_time_t(tt);
    // return ptime2str(pt);
    return to_simple_string(pt);
}

// 当前日期整型 YYYYMMDD
int32_t tointdate() {
    time_t current_time;
    time(&current_time);
    return tointdate(current_time);
}

// 当前时间整型 HHMMSS
int32_t tointtime() {
    time_t current_time;
    time(&current_time);
    return tointtime(current_time);
}

// 指定 time_t 的日期整型 YYYYMMDD
int32_t tointdate(time_t time) {
    struct tm timeinfo;
    LOCALTIME_S(&timeinfo, &time);

    return ((timeinfo.tm_year + 1900) * 10000) + ((timeinfo.tm_mon + 1) * 100) + timeinfo.tm_mday;
}

// 指定 time_t 的时间整型 HHMMSS
int32_t tointtime(time_t time) {
    // std::time_t rawtime;
    // std::tm* timeinfo;
    // char queryTime[80];
    // std::time(&rawtime);
    // timeinfo = std::localtime(&rawtime);
    // std::strftime(queryTime, 80, "%Y%m%d %H:%M:%S", timeinfo);
    struct tm timeinfo;
    LOCALTIME_S(&timeinfo, &time);

    return (timeinfo.tm_hour * 10000) + (timeinfo.tm_min * 100) + (timeinfo.tm_sec);
}

// 整型时间 HHMMSS 转秒数
int32_t inttimetointtimespan(int32_t time) {
    int32_t s1 = time % 100;
    int32_t m1 = ((time - s1) / 100) % 100;
    int32_t h1 = (int32_t)((time - (m1 * 100) - s1) / 10000);

    return h1 * 3600 + m1 * 60 + s1;
}

// 秒数转整型时间 HHMMSS
int32_t inttimespantointtime(int32_t timespan) {
    int32_t hour = timespan / 3600;
    int32_t second = timespan % 3600;
    int32_t minute = second / 60;
    second = second % 60;
    return (hour * 10000 + minute * 100 + second);
}

// 整型时间加秒数（不处理 24 小时跨日）
int32_t inttimeadd(int32_t firsttime, int32_t timespaninseconds) {
    int32_t s1 = firsttime % 100;
    int32_t m1 = ((firsttime - s1) / 100) % 100;
    int32_t h1 = (int32_t)((firsttime - m1 * 100 - s1) / 10000);
    s1 += timespaninseconds;
    if (s1 >= 60) {
        m1 += (int32_t)(s1 / 60);
        s1 = s1 % 60;
    }
    if (m1 >= 60) {
        h1 += (int32_t)(m1 / 60);
        m1 = m1 % 60;
    }
    int32_t sum = h1 * 10000 + m1 * 100 + s1;
    return sum;
}

// 两整型时间差（秒），latertime - firsttime
int32_t inttimediff(int32_t firsttime, int32_t latertime) {
    int32_t span1 = inttimetointtimespan(firsttime);
    int32_t span2 = inttimetointtimespan(latertime);
    return span2 - span1;
}




// 时间字符串（YYYY-MM-DD HH:MM:SS.fff）转 Unix 毫秒时间戳
int64_t string2unixtimems(const string& s) {
    struct tm tm_;
    int64_t unixtimems;
    int32_t year, month, day, hour, minute, second, millisec;
    sscanf(s.c_str(), "%d-%d-%d %d:%d:%d.%d", &year, &month, &day, &hour, &minute, &second, &millisec);
    tm_.tm_year  = year - 1900;
    tm_.tm_mon   = month - 1;
    tm_.tm_mday  = day;
    tm_.tm_hour  = hour;
    tm_.tm_min   = minute;
    tm_.tm_sec   = second;
    tm_.tm_isdst = 0;

    time_t t_ = mktime(&tm_);
    unixtimems = t_ * 1000 + millisec;
    return unixtimems;
}


}  // namespace StarQuant
