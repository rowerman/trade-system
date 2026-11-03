/*****************************************************************************
 * 策略管理服务实现：连接行情、策略、交易引擎
 *
 * 依赖：SmaCross、strategyFactory、DataManager、CConfig、nanomsg
 *****************************************************************************/
#include <Common/config.h>
#include <Common/util.h>
#include <Common/timeutil.h>
#include <Common/logger.h>
#include <Trade/ordermanager.h>
#include <Data/datamanager.h>
#include <Services/strategyservice.h>
#include <Strategy/strategyFactory.h>
#include <Strategy/smacross.h>
#include <atomic>
#ifdef _WIN32
#include <nanomsg/src/nn.h>
#include <nanomsg/src/pair.h>
#else
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#endif

using namespace std;

namespace StarQuant {

extern std::atomic<bool> gShutdown;

/** @brief 策略管理服务主循环：订阅行情 -> OnTick -> 转发订单 -> 接收回报 */
void StrategyManagerService() {
    std::unique_ptr<CMsgq> msgq_sub_;
    std::unique_ptr<CMsgq> msgq_pair_;

    // 消息队列：SUB 订阅行情，PAIR 与交易引擎双向通信
    // if (CConfig::instance()._msgq == MSGQ::ZMQ) {
        // 	//msgq_sub_ = std::make_unique<CMsgqZmq>(MSGQ_PROTOCOL::SUB, CConfig::instance().MKT_DATA_PUBSUB_PORT);
        // 	//msgq_pair_ = std::make_unique<CMsgqZmq>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT);
        // 	msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().MKT_DATA_PUBSUB_PORT);
        // 	msgq_pair_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT);
        // }
        // else {
        // 	msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().MKT_DATA_PUBSUB_PORT,false);
        // 	msgq_pair_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT,false);
        // }
    msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().SERVERPUB_URL, false);
    msgq_pair_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT, false);

    // 加载策略（可改为 strategyFactory(CConfig::instance().strategy_name) 实现可配置）
    auto strategy = make_unique<SmaCross>();
    strategy->initialize();

    string msg;
    while (!gShutdown) {
        // 1. 接收行情：非阻塞，0 表示立即返回
        msg = msgq_sub_->recmsg(0);
        if (!msg.empty()) {
            vector<string> vs = stringsplit(msg, SERIALIZATION_SEPARATOR);
            // 6 字段：基础 Tick；33 字段：扩展 Tick（含买卖档等）
            if ((vs.size() == 6) || (vs.size() == 33)) {
                Tick k;
                k.fullsymbol_ = vs[0];
                k.time_ = vs[1];
                k.msgtype_ = (MSG_TYPE)(atoi(vs[2].c_str()));
                k.price_ = atof(vs[3].c_str());
                k.size_ = atoi(vs[4].c_str());
                k.depth_ = atoi(vs[5].c_str());

                strategy->OnTick(k);  // 触发策略逻辑，可能 push 订单到 msgstobrokerage
            }
        }

        // 2. 转发订单：将策略待发订单发送给交易引擎
        while (!strategy->msgstobrokerage.empty()) {
            string smsg = strategy->msgstobrokerage.front();
            cout << "strategy sendout msg:" << smsg << endl;
            msgq_pair_->sendmsg(smsg);
            strategy->msgstobrokerage.pop();
        }

        // 3. 接收经纪商回报：订单确认、成交、持仓等
        msg = msgq_pair_->recmsg(1);  // 超时 1ms，避免阻塞
        if (!msg.empty()) {
            strategy->OnGeneralMessage(msg);
        }
    }
    PRINT_TO_FILE("INFO:[%s,%d][%s]Exit strategy thread\n", __FILE__, __LINE__, __FUNCTION__);
}

}  // namespace StarQuant
