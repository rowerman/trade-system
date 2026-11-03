#include <Strategy/strategybase.h>
#include <Common/config.h>
#include <Common/timeutil.h>
#include <Trade/ordermanager.h>

namespace StarQuant {

extern std::atomic<bool> gShutdown;
int StrategyBase::m_orderId = 0;

/** @brief 构造函数（当前为空实现，消息队列已移至 StrategyManager） */
StrategyBase::StrategyBase() {
		// if (CConfig::instance()._msgq == MSGQ::ZMQ) {
		// 	//msgq_pair_ = std::make_unique<CMsgqZmq>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT);
		// 	msgq_pair_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT);
		// 	msgq_sub_ = std::make_unique<CMsgqZmq>(MSGQ_PROTOCOL::SUB, CConfig::instance().MKT_DATA_PUBSUB_PORT, false);
		// }
		// else {
		// 	msgq_pair_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PAIR, CConfig::instance().BROKERAGE_PAIR_PORT);
		// 	msgq_sub_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, CConfig::instance().MKT_DATA_PUBSUB_PORT, false);
		// }

		//m_orderId = 0;
}

/** @brief 析构函数（当前为空实现） */
StrategyBase::~StrategyBase() {
}

/** @brief 初始化策略，msleep(2000) 后置 initialized=true */
void StrategyBase::initialize() {
		msleep(2000);
		initialized = true;
}

/** @brief 重置策略状态（当前为空实现） */
void StrategyBase::reset() {
}

/** @brief 清理资源，置 initialized=false */
void StrategyBase::teardown() {
		initialized = false;
}

/** @brief 序列化订单为 MSG_TYPE_ORDER 消息，push 到 msgstobrokerage，由 StrategyManager 转发 */
void StrategyBase::SendOrder(std::shared_ptr<Order> o)
	{
//TODO：根据订单类型发送相关信息，如限价单需要有价格信息
		lock_guard<mutex> g(oid_mtx);

		o->createTime = time(nullptr);
		o->orderStatus_ = OrderStatus::OS_NewBorn;
		o->clientOrderId = m_orderId;
		//o->serverOrderId= m_serverOrderId;
		// increase order id
		m_orderId++; 
		//OrderManager::instance().trackOrder(o);  

		string s;
		string s_price = "0.0";
		if (o->orderType == OrderType::OT_Limit){
			s_price = to_string(o->limitPrice);
		}else if (o->orderType == OrderType::OT_StopLimit){
			s_price =to_string(o->stopPrice);
		}
		s = to_string(MSG_TYPE::MSG_TYPE_ORDER) //CConfig::instance().new_order_msg
		+ SERIALIZATION_SEPARATOR + CConfig::instance().account
		+ SERIALIZATION_SEPARATOR + to_string(o->source)
		+ SERIALIZATION_SEPARATOR + to_string(o->clientOrderId)
		+ SERIALIZATION_SEPARATOR + to_string(static_cast<int>(o->orderType))
		+ SERIALIZATION_SEPARATOR + o->fullSymbol
		+ SERIALIZATION_SEPARATOR + to_string(o->orderSize)
		+ SERIALIZATION_SEPARATOR + s_price
		+ SERIALIZATION_SEPARATOR + to_string(static_cast<int>(o->orderFlag))
		+ SERIALIZATION_SEPARATOR + o->tag;
		msgstobrokerage.push(s);
//		msgq_pair->sendmsg(s)
}

/** @brief 发送撤单请求，序列化为 MSG_TYPE_CANCEL_ORDER 消息并 push 到 msgstobrokerage */
void StrategyBase::SendOrderCancel(long oid)
	{
		// TODO: put it into order process queue
		string msg = to_string(MSG_TYPE::MSG_TYPE_CANCEL_ORDER)// CConfig::instance().cancel_order_msg
			+ SERIALIZATION_SEPARATOR + std::to_string(oid);

		msgstobrokerage.push(msg);
}

/** @brief 发送行情订阅请求（当前空实现） */
void StrategyBase::SendSubscriptionRequest()
	{
}

/** @brief 发送历史 K 线请求（当前空实现） */
void StrategyBase::SendHistoricalBarRequest()
	{
}

/** @brief 发送通用信息（当前空实现） */
void StrategyBase::SendGeneralInformation()
	{
}

/** @brief 发送日志（当前空实现） */
void StrategyBase::SendLog()
	{
}

}  // namespace StarQuant