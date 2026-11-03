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

#ifndef CPPSRC_STARQUANT_DATA_TICKWRITER_H_
#define CPPSRC_STARQUANT_DATA_TICKWRITER_H_

#include <Common/config.h>
#include <Common/util.h>
#include <Common/datastruct.h>

#include <APIs/Ctp/ThostFtdcUserApiDataType.h>
#include <APIs/Ctp/ThostFtdcUserApiStruct.h>
#include <APIs/Tap/TapQuoteAPIDataType.h>
#include <APIs/Tap/TapAPICommDef.h>
#include <bson/bson.h>
#include <mongoc.h>
#include <time.h>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <string>

using std::mutex;

//////////////////////////////////////////////////////////////////////////
// TickWriter - Tick 行情数据记录器
//
// 功能概述：
//   1. 文件写入：将 Tick 行情数据以带时间戳的文本形式缓冲写入本地文件
//   2. MongoDB 写入：将 Tick 数据解析后持久化到 MongoDB 数据库（findata 库）
//
// 支持的数据类型：
//   - MSG_TYPE_TICK_L1：1 档深度行情（最新价、买卖一档）
//   - MSG_TYPE_TICK_L5：5 档深度行情（预留，当前未实现插入逻辑）
//
// 依赖：MongoDB C Driver (libmongoc)、BSON 库
//////////////////////////////////////////////////////////////////////////
namespace StarQuant {
struct TickWriter {
    // ========== 文件写入相关成员 ==========
    int32_t bufSize;           // 缓冲区大小（字节），默认 1024
    FILE* fp = nullptr;        // 写入目标文件句柄，需在调用 put() 前通过 open() 打开
    int32_t count = 0;        // 当前缓冲区中已写入的字符数（有效数据长度）
    char* head = nullptr;     // 缓冲区指针，用于累积待写入的字符串

    // ========== MongoDB 写入相关成员 ==========
    bson_error_t error;        // BSON/MongoDB 操作错误信息
    mongoc_client_pool_t *pool;  // MongoDB 客户端连接池，用于多线程复用连接
    mongoc_uri_t         *uri;   // MongoDB 连接 URI（如 mongodb://localhost:27017）

    /**
     * @brief 构造函数
     * 初始化缓冲区、MongoDB 驱动及连接池（默认连接本地 27017 端口）
     */
    TickWriter() {
        bufSize = 1024;
        head = new char[bufSize];
        mongoc_init();                                    // 初始化 MongoDB C 驱动
        uri = mongoc_uri_new("mongodb://localhost:27017"); // 解析连接字符串
        pool = mongoc_client_pool_new(uri);               // 创建客户端连接池
        }
    /**
     * @brief 析构函数
     * 将缓冲区剩余数据刷入文件、关闭文件、释放缓冲区及 MongoDB 资源
     */
    ~TickWriter() {
        if (fp) {
            fwrite(head, sizeof(char), count, fp);  // 将缓冲区剩余数据写入文件
            fflush(fp);                             // 确保数据落盘
            fclose(fp);
        }
        delete[] head;
        mongoc_client_pool_destroy(pool);
        mongoc_uri_destroy(uri);
        mongoc_cleanup();
    }

    /**
     * @brief 将字符串追加写入文件（带时间戳）
     * @param _str 待写入的字符串内容
     *
     * 写入格式："{ymdhmsf} @{_str}\n"
     * 使用内存缓冲区减少磁盘 I/O：数据先写入 head 缓冲区，满时一次性刷入 fp。
     * 注意：调用前需确保 fp 已通过 fopen 打开，否则 fwrite 行为未定义。
     */
    void put(const string& _str) {
        if (!_str.empty()) {
            char tmp[512] = {};
            sprintf(tmp, "%s @%s\n", ymdhmsf().c_str(), _str.c_str());  // 格式化为 "YYYY-MM-DD HH:MM:SS.fff @内容\n"
            uint32_t strsize = strlen(tmp);
            uint32_t required_buffer_len = count + strsize;

            // 缓冲区满时，先刷入文件再写入新数据
            if (required_buffer_len > bufSize) {
                size_t r = fwrite(head, sizeof(char), count, fp);
                if (r == count) {
                    memcpy(head, tmp, strsize * sizeof(char));  // 缓冲区清空后写入新数据
                    count = strsize;
                    fflush(fp);
                    return;
                }
                // 若 fwrite 失败，r != count，当前实现未处理错误
            }
            memcpy(head + count, tmp, strsize * sizeof(char));
            count = required_buffer_len;
        }
    }

