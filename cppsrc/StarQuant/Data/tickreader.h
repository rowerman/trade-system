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

#ifndef CPPSRC_STARQUANT_DATA_TICKREADER_H_
#define CPPSRC_STARQUANT_DATA_TICKREADER_H_

#include <Common/util.h>
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

using std::mutex;

//////////////////////////////////////////////////////////////////////////
// TickReader - Tick 行情回放读取模块
//
// 功能概述：
//   从 TickWriter 生成的回放文件中读取历史 Tick 数据，供 TickReplayService 重放。
//
// 文件格式（与 TickWriter::put 输出一致）：
//   每行一条记录，格式为 "时间戳@消息内容"
//   - 时间戳：如 "2024-01-01 09:00:00.123 " 或 Unix 毫秒时间戳
//   - 消息内容：序列化后的 Tick 消息（如 "1001|SHFE F cu 2401|..."）
//
// 依赖：Common/util.h（stringsplit）
//////////////////////////////////////////////////////////////////////////
namespace StarQuant
{
/**
 * @brief 带时间戳的消息结构体（预留，当前未使用）
 * 用于存储回放文件中的时间戳与消息对，可支持按时间间隔重放。
 */
struct TimeAndMsg {
    uint64_t t;    // 时间戳（Unix 毫秒或微秒）
    string msg;    // 消息内容（序列化 Tick 字符串）
};

// 原始版本：返回 vector<TimeAndMsg>，包含时间戳与消息，可按时间控制回放节奏
// vector<TimeAndMsg> readreplayfile(const string& filetoreplay) {
// 	ifstream f(filetoreplay);
// 	vector<TimeAndMsg> lines;
// 	string x;
// 	while (f.is_open() && f.good()) {
// 		getline(f, x);
// 		if (!x.empty()) {
// 			vector<string> tmp = stringsplit(x, '@');
// 			if (tmp.size() == 2) {
// 				TimeAndMsg tam = { (uint64_t)atoll(tmp[0].c_str()), tmp[1] };
// 				lines.push_back(tam);
// 			}
// 		}
// 	}
// 	return lines;
// }

/**
 * @brief 从回放文件中读取所有 Tick 消息
 * @param filetoreplay 回放文件路径（如 marketdata-20240101.txt）
 * @return 消息内容字符串向量，按文件中的顺序排列
 *
 * 解析逻辑：
 *   1. 每行格式为 "时间戳@消息内容"，以 '@' 分割
 *   2. 提取 tmp[1] 作为消息内容
 *   3. tam.erase(0, 1) 移除首字符（若格式为 "时间戳@ 内容" 则去除前导空格）
 *   4. 仅当分割结果恰好为 2 段时纳入结果，跳过格式异常行
 *
 * 调用方：DataService::TickReplayService 将返回的 lines 逐条发送到 PUB 消息队列
 */
vector<string> readreplayfile(const string& filetoreplay) {
    ifstream f(filetoreplay);
    vector<string> lines;
    string x;
    while (f.is_open() && f.good()) {
        getline(f, x);
        if (!x.empty()) {
            vector<string> tmp = stringsplit(x, '@');  // 格式：时间戳@消息内容
            if (tmp.size() == 2) {
                string tam = tmp[1];                  // 取消息部分
                tam.erase(0, 1);                      // 移除首字符（前导空格等）
                lines.push_back(tam);
            }
        }
    }
    return lines;
}

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_DATA_TICKREADER_H_
