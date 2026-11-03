/*****************************************************************************
 * 数据服务实现：DataBoard、Bar 聚合、Tick 录制、Tick 回放
 *
 * 依赖：DataManager、TickWriter、TickReader、CConfig、msgq(SUB/PUB)
 *****************************************************************************/
#include <atomic>
#include <Services/dataservice.h>
#include <Common/datastruct.h>
#include <Common/config.h>
#include <Common/util.h>
#include <Common/logger.h>
#include <Common/msgq.h>
#include <Data/datamanager.h>
#include <Data/tickwriter.h>
#include <Data/tickreader.h>

using namespace std;

namespace StarQuant {

extern std::atomic<bool> gShutdown;
atomic<uint64_t> MICRO_SERVICE_NUMBER(0);

/** @brief 数据看板服务：订阅行情，按 MSG_TYPE 解析 Tick 并更新 DataManager
 *  当前主逻辑已注释，原设计为：msgq_sub_ 接收 -> 解析 MSG_TYPE_Trade/TICK_L1/TICK_L5 -> SetTickValue */
void DataBoardService() {
        try {
            // std::unique_ptr<CMsgq> msgq_sub_;
            // msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().SERVERPUB_URL, false);
            // while (!gShutdown) {
            // 	string msg = msgq_sub_->recmsg(0);
            // 	if (!msg.empty()) {

            // 		vector<string> vs = stringsplit(msg, SERIALIZATION_SEPARATOR);
            // 		//cout<<"Databoad rec msg size:"<<vs.size()<<endl;
            // 		if ((MSG_TYPE)(atoi(vs[0].c_str())) == MSG_TYPE::MSG_TYPE_Trade)	// Always Tick; actual contents are determined by MSG_TYPE
            // 		{
            // 			Tick k;
            // 			k.fullsymbol_ = vs[1];
            // 			k.time_ = vs[2];					//atoi(vs[1].c_str());
            // 			k.price_ = atof(vs[3].c_str());
            // 			k.size_ = atoi(vs[4].c_str());
            // 			k.depth_ = 1;

            // 			if (DataManager::instance()._latestmarkets.find(k.fullsymbol_) != DataManager::instance()._latestmarkets.end()) {
            // 				//cout<<"Databoard latest price:"<<k.price_<<" Datatype:"<<k.msgtype_<<endl;
            // 				DataManager::instance().SetTickValue(k);
            // 				// PRINT_TO_FILE("ERROR:[%s,%d][%s]%s.\n", __FILE__, __LINE__, __FUNCTION__, buf);
            // 			}
            // 			//cout<<"Databoard latest price:"<<k.fullsymbol_.length()<<DataManager::instance()._latestmarkets[k.fullsymbol_].price_<<endl;
            // 		}
            // 		else if ((MSG_TYPE)(atoi(vs[0].c_str())) == MSG_TYPE::MSG_TYPE_TICK_L1)		// Always Tick; actual contents are determined by MSG_TYPE
            // 		{
            // 			Tick_L1 k;
            // 			k.fullsymbol_ = vs[1];
            // 			k.time_ = vs[2];
            // 			k.price_ = atof(vs[3].c_str());
            // 			k.size_ = atoi(vs[4].c_str());
            // 			k.depth_ = 1;
            // 			k.bidprice_L1_ = atoi(vs[5].c_str());
            // 			k.bidsize_L1_ = atoi(vs[6].c_str());
            // 			k.askprice_L1_ = atoi(vs[7].c_str());
            // 			k.asksize_L1_ = atoi(vs[8].c_str());
            // 			k.open_interest = atoi(vs[9].c_str());
            // 			k.open_ = atoi(vs[10].c_str());
            // 			k.high_ = atoi(vs[11].c_str());
            // 			k.low_ = atoi(vs[12].c_str());
            // 			k.pre_close_ = atoi(vs[13].c_str());
            // 			k.upper_limit_price_ = atoi(vs[14].c_str());
            // 			k.lower_limit_price_ = atoi(vs[15].c_str());

            // 			if (DataManager::instance()._latestmarkets.find(k.fullsymbol_) != DataManager::instance()._latestmarkets.end()) {
            // 				DataManager::instance().SetTickValue(k);
            // 				// PRINT_TO_FILE("ERROR:[%s,%d][%s]%s.\n", __FILE__, __LINE__, __FUNCTION__, buf);
            // 			}
                        
            // 		}
            // 		else if ((MSG_TYPE)(atoi(vs[0].c_str())) == MSG_TYPE::MSG_TYPE_TICK_L5)		// Always Tick; actual contents are determined by MSG_TYPE
            // 		{
            // 			Tick_L5 k;
            // 			k.fullsymbol_ = vs[1];
            // 			k.time_ = vs[2];
            // 			k.price_ = atof(vs[3].c_str());
            // 			k.size_ = atoi(vs[4].c_str());
            // 			k.depth_ = 5;
            // 			k.bidprice_L1_ = atoi(vs[5].c_str());
            // 			k.bidsize_L1_ = atoi(vs[6].c_str());
            // 			k.askprice_L1_ = atoi(vs[7].c_str());
            // 			k.asksize_L1_ = atoi(vs[8].c_str());
            // 			k.bidprice_L2_ = atoi(vs[9].c_str());
            // 			k.bidsize_L2_ = atoi(vs[10].c_str());
            // 			k.askprice_L2_ = atoi(vs[11].c_str());
            // 			k.asksize_L2_ = atoi(vs[12].c_str());
            // 			k.bidprice_L3_ = atoi(vs[13].c_str());
            // 			k.bidsize_L3_ = atoi(vs[14].c_str());
            // 			k.askprice_L3_ = atoi(vs[15].c_str());
            // 			k.asksize_L3_ = atoi(vs[16].c_str());
            // 			k.bidprice_L4_ = atoi(vs[17].c_str());
            // 			k.bidsize_L4_ = atoi(vs[18].c_str());
            // 			k.askprice_L4_ = atoi(vs[19].c_str());
            // 			k.asksize_L4_ = atoi(vs[20].c_str());
            // 			k.bidprice_L5_ = atoi(vs[21].c_str());
            // 			k.bidsize_L5_ = atoi(vs[22].c_str());
            // 			k.askprice_L5_ = atoi(vs[23].c_str());
            // 			k.asksize_L5_ = atoi(vs[24].c_str());
            // 			k.open_interest = atoi(vs[25].c_str());
            // 			k.open_ = atoi(vs[26].c_str());
            // 			k.high_ = atoi(vs[27].c_str());
            // 			k.low_ = atoi(vs[28].c_str());
            // 			k.pre_close_ = atoi(vs[29].c_str());
            // 			k.upper_limit_price_ = atoi(vs[30].c_str());
            // 			k.lower_limit_price_ = atoi(vs[31].c_str());
            // 			if (DataManager::instance()._latestmarkets.find(k.fullsymbol_) != DataManager::instance()._latestmarkets.end()) {
            // 				DataManager::instance().SetTickValue(k);
            // 				// PRINT_TO_FILE("ERROR:[%s,%d][%s]%s.\n", __FILE__, __LINE__, __FUNCTION__, buf);
            // 			}
            // 		}
            // 	}
            // 	//TODO: adding trig event,sigal other thread to continue
            // }
        }
        catch (exception& e) {
            PRINT_TO_FILE_AND_CONSOLE("ERROR:[%s,%d][%s]%s.\n", __FILE__, __LINE__, __FUNCTION__, e.what());
        }
        catch (...) {
        }
        PRINT_TO_FILE("INFO:[%s,%d][%s]disconnect from market data feed.\n", __FILE__, __LINE__, __FUNCTION__);
}

/** @brief Bar 聚合服务：将 Tick 聚合成 K 线，当前为空实现 */
void BarAggregatorServcie() {
    // TODO: separate from DataBoardService
}

/** @brief Tick 录制服务：订阅行情，将每条消息追加写入当日文件
 *  文件路径：dataDir/marketdata-YYYYMMDD.txt */
void TickRecordingService() {
    std::unique_ptr<CMsgq> msgq_sub_;
    msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().SERVERPUB_URL, false);