    /**
     * @brief 将序列化字符串解析后插入 MongoDB
     * @param _str 序列化字符串，格式：msgtype|fullSymbol|time|price|size|bidprice1|bidsize1|askprice1|asksize1|openinterest|...
     *
     * 仅处理 MSG_TYPE_TICK_L1 类型。fullSymbol 格式为 "交易所 类型 品种 合约号"（空格分隔），
     * 集合名取 fullSymbol 第 3 段（品种），数据库固定为 "findata"。
     * 时间字段会加 8 小时偏移（UTC+8 时区转换）。
     */
    void insertdb(const string& _str) {
        if (!_str.empty()) {
            vector<string> vs = stringsplit(_str, SERIALIZATION_SEPARATOR);  // 按 '|' 分割
            if ((MSG_TYPE)(atoi(vs[0].c_str())) == MSG_TYPE::MSG_TYPE_TICK_L1) {
                // vs[0]=msgtype, vs[1]=fullSymbol, vs[2]=time, vs[3]=price, vs[4]=size,
                // vs[5..8]=bidprice1,bidsize1,askprice1,asksize1, vs[9]=openinterest
                vector<string> fullsym = stringsplit(vs[1], ' ');  // fullSymbol: "交易所 类型 品种 合约号"
                string collectionname = fullsym[2];               // 集合名使用品种（如 IF、IC）
                mongoc_client_t     *client = mongoc_client_pool_pop(pool);
                mongoc_collection_t *collection = mongoc_client_get_collection(client, "findata", collectionname.c_str());

                bson_t *doc = bson_new();
                BSON_APPEND_UTF8(doc, "contractno", fullsym[3].c_str());   // 合约号
                BSON_APPEND_DATE_TIME(doc, "datetime", string2unixtimems(vs[2]) + 8*3600000);  // 时间 +8h 转为北京时间
                BSON_APPEND_DOUBLE(doc, "price", atof(vs[3].c_str()));    // 最新价
                BSON_APPEND_INT32(doc, "size", atoi(vs[4].c_str()));      // 最新成交量
                BSON_APPEND_DOUBLE(doc, "bidprice1", atof(vs[5].c_str())); // 买一价
                BSON_APPEND_INT32(doc, "bidsize1", atoi(vs[6].c_str()));   // 买一量
                BSON_APPEND_DOUBLE(doc, "askprice1", atof(vs[7].c_str())); // 卖一价
                BSON_APPEND_INT32(doc, "asksize1", atoi(vs[8].c_str()));   // 卖一量
                BSON_APPEND_INT32(doc, "openinterest", atoi(vs[9].c_str())); // 持仓量
                BSON_APPEND_INT32(doc, "dominant", 0);                    // 主力标志（预留）
                if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
                    fprintf(stderr, "Count failed: %s\n", error.message);
                }
                bson_destroy(doc);
                mongoc_collection_destroy(collection);
                mongoc_client_pool_push(pool, client);  // 归还连接到池
            } else if ((MSG_TYPE)(atoi(vs[0].c_str())) == MSG_TYPE::MSG_TYPE_TICK_L5) {
                // 5 档深度行情：当前未实现插入逻辑，字段映射见下方注释
                // Tick_L5 k;
                // k.fullsymbol_ = vs[1];
                // k.time_ = vs[2];
                // k.price_ = atof(vs[3].c_str());
                // k.size_ = atoi(vs[4].c_str());
                // k.depth_ = 5;
                // k.bidprice_L1_ = atoi(vs[5].c_str());
                // k.bidsize_L1_ = atoi(vs[6].c_str());
                // k.askprice_L1_ = atoi(vs[7].c_str());
                // k.asksize_L1_ = atoi(vs[8].c_str());
                // k.bidprice_L2_ = atoi(vs[9].c_str());
                // k.bidsize_L2_ = atoi(vs[10].c_str());
                // k.askprice_L2_ = atoi(vs[11].c_str());
                // k.asksize_L2_ = atoi(vs[12].c_str());
                // k.bidprice_L3_ = atoi(vs[13].c_str());
                // k.bidsize_L3_ = atoi(vs[14].c_str());
                // k.askprice_L3_ = atoi(vs[15].c_str());
                // k.asksize_L3_ = atoi(vs[16].c_str());
                // k.bidprice_L4_ = atoi(vs[17].c_str());
                // k.bidsize_L4_ = atoi(vs[18].c_str());
                // k.askprice_L4_ = atoi(vs[19].c_str());
                // k.asksize_L4_ = atoi(vs[20].c_str());
                // k.bidprice_L5_ = atoi(vs[21].c_str());
                // k.bidsize_L5_ = atoi(vs[22].c_str());
                // k.askprice_L5_ = atoi(vs[23].c_str());
                // k.asksize_L5_ = atoi(vs[24].c_str());
                // k.open_interest = atoi(vs[25].c_str());
                // k.open_ = atoi(vs[26].c_str());
                // k.high_ = atoi(vs[27].c_str());
                // k.low_ = atoi(vs[28].c_str());
                // k.pre_close_ = atoi(vs[29].c_str());
                // k.upper_limit_price_ = atoi(vs[30].c_str());
                // k.lower_limit_price_ = atoi(vs[31].c_str());
            }
        }
    }

