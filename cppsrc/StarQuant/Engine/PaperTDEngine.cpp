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

#include <Common/datastruct.h>
#include <Trade/ordermanager.h>
#include <Trade/portfoliomanager.h>
#include <Data/datamanager.h>
#include <Common/logger.h>
#include <Common/util.h>
#include <Engine/PaperTDEngine.h>

#include <mutex>
#include <memory>
#include <string>
#include <boost/locale.hpp>
#include <boost/algorithm/algorithm.hpp>


using namespace std;
namespace StarQuant {

/**
 * @brief 构造函数：初始化 m_brokerOrderId_、fillID_，调用 init()
 */
PaperTDEngine::PaperTDEngine() {
    m_brokerOrderId_ = 0;
    fillID_ = 0;
    init();
}

/**
 * @brief 析构函数：若未停止则调用 stop()
 */
PaperTDEngine::~PaperTDEngine() {
    if (estate_ != STOP)
        stop();
}

/**
 * @brief 初始化：设置 name_="PAPER.TD"，创建 CMsgqEMessenger，置 estate_=DISCONNECTED
 */
void PaperTDEngine::init() {
    name_ = "PAPER.TD";
    if (logger == nullptr) {
        logger = SQLogger::getLogger("TDEngine.Paper");
    }
    if (messenger_ == nullptr) {
        messenger_ = std::make_unique<CMsgqEMessenger>(name_, CConfig::instance().SERVERSUB_URL);
        msleep(100);
    }
    estate_ = DISCONNECTED;
    auto pmsgs = make_shared<InfoMsg>(DESTINATION_ALL, name_,
                    MSG_TYPE_ENGINE_STATUS,
                    to_string(estate_));
    messenger_->send(pmsgs);
    LOG_DEBUG(logger, "Paper TD inited");
}

/**
 * @brief 停止引擎：置 estate_=STOP，发送状态通知
 */
void PaperTDEngine::stop() {
    estate_  = EState::STOP;
    auto pmsgs = make_shared<InfoMsg>(DESTINATION_ALL, name_,
                    MSG_TYPE_ENGINE_STATUS,
                    to_string(estate_));
    messenger_->send(pmsgs);
    LOG_DEBUG(logger, "Paper TD stoped");
}

/**
 * @brief 主循环：接收消息，根据 msgtype_ 分发处理 ENGINE_CONNECT、ORDER、CANCEL_ORDER、QRY_POS、QRY_ACCOUNT 等
 */
void PaperTDEngine::start() {
    while (estate_ != EState::STOP) {
        auto pmsgin = messenger_->recv();
        bool processmsg = ((pmsgin != nullptr)
        && ( startwith(pmsgin->destination_,DESTINATION_ALL) || (startwith(pmsgin->destination_,name_))));
        // if (pmsgin == nullptr || !startwith(pmsgin->destination_, name_) )
        // 	continue;
        if (processmsg) {
            switch (pmsgin->msgtype_) {
                case MSG_TYPE_ENGINE_CONNECT:
                    if (connect()) {
                        auto pmsgout = make_shared<InfoMsg>(pmsgin->source_, name_,
                            MSG_TYPE_INFO_ENGINE_TDCONNECTED, "PAPER TD Connected.");
                        messenger_->send(pmsgout, 1);
                    }
                    break;
                case MSG_TYPE_ENGINE_DISCONNECT:
                    disconnect();
                    break;
                case MSG_TYPE_ORDER_PAPER:
                case MSG_TYPE_ORDER:
                    if (estate_ == LOGIN_ACK) {
                        auto pmsgin2 = static_pointer_cast<PaperOrderMsg>(pmsgin);
                        insertOrder(pmsgin2);
                    } else {
                        LOG_DEBUG(logger,"PAPER_TD is not connected,can not insert order!");
                        auto pmsgout = make_shared<ErrorMsg>(pmsgin->source_, name_,
                            MSG_TYPE_ERROR_ENGINENOTCONNECTED,
                            "Paper Td is not connected,can not insert order!");
                        messenger_->send(pmsgout);
                    }
                    break;
                case MSG_TYPE_CANCEL_ORDER:
                    if (estate_ == LOGIN_ACK) {
                        auto pmsgin2 = static_pointer_cast<OrderActionMsg>(pmsgin);
                        cancelOrder(pmsgin2);
                    } else {
                        LOG_DEBUG(logger,"PAPER_TD is not connected,can not cancel order!");
                        auto pmsgout = make_shared<ErrorMsg>(pmsgin->source_, name_,
                            MSG_TYPE_ERROR_ENGINENOTCONNECTED,
                            "Paper Td is not connected,can not cancel order!");
                        messenger_->send(pmsgout);
                    }
                    break;
                case MSG_TYPE_QRY_POS:
                    if (estate_ == LOGIN_ACK) {
                        queryPosition(pmsgin);
                    } else {
                        LOG_DEBUG(logger, "PAPER_TD is not connected,can not qry pos!");
                        auto pmsgout = make_shared<ErrorMsg>(pmsgin->source_, name_,
                            MSG_TYPE_ERROR_ENGINENOTCONNECTED,
                            "PAPER TD  is not connected,can not qry pos!");
                        messenger_->send(pmsgout);
                    }
                    break;
                case MSG_TYPE_QRY_ACCOUNT:
                    if (estate_ == LOGIN_ACK){
                        queryAccount(pmsgin);
                    } else {
                        LOG_DEBUG(logger, "PAPER_TD is not connected,can not qry acc!");
                        auto pmsgout = make_shared<ErrorMsg>(pmsgin->source_, name_,
                            MSG_TYPE_ERROR_ENGINENOTCONNECTED,
                            "paper Td is not connected,can not qry acc!");
                        messenger_->send(pmsgout);
                    }
                    break;
                case MSG_TYPE_ENGINE_STATUS:
                    {
                        auto pmsgout = make_shared<InfoMsg>(pmsgin->source_, name_,
                            MSG_TYPE_ENGINE_STATUS,
                            to_string(estate_));
                        messenger_->send(pmsgout);
                    }
                    break;
                case MSG_TYPE_TEST:
                    {
                        auto pmsgout = make_shared<InfoMsg>(pmsgin->source_, name_,
                            MSG_TYPE_TEST,
                            "test");
                        messenger_->send(pmsgout);
                        LOG_DEBUG(logger,"PAPER_TD return test msg!");
                    }
                    break;
                case MSG_TYPE_TIMER:
                    timertask();
                    break;
                default:
                    processbuf();
                    break;
            }
        } else {
            processbuf();
        }
    }
}

/**
 * @brief 处理缓冲区（预留，空实现）
 */
void PaperTDEngine::processbuf() {
}

/**
 * @brief 定时任务（预留，空实现）
 */
void PaperTDEngine::timertask() {
}

/**
 * @brief 模拟连接：等待 1 秒后直接置 estate_=LOGIN_ACK，无真实网络连接
 */
bool PaperTDEngine::connect() {
    msleep(1000);
    estate_ = LOGIN_ACK;
    auto pmsg = make_shared<InfoMsg>(DESTINATION_ALL, name_,
                        MSG_TYPE_ENGINE_STATUS,
                        to_string(estate_));
    messenger_->send(pmsg);
    return true;
}

/**
 * @brief 模拟断开：等待 1 秒后置 estate_=DISCONNECTED
 */
bool PaperTDEngine::disconnect() {
    msleep(1000);
    estate_ = DISCONNECTED;
    auto pmsg = make_shared<InfoMsg>(DESTINATION_ALL, name_,
                    MSG_TYPE_ENGINE_STATUS,
                    to_string(estate_));
    messenger_->send(pmsg);
    return true;
}

/**
 * @brief 模拟下单并立即撮合成交
 * 根据 orderBook_ 中的买卖一档、最新价模拟成交：
 *   - 市价单：买按 ask1、卖按 bid1 成交
 *   - 限价单：买限价>=ask1 或 卖限价<=bid1 时按可成交价成交，否则 OS_Error
 *   - OT_StopLimit：暂不支持，返回 OS_Error
 * 若无行情则 OS_Error。成交后 gotFill、发送 FillMsg、OrderStatusMsg
 */
void PaperTDEngine::insertOrder(shared_ptr<PaperOrderMsg> pmsg) {
    lock_guard<mutex> g(oid_mtx);
    pmsg->data_.serverOrderID_ = m_serverOrderId++;
    pmsg->data_.brokerOrderID_ = m_brokerOrderId_++;
    pmsg->data_.orderNo_ = string("PAPER.TD-") + to_string(pmsg->data_.brokerOrderID_);
    pmsg->data_.localNo_ = pmsg->data_.orderNo_;
    pmsg->data_.createTime_ = ymdhmsf();
    pmsg->data_.orderStatus_ = OrderStatus::OS_Submitted;
    pmsg->data_.price_ = pmsg->data_.limitPrice_;
    pmsg->data_.quantity_ = pmsg->data_.orderSize_;
    pmsg->data_.flag_ = pmsg->data_.orderFlag_;

    auto o = static_pointer_cast<PaperOrder>(pmsg->toPOrder());
    OrderManager::instance().trackOrder(o);
    // begin simulate trade, now only support L1
    if (DataManager::instance().orderBook_.find(o->fullSymbol_) != DataManager::instance().orderBook_.end()) {
        double lastprice = DataManager::instance().orderBook_[o->fullSymbol_].price_;
        double lastaskprice1 = DataManager::instance().orderBook_[o->fullSymbol_].askPrice_[0];
        double lastbidprice1 = DataManager::instance().orderBook_[o->fullSymbol_].bidPrice_[0];
        int64_t lastasksize1 = DataManager::instance().orderBook_[o->fullSymbol_].askSize_[0];
        int64_t lastbidsize1 = DataManager::instance().orderBook_[o->fullSymbol_].bidSize_[0];
        auto pmsgfill = make_shared<FillMsg>();
        pmsgfill->destination_ = pmsg->source_;
        pmsgfill->source_ = name_;
        pmsgfill->data_.fullSymbol_ = o->fullSymbol_;
        pmsgfill->data_.tradeTime_ = ymdhmsf();
        pmsgfill->data_.orderNo_ = o->orderNo_;
        pmsgfill->data_.serverOrderID_ = o->serverOrderID_;
        pmsgfill->data_.clientOrderID_ = o->clientOrderID_;
        pmsgfill->data_.brokerOrderID_ = o->brokerOrderID_;
        pmsgfill->data_.tradeId_ = fillID_++;
        pmsgfill->data_.tradeNo_ = to_string(pmsgfill->data_.tradeId_);
        pmsgfill->data_.account_ = o->account_;
        pmsgfill->data_.api_ = o->api_;

        if (o->orderType_ == OrderType::OT_Market) {
            pmsgfill->data_.fillFlag_ = o->orderFlag_;
            if (o->orderSize_ > 0) {
                pmsgfill->data_.tradePrice_ = lastaskprice1;
                pmsgfill->data_.tradeSize_ = o->orderSize_ < lastasksize1 ? o->orderSize_ : lastasksize1;
            } else {
                pmsgfill->data_.tradePrice_ = lastbidprice1;
                pmsgfill->data_.tradeSize_ = (-1)*o->orderSize_ < lastasksize1 ? o->orderSize_ : lastasksize1*(-1);
            }
        } else if (o->orderType_ == OrderType::OT_Limit) {
            if (o->orderSize_ > 0) {
                if (o->limitPrice_ >= lastaskprice1) {
                    if (lastprice < lastaskprice1) {
                        pmsgfill->data_.tradePrice_ = lastaskprice1;
                    } else if (lastprice > o->limitPrice_) {
                        pmsgfill->data_.tradePrice_ = o->limitPrice_;
                    } else {
                        pmsgfill->data_.tradePrice_ = lastprice;
                    }
                    pmsgfill->data_.tradeSize_ = o->orderSize_ < lastasksize1 ? o->orderSize_ : lastasksize1;
                    pmsgfill->data_.fillFlag_ = o->orderFlag_;
                } else {
                    lock_guard<mutex> gs(orderStatus_mtx);
                    o->orderStatus_ = OrderStatus::OS_Error;
                    auto pos = make_shared<OrderStatusMsg>(pmsg->source_, name_);
                    pos->set(o);
                    // auto pmsgout = make_shared<ErrorMsg>(pmsg->source_, name_,
                    // 	MSG_TYPE_ERROR_INSERTORDER,
                    // 	to_string(o->clientOrderID_));
                    messenger_->send(pos);
                    LOG_INFO(logger, "Paper TD cannot deal due to price is below ask price");
                    return;
                }
            } else {
                if (o->limitPrice_ <= lastbidprice1) {
                    if (lastprice > lastbidprice1) {
                        pmsgfill->data_.tradePrice_ = lastbidprice1;
                    } else if (lastprice < o->limitPrice_) {
                        pmsgfill->data_.tradePrice_ = o->limitPrice_;
                    } else {
                        pmsgfill->data_.tradePrice_ = lastprice;
                    }
                    pmsgfill->data_.tradeSize_ = (-1)*o->orderSize_ < lastbidsize1 ? o->orderSize_ : (-1)*lastbidsize1;
                    pmsgfill->data_.fillFlag_ = o->orderFlag_;
                } else {
                    lock_guard<mutex> gs(orderStatus_mtx);
                    o->orderStatus_ = OrderStatus::OS_Error;
                    auto pos = make_shared<OrderStatusMsg>(pmsg->source_, name_);
                    pos->set(o);
                    // auto pmsgout = make_shared<ErrorMsg>(pmsg->source_, name_,
                    // 	MSG_TYPE_ERROR_INSERTORDER,
                    // 	to_string(o->clientOrderID_));
                    messenger_->send(pos);
                    LOG_INFO(logger, "Paper TD cannot deal due to price is above bid price");
                    return;
                }
            }
        } else if (o->orderType_ == OrderType::OT_StopLimit) {
            lock_guard<mutex> gs(orderStatus_mtx);
            o->orderStatus_ = OrderStatus::OS_Error;
            auto pos = make_shared<OrderStatusMsg>(pmsg->source_,name_);
            pos->set(o);
            // auto pmsgout = make_shared<ErrorMsg>(pmsg->source_, name_,
            // 	MSG_TYPE_ERROR_INSERTORDER,
            // 	to_string(o->clientOrderID_));
            messenger_->send(pos);
            LOG_ERROR(logger,"Paper TD donot support stop order yet");
            return;
        }
        OrderManager::instance().gotFill(pmsgfill->data_);
        lock_guard<mutex> gs(orderStatus_mtx);
        o->orderStatus_ = OrderStatus::OS_Filled;
        o->tradedvol_ = pmsgfill->data_.tradeSize_;
        auto pos = make_shared<OrderStatusMsg>(pmsg->source_,name_);
        pos->set(o);
        messenger_->send(pos);
        messenger_->send(pmsgfill);
        LOG_INFO(logger,name_<<" Fill Order: clientorderid ="<<o->clientOrderID_<<"fullsymbol = "<<o->fullSymbol_);
    } else {
        lock_guard<mutex> gs(orderStatus_mtx);
        o->orderStatus_ = OrderStatus::OS_Error;
        auto pos = make_shared<OrderStatusMsg>(pmsg->source_,name_);
        pos->set(o);
        // auto pmsgout = make_shared<ErrorMsg>(pmsg->source_, name_,
        // 	MSG_TYPE_ERROR_INSERTORDER,
        // 	to_string(o->clientOrderID_));
        messenger_->send(pos);
        LOG_ERROR(logger,"Paper TD order insert error: due to DM dont have markets info");
        return;
    }
}

/**
 * @brief 撤单（当前未实现）：仅记录日志
 */
void PaperTDEngine::cancelOrder(shared_ptr<OrderActionMsg> pmsg) {
    LOG_INFO(logger, name_<< " don't support cancelorder yet!");
}

/**
 * @brief 查询账户（当前空实现）
 */
void PaperTDEngine::queryAccount(shared_ptr<MsgHeader> pmsg) {
}

/**
 * @brief 查询持仓（当前空实现）
 */
void PaperTDEngine::queryPosition(shared_ptr<MsgHeader> pmsg) {
}

}  // namespace StarQuant