    string ymdstr = ymd();  // 当日日期字符串 YYYYMMDD
    string fname = CConfig::instance().dataDir() + "/marketdata-" + ymdstr + ".txt";
    FILE* fp = fopen(fname.c_str(), "a+");  // 追加模式，不存在则创建
    TickWriter fwriter;
    fwriter.fp = fp;

    if (fp) {
        while (!gShutdown) {
            string msg = msgq_sub_->recmsg(0);  // 非阻塞接收
            if (!msg.empty())
                fwriter.put(msg);  // 写入文件
        }
    }
}

/** @brief Tick 回放服务：从文件读取历史行情，按 tickinterval 间隔发布到 PUB
 *  @param filetoreplay 回放文件路径（由 TickReader::readreplayfile 解析）
 *  @param tickinterval 每条消息间隔毫秒数，0 表示尽快发送 */
void TickReplayService(const std::string& filetoreplay, int32_t tickinterval) {
    std::unique_ptr<CMsgq> msgq_pub_;
    msgq_pub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PUB, CConfig::instance().SERVERPUB_URL);

    // 从文件读取所有行（每行一条序列化消息）
    vector<string> lines = readreplayfile(filetoreplay);
    int32_t i = 0, sz = lines.size();

    while (!gShutdown && i++ < sz) {
        string& msg = lines[i - 1];  // i 已自增，实际索引为 i-1
        msgq_pub_->sendmsg(msg);     // 发布到 PUB，订阅者（策略服务等）可接收
        msleep(tickinterval);         // 控制回放速度
    }
    msleep(2000);  // 回放结束后等待，确保最后一条消息被接收
    PRINT_TO_FILE("INFO:[%s,%d][%s]replay service stopped: %s\n", __FILE__, __LINE__, __FUNCTION__);
}

}  // namespace StarQuant