    /**
     * @brief 将 Tick 对象直接插入 MongoDB
     * @param k Tick 行情对象（含 fullSymbol_、time_、price_、买卖档、持仓量等）
     *
     * 从 fullSymbol_ 解析出品种和合约号，集合名取品种，数据库为 "findata"。
     * 时间字段加 8 小时偏移转为北京时间（UTC+8）。
     */
    void insertdb(const Tick& k){
        vector<string> fullsym = stringsplit(k.fullSymbol_, ' ');  // "交易所 类型 品种 合约号"
        string collectionname = fullsym[2];
        mongoc_client_t     *client = mongoc_client_pool_pop(pool);
        mongoc_collection_t *collection = mongoc_client_get_collection(client, "findata", collectionname.c_str());

        bson_t *doc = bson_new();
        BSON_APPEND_UTF8(doc, "contractno", fullsym[3].c_str());
        BSON_APPEND_DATE_TIME(doc, "datetime", string2unixtimems(k.time_) + 8*3600000);
        BSON_APPEND_DOUBLE(doc, "price", k.price_);
        BSON_APPEND_INT32(doc, "size", k.size_);
        BSON_APPEND_DOUBLE(doc, "bidprice1", k.bidPrice_[0]);   // 买一价
        BSON_APPEND_INT32(doc, "bidsize1", k.bidSize_[0]);      // 买一量
        BSON_APPEND_DOUBLE(doc, "askprice1", k.askPrice_[0]);   // 卖一价
        BSON_APPEND_INT32(doc, "asksize1", k.askSize_[0]);      // 卖一量
        BSON_APPEND_INT32(doc, "openinterest", k.openInterest_);
        BSON_APPEND_INT32(doc, "dominant", 0);
        if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
            cout << "insert mongodb failed, errormsg = " << error.message;
        }
        bson_destroy(doc);
        mongoc_collection_destroy(collection);
        mongoc_client_pool_push(pool, client);
    }
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_DATA_TICKWRITER_H_
