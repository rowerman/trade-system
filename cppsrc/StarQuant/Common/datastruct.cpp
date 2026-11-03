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
#include <Common/datastruct.h>
#include <string>

using namespace std;

namespace StarQuant {

// 服务端全局订单 ID 计数器（每个经纪商独立递增）
int64_t m_serverOrderId = 0;
// 订单 ID 递增时的互斥锁
std::mutex oid_mtx;
// 修改订单状态时的互斥锁
std::mutex orderStatus_mtx;

/**
 * 从序列化消息中解析出消息类型
 * @param msg 格式为 "dest|src|msgtype|..." 的字符串
 * @return 解析得到的 MSG_TYPE 枚举值
 * @note 前两个 getline 跳过 dest 和 src，第三个 getline 取 msgtype 并转为整数
 */
MSG_TYPE MsgType(const string& msg) {
    string header;
    stringstream ss(msg);
    getline(ss, header, SERIALIZATION_SEPARATOR);
    getline(ss, header, SERIALIZATION_SEPARATOR);
    getline(ss, header, SERIALIZATION_SEPARATOR);
    MSG_TYPE msgtype_ = MSG_TYPE(atoi(header.c_str()));
    return msgtype_;
}

/**
 * 将消息帧序列化为字符串
 * 格式：destination|source|msgtype|[dataPtr->serialize()]
 * @return 以 SERIALIZATION_SEPARATOR 分隔的完整消息字符串
 */
string MsgFrame::serialize() {
    string tmp = destination_
    + SERIALIZATION_SEPARATOR + source_
    + SERIALIZATION_SEPARATOR + to_string(msgtype_);
    if (dataPtr != nullptr) {
        tmp = tmp + SERIALIZATION_SEPARATOR + dataPtr->serialize();
    }
    return tmp;
}

/**
 * 从序列化字符串反序列化消息帧
 * @param msgin 格式为 "dest|src|msgtype|datas" 的字符串
 * 解析出 destination_、source_、msgtype_，若 datas 非空则调用 dataPtr->deserialize(datas)
 */
void MsgFrame::deserialize(const string& msgin) {
    string des;
    string src;
    string stype;
    string datas;
    stringstream ss(msgin);
    getline(ss, des, SERIALIZATION_SEPARATOR);
    getline(ss, src, SERIALIZATION_SEPARATOR);
    getline(ss, stype, SERIALIZATION_SEPARATOR);
    getline(ss, datas);
    MSG_TYPE mtype = MSG_TYPE(stoi(stype));
    destination_ = des;
    source_ = src;
    msgtype_ = mtype;
    if (!datas.empty())
        dataPtr->deserialize(datas);
}

/**
 * 从消息地址字符串中提取账户部分
 * @param msg 格式为 "xxx|xxx|account" 的地址字符串（按 DESTINATION_SEPARATOR 分隔）
 * @return 第三个字段，即账户标识
 */
string accAddress(const string& msg) {
    string acc;
    stringstream ss(msg);
    getline(ss, acc, DESTINATION_SEPARATOR);
    getline(ss, acc, DESTINATION_SEPARATOR);
    getline(ss, acc, DESTINATION_SEPARATOR);
    return acc;
}

/**
 * 将 Tick 行情消息序列化为字符串
 * 包含：destination、source、msgtype、fullSymbol、time、price、size、
 * 各档 bid/ask 价量、openInterest、OHLC、涨跌停价
 * @return 以 | 分隔的完整行情字符串
 */
string TickMsg::serialize() {
    string ba;  // bid ask price and size
    for (int32_t i = 0; i < data_.depth_; i++) {
        ba = ba
            + SERIALIZATION_SEPARATOR + to_string(data_.bidPrice_[i])
            + SERIALIZATION_SEPARATOR + to_string(data_.bidSize_[i])
            + SERIALIZATION_SEPARATOR + to_string(data_.askPrice_[i])
            + SERIALIZATION_SEPARATOR + to_string(data_.askSize_[i]);
    }
    string s;
    s = destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_.fullSymbol_
        + SERIALIZATION_SEPARATOR + data_.time_
        + SERIALIZATION_SEPARATOR + to_string(data_.price_)
        + SERIALIZATION_SEPARATOR + to_string(data_.size_)
        + ba
        + SERIALIZATION_SEPARATOR + to_string(data_.openInterest_)
        + SERIALIZATION_SEPARATOR + to_string(data_.open_)
        + SERIALIZATION_SEPARATOR + to_string(data_.high_)
        + SERIALIZATION_SEPARATOR + to_string(data_.low_)
        + SERIALIZATION_SEPARATOR + to_string(data_.preClose_)
        + SERIALIZATION_SEPARATOR + to_string(data_.upperLimitPrice_)
        + SERIALIZATION_SEPARATOR + to_string(data_.lowerLimitPrice_);
    return s;
}

/**
 * 将合约/证券消息序列化为字符串
 * 包含：destination、source、msgtype、symbol、exchange、localName、securityType、
 * multiplier、ticksize、postype、保证金比例、期权相关字段等
 * @return 以 | 分隔的合约信息字符串
 */
string SecurityMsg::serialize() {
    string s;
    s = destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_.symbol_
        + SERIALIZATION_SEPARATOR + data_.exchange_
        + SERIALIZATION_SEPARATOR + data_.localName_
        + SERIALIZATION_SEPARATOR + data_.securityType_
        + SERIALIZATION_SEPARATOR + to_string(data_.multiplier_)
        + SERIALIZATION_SEPARATOR + to_string(data_.ticksize_)
        + SERIALIZATION_SEPARATOR + data_.postype_
        + SERIALIZATION_SEPARATOR + to_string(data_.longMarginRatio_)
        + SERIALIZATION_SEPARATOR + to_string(data_.shortMarginRatio_)
        + SERIALIZATION_SEPARATOR + data_.underlyingSymbol_
        + SERIALIZATION_SEPARATOR + data_.optionType_
        + SERIALIZATION_SEPARATOR + to_string(data_.strikePrice_)
        + SERIALIZATION_SEPARATOR + data_.expiryDate_;
    return s;
}

/**
 * 将账户信息消息序列化为字符串
 * 包含：destination、source、msgtype、accountID、昨日权益、净资产、可用资金、
 * 手续费、维持保证金、已实现/未实现盈亏、余额、冻结资金、当前时间
 * @return 以 | 分隔的账户信息字符串
 */
string AccMsg::serialize() {
    string s;
    s = destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_.accountID_
        + SERIALIZATION_SEPARATOR + to_string(data_.previousDayEquityWithLoanValue_)
        + SERIALIZATION_SEPARATOR + to_string(data_.netLiquidation_)
        + SERIALIZATION_SEPARATOR + to_string(data_.availableFunds_)
        + SERIALIZATION_SEPARATOR + to_string(data_.commission_)
        + SERIALIZATION_SEPARATOR + to_string(data_.fullMaintainanceMargin_)
        + SERIALIZATION_SEPARATOR + to_string(data_.realizedPnL_)
        + SERIALIZATION_SEPARATOR + to_string(data_.unrealizedPnL_)
        + SERIALIZATION_SEPARATOR + to_string(data_.balance_)
        + SERIALIZATION_SEPARATOR + to_string(data_.frozen_)
        + SERIALIZATION_SEPARATOR + ymdhmsf();
    return s;
}

/**
 * 将成交回报消息序列化为字符串
 * 包含：destination、source、msgtype、serverOrderID、clientOrderID、clientID、
 * localNo、orderNo、tradeNo、tradeTime、fullSymbol、tradePrice、tradeSize、
 * fillFlag、commission、account、api、当前时间
 * @return 以 | 分隔的成交信息字符串
 */
string FillMsg::serialize() {
    string str =  destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.serverOrderID_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.clientOrderID_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.clientID_)
        + SERIALIZATION_SEPARATOR + data_.localNo_
        + SERIALIZATION_SEPARATOR + data_.orderNo_
        + SERIALIZATION_SEPARATOR + data_.tradeNo_
        + SERIALIZATION_SEPARATOR + data_.tradeTime_
        + SERIALIZATION_SEPARATOR + data_.fullSymbol_
        + SERIALIZATION_SEPARATOR + std::to_string(data_.tradePrice_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.tradeSize_)
        + SERIALIZATION_SEPARATOR + std::to_string(static_cast<int32_t>(data_.fillFlag_))
        + SERIALIZATION_SEPARATOR + std::to_string(data_.commission_)
        + SERIALIZATION_SEPARATOR + data_.account_
        + SERIALIZATION_SEPARATOR + data_.api_
        + SERIALIZATION_SEPARATOR + ymdhms();
    return str;
}

/**
 * 判断订单是否处于活跃状态（可撤单）
 * 已成交、已取消、错误的订单视为非活跃
 * @param o 待检查的订单
 * @return true：活跃（可撤）；false：已结束
 */
bool isActiveOrder(const Order& o) {
    if (o.orderStatus_ == OrderStatus::OS_Filled)
        return false;
    if (o.orderStatus_ == OrderStatus::OS_Error)
        return false;
    if (o.orderStatus_ == OrderStatus::OS_Canceled)
        return false;
    return true;
}

/**
 * 判断订单状态是否为活跃状态
 * @param os 订单状态枚举
 * @return true：活跃；false：已成交/已取消/错误
 */
bool isActiveOS(const OrderStatus& os) {
    if (os == OrderStatus::OS_Filled)
        return false;
    if (os == OrderStatus::OS_Error)
        return false;
    if (os == OrderStatus::OS_Canceled)
        return false;
    return true;
}

/**
 * 从序列化字符串反序列化通用订单消息
 * 格式：dest|src|msgtype|api|account|clientID|clientOrderID|tag|...
 * @param msgin 以 | 分隔的订单字符串
 * @throw std::out_of_range 若字段数少于 8
 */
void OrderMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 8)
        throw std::out_of_range("OutofRange in Order deserialize");
    destination_ = v[0];
    source_ = v[1];
    data_.api_ = v[3];
    data_.account_ = v[4];
    data_.clientID_ = stoi(v[5]);
    data_.clientOrderID_ = stol(v[6]);
    data_.tag_ = v[7];
    // data_.clientID_ = stoi(v[1]);
    // data_.clientOrderID_ = stol(v[4]);
    // data_.orderType_ = static_cast<OrderType>(stoi(v[5]));
    // data_.fullSymbol_ = v[6];
    // data_.orderSize_ = stoi(v[7]);
    // if (data_.orderType_ == OrderType::OT_Limit){
    //     data_.limitPrice_ = stof(v[8]);
    // }else if (data_.orderType_ == OrderType::OT_StopLimit){
    //     data_.stopPrice_ = stof(v[8]);
    // }
    // data_.orderFlag_ = static_cast<OrderFlag>(stoi(v[9]));
    // data_.tag_ = v[10];
}

/**
 * 将 OrderMsg 的 data_ 转为 shared_ptr<Order>
 * 供 OrderManager::trackOrder 等使用，用于统一管理订单
 * @return 包含订单数据的 shared_ptr，可直接加入订单管理器
 */
std::shared_ptr<Order> OrderMsg::toPOrder() {
    std::shared_ptr<Order> o = make_shared<Order>();
    o->api_ = data_.api_;
    o->account_ = data_.account_;
    o->clientID_ = data_.clientID_;
    o->clientOrderID_ = data_.clientOrderID_;
    o->tag_ =  data_.tag_;
    o->orderType_ = data_.orderType_;

    o->fullSymbol_ = data_.fullSymbol_;
    o->price_ = data_.price_;
    o->quantity_ = data_.quantity_;
    o->tradedvol_ = data_.tradedvol_;
    o->flag_ = data_.flag_;
    o->serverOrderID_ = data_.serverOrderID_;
    o->brokerOrderID_ = data_.brokerOrderID_;
    o->orderNo_ = data_.orderNo_;
    o->localNo_ = data_.localNo_;
    o->createTime_ = data_.createTime_;
    o->updateTime_ = data_.updateTime_;
    o->orderStatus_ = data_.orderStatus_;

    return o;
}

/**
 * 从序列化字符串反序列化模拟盘订单消息
 * 格式：dest|src|msgtype|api|account|clientID|clientOrderID|tag|orderType|fullSymbol|orderFlag|orderSize|limitPrice|stopPrice
 * @param msgin 以 | 分隔的模拟盘订单字符串
 * @throw std::out_of_range 若字段数少于 14
 */
void PaperOrderMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 14)
        throw std::out_of_range("OutofRange in PaperOrder deserialize.");
    destination_ = v[0];
    source_ = v[1];
    data_.api_ = v[3];
    data_.account_ = v[4];
    data_.clientID_ = stoi(v[5]);
    data_.clientOrderID_ = stol(v[6]);
    data_.tag_ = v[7];

    data_.orderType_ = static_cast<OrderType>(stoi(v[8]));
    data_.fullSymbol_ = v[9];
    data_.orderFlag_ = static_cast<OrderFlag>(stoi(v[10]));
    data_.orderSize_ = stoi(v[11]);
    data_.limitPrice_ = stof(v[12]);
    data_.stopPrice_ = stof(v[13]);
}

/**
 * 将 PaperOrderMsg 的 data_ 转为 shared_ptr<PaperOrder>
 * 模拟盘订单包含 Order 基类字段及 limitPrice、stopPrice、orderSize 等扩展字段
 * @return 包含模拟盘订单的 shared_ptr，可转为 Order* 供 OrderManager 使用
 */
std::shared_ptr<Order> PaperOrderMsg::toPOrder() {
    auto o = make_shared<PaperOrder>();
    o->api_ = data_.api_;
    o->account_ = data_.account_;
    o->clientID_ = data_.clientID_;
    o->clientOrderID_ = data_.clientOrderID_;
    o->tag_ =  data_.tag_;
    o->fullSymbol_ = data_.fullSymbol_;
    o->price_ = data_.price_;
    o->quantity_ = data_.quantity_;
    o->tradedvol_ = data_.tradedvol_;
    o->flag_ = data_.flag_;
    o->serverOrderID_ = data_.serverOrderID_;
    o->brokerOrderID_ = data_.brokerOrderID_;
    o->orderNo_ = data_.orderNo_;
    o->localNo_ = data_.localNo_;
    o->createTime_ = data_.createTime_;
    o->updateTime_ = data_.updateTime_;
    o->orderStatus_ = data_.orderStatus_;

    o->orderType_ = data_.orderType_;
    o->orderSize_ = data_.orderSize_;
    o->limitPrice_ = data_.limitPrice_;
    o->stopPrice_ = data_.stopPrice_;
    o->orderFlag_ = data_.orderFlag_;

    return o;
}

/**
 * 从序列化字符串反序列化 CTP 实盘订单消息
 * 解析出 destination、source、api、account 等，并填充 CThostFtdcInputOrderField
 * 包括：合约代码、价格类型、方向、开平标志、限价、数量、时间条件、成交量条件等
 * @param msgin 以 | 分隔的 CTP 订单字符串
 * @throw std::out_of_range 若字段数少于 27
 */
void CtpOrderMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 27)
        throw std::out_of_range("OutofRange in CtpOrder deserialize.");
    destination_ = v[0];
    source_ = v[1];
    data_.api_ = v[3];
    data_.account_ = v[4];
    data_.clientID_ = stoi(v[5]);
    data_.clientOrderID_ = stol(v[6]);
    data_.tag_ = v[7];
    data_.orderType_ = OrderType(stoi(v[7]));

    data_.orderField_ = {};
    strcpy(data_.orderField_.InstrumentID, v[8].c_str());
    data_.orderField_.OrderPriceType = v[9][0];
    data_.orderField_.Direction = v[10][0];
    strcpy(data_.orderField_.CombOffsetFlag, v[11].c_str());
    strcpy(data_.orderField_.CombHedgeFlag, v[12].c_str());
    data_.orderField_.LimitPrice = stof(v[13]);
    data_.orderField_.VolumeTotalOriginal = stoi(v[14]);
    data_.orderField_.TimeCondition = v[15][0];
    strcpy(data_.orderField_.GTDDate, v[16].c_str());
    data_.orderField_.VolumeCondition = v[17][0];
    data_.orderField_.MinVolume = stoi(v[18]);
    data_.orderField_.ContingentCondition = v[19][0];
    data_.orderField_.StopPrice = stof(v[20]);
    data_.orderField_.ForceCloseReason = v[21][0];
    data_.orderField_.IsAutoSuspend = stoi(v[22]);
    data_.orderField_.UserForceClose = stoi(v[23]);
    data_.orderField_.IsSwapOrder = stoi(v[24]);
    strcpy(data_.orderField_.BusinessUnit, v[25].c_str());
    strcpy(data_.orderField_.CurrencyID, v[26].c_str());
}

/**
 * 将 CtpOrderMsg 的 data_ 转为 shared_ptr<CtpOrder>
 * 完整复制 CThostFtdcInputOrderField 到 CtpOrder，供 CTP 交易引擎下单
 * @return 包含 CTP 订单及原始 orderField 的 shared_ptr
 */
std::shared_ptr<Order> CtpOrderMsg::toPOrder() {
    std::shared_ptr<CtpOrder> o = make_shared<CtpOrder>();
    o->api_ = data_.api_;
    o->account_ = data_.account_;
    o->clientID_ = data_.clientID_;
    o->clientOrderID_ = data_.clientOrderID_;
    o->tag_ =  data_.tag_;
    o->orderType_ = data_.orderType_;

    o->fullSymbol_ = data_.fullSymbol_;
    o->price_ = data_.price_;
    o->quantity_ = data_.quantity_;
    o->tradedvol_ = data_.tradedvol_;
    o->flag_ = data_.flag_;
    o->serverOrderID_ = data_.serverOrderID_;
    o->brokerOrderID_ = data_.brokerOrderID_;
    o->orderNo_ = data_.orderNo_;
    o->localNo_ = data_.localNo_;
    o->createTime_ = data_.createTime_;
    o->updateTime_ = data_.updateTime_;
    o->orderStatus_ = data_.orderStatus_;
    memcpy(&o->orderField_, &data_.orderField_, sizeof(CThostFtdcInputOrderField));
    // return static_pointer_cast<Order>(o);
    return o;
}

/**
 * 从序列化字符串反序列化 CTP 预埋单消息
 * 格式与 CtpOrderMsg 类似，解析 CThostFtdcParkedOrderField 各字段
 * @param msgin 以 | 分隔的 CTP 预埋单字符串
 * @throw std::out_of_range 若字段数少于 27
 */
void CtpParkedOrderMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 27)
        throw std::out_of_range("OutofRange in CtpParkOrder deserialize.");
    destination_ = v[0];
    source_ = v[1];
    data_.api_ = v[3];
    data_.account_ = v[4];
    data_.clientID_ = stoi(v[5]);
    data_.clientOrderID_ = stol(v[6]);
    data_.tag_ = v[7];

    data_.parkedOrderField_ = {};
    strcpy(data_.parkedOrderField_.InstrumentID, v[8].c_str());
    data_.parkedOrderField_.OrderPriceType = v[9][0];
    data_.parkedOrderField_.Direction = v[10][0];
    strcpy(data_.parkedOrderField_.CombOffsetFlag, v[11].c_str());
    strcpy(data_.parkedOrderField_.CombHedgeFlag, v[12].c_str());
    data_.parkedOrderField_.LimitPrice = stof(v[13]);
    data_.parkedOrderField_.VolumeTotalOriginal = stoi(v[14]);
    data_.parkedOrderField_.TimeCondition = v[15][0];
    strcpy(data_.parkedOrderField_.GTDDate, v[16].c_str());
    data_.parkedOrderField_.VolumeCondition = v[17][0];
    data_.parkedOrderField_.MinVolume = stoi(v[18]);
    data_.parkedOrderField_.ContingentCondition = v[19][0];
    data_.parkedOrderField_.StopPrice = stof(v[20]);
    data_.parkedOrderField_.ForceCloseReason = v[21][0];
    data_.parkedOrderField_.IsAutoSuspend = stoi(v[22]);
    data_.parkedOrderField_.UserForceClose = stoi(v[23]);
    data_.parkedOrderField_.IsSwapOrder = stoi(v[24]);
    strcpy(data_.parkedOrderField_.BusinessUnit, v[25].c_str());
    strcpy(data_.parkedOrderField_.CurrencyID, v[26].c_str());
}

/**
 * 将 CtpParkedOrderMsg 的 data_ 转为 shared_ptr<CtpParkedOrder>
 * 完整复制 CThostFtdcParkedOrderField，供 CTP 预埋单引擎使用
 * @return 包含 CTP 预埋单的 shared_ptr
 */
std::shared_ptr<Order> CtpParkedOrderMsg::toPOrder() {
    auto o = make_shared<CtpParkedOrder>();
    o->api_ = data_.api_;
    o->account_ = data_.account_;
    o->clientID_ = data_.clientID_;
    o->clientOrderID_ = data_.clientOrderID_;
    o->tag_ =  data_.tag_;
    o->fullSymbol_ = data_.fullSymbol_;
    o->price_ = data_.price_;
    o->quantity_ = data_.quantity_;
    o->tradedvol_ = data_.tradedvol_;
    o->flag_ = data_.flag_;
    o->serverOrderID_ = data_.serverOrderID_;
    o->brokerOrderID_ = data_.brokerOrderID_;
    o->orderNo_ = data_.orderNo_;
    o->localNo_ = data_.localNo_;
    o->createTime_ = data_.createTime_;
    o->updateTime_ = data_.updateTime_;
    o->orderStatus_ = data_.orderStatus_;
    memcpy(&o->parkedOrderField_, &data_.parkedOrderField_, sizeof(CThostFtdcParkedOrderField));
    //return static_pointer_cast<Order>(o);
    return o;
}

/**
 * 从 Order 指针填充 OrderStatusMsg 的 data_
 * 用于将 OrderManager 中的订单状态封装为回报消息发送
 * @param po 订单指针，其各字段将被拷贝到 data_
 */
void OrderStatusMsg::set(std::shared_ptr<Order> po) {
    // data_ = *po;
    data_.api_ = po->api_;
    data_.account_ = po->account_;
    data_.clientID_ = po->clientID_;
    data_.clientOrderID_ = po->clientOrderID_;
    data_.tag_ =  po->tag_;
    data_.orderType_ = po->orderType_;
    data_.fullSymbol_ = po->fullSymbol_;
    data_.price_ = po->price_;
    data_.quantity_ = po->quantity_;
    data_.tradedvol_ = po->tradedvol_;
    data_.flag_ = po->flag_;
    data_.serverOrderID_ = po->serverOrderID_;
    data_.brokerOrderID_ = po->brokerOrderID_;
    data_.orderNo_ = po->orderNo_;
    data_.localNo_ = po->localNo_;
    data_.createTime_ = po->createTime_;
    data_.updateTime_ = po->updateTime_;
    data_.orderStatus_ = po->orderStatus_;
}

/**
 * 将订单状态回报消息序列化为字符串
 * 包含：destination、source、msgtype、api、account、clientID、clientOrderID、tag、
 * fullSymbol、price、quantity、tradedvol、flag、serverOrderID、brokerOrderID、
 * orderNo、localNo、createTime、updateTime、orderStatus
 * @return 以 | 分隔的订单状态字符串
 */
string OrderStatusMsg::serialize() {
    string str =  destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_.api_
        + SERIALIZATION_SEPARATOR + data_.account_
        + SERIALIZATION_SEPARATOR + std::to_string(data_.clientID_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.clientOrderID_)
        + SERIALIZATION_SEPARATOR + data_.tag_
        + SERIALIZATION_SEPARATOR + data_.fullSymbol_
        + SERIALIZATION_SEPARATOR + std::to_string(data_.price_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.quantity_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.tradedvol_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.flag_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.serverOrderID_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.brokerOrderID_)
        + SERIALIZATION_SEPARATOR + data_.orderNo_
        + SERIALIZATION_SEPARATOR + data_.localNo_
        + SERIALIZATION_SEPARATOR + data_.createTime_
        + SERIALIZATION_SEPARATOR + data_.updateTime_
        + SERIALIZATION_SEPARATOR + std::to_string(data_.orderStatus_);
    return str;
}

/**
 * 将持仓查询响应消息序列化为字符串
 * 包含：destination、source、msgtype、key、account、api、fullSymbol、type、
 * avgPrice、size、preSize、freezedSize、closedpl、openpl、当前时间
 * @return 以 | 分隔的持仓信息字符串
 */
string PosMsg::serialize() {
    string str =  destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_.key_
        + SERIALIZATION_SEPARATOR + data_.account_
        + SERIALIZATION_SEPARATOR + data_.api_
        + SERIALIZATION_SEPARATOR + data_.fullSymbol_
        + SERIALIZATION_SEPARATOR + data_.type_
        + SERIALIZATION_SEPARATOR + std::to_string(data_.avgPrice_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.size_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.preSize_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.freezedSize_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.closedpl_)
        + SERIALIZATION_SEPARATOR + std::to_string(data_.openpl_)
        + SERIALIZATION_SEPARATOR + ymdhmsf();
    return str;
}

/**
 * 从 Position 指针填充 PosMsg 的 data_
 * 用于将持仓查询结果封装为消息发送
 * @param pp 持仓指针，其各字段将被拷贝到 data_
 */
void PosMsg::set(std::shared_ptr<Position> pp) {
    // data_ = *pp;
    data_.key_ = pp->key_;
    data_.account_ = pp->account_;
    data_.api_ = pp->api_;
    data_.fullSymbol_ = pp->fullSymbol_;
    data_.avgPrice_ = pp->avgPrice_;
    data_.size_ = pp->size_;
    data_.preSize_ = pp->preSize_;
    data_.freezedSize_ = pp->freezedSize_;
    data_.openpl_ = pp->openpl_;
    data_.closedpl_ = pp->closedpl_;
    data_.type_ = pp->type_;
    data_.posNo_ = pp->posNo_;
}

/**
 * 从序列化字符串反序列化撤单消息
 * 格式：dest|src|msgtype|clientID|clientOrderID|serverOrderID
 * @param msgin 以 | 分隔的撤单请求字符串
 * @throw std::out_of_range 若字段数少于 6
 */
void OrderActionMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 6)
        throw std::out_of_range("OutofRange in OrderAction deserialize.");
    destination_ = v[0];
    source_ = v[1];
    data_.clientID_ = stoi(v[3]);
    data_.clientOrderID_ = stol(v[4]);
    data_.serverOrderID_ = stol(v[5]);
}

/**
 * 从序列化字符串反序列化订阅行情消息
 * 格式：dest|src|msgtype|symtype|symbol1|symbol2|...
 * @param msgin 以 | 分隔的订阅消息字符串
 * @throw std::out_of_range 若字段数少于 5
 * symtype 为标的类型，v[4] 起为待订阅合约列表
 */
void SubscribeMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 5)
        throw std::out_of_range("OutofRange in Subscribe deserialize.");
    destination_ = v[0];
    source_ = v[1];
    symtype_ = SymbolType(stoi(v[3]));
    for (int32_t i = 4; i < v.size(); i++)
        data_.push_back(v[i]);
}

/**
 * 从序列化字符串反序列化取消订阅消息
 * 格式与 SubscribeMsg 相同：dest|src|msgtype|symtype|symbol1|symbol2|...
 * @param msgin 以 | 分隔的取消订阅消息字符串
 * @throw std::out_of_range 若字段数少于 5
 */
void UnSubscribeMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 5)
        throw std::out_of_range("OutofRange in Unsubscribe deserialize.");
    destination_ = v[0];
    source_ = v[1];
    symtype_ = SymbolType(stoi(v[3]));
    for (int32_t i = 4; i < v.size(); i++)
        data_.push_back(v[i]);
}

/**
 * 从序列化字符串反序列化查询合约消息
 * 格式：dest|src|msgtype|symtype|queryCondition
 * @param msgin 以 | 分隔的查询合约消息字符串
 * @throw std::out_of_range 若字段数少于 5
 * data_ 为查询条件（如合约代码、交易所等）
 */
void QryContractMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 5)
        throw std::out_of_range("OutofRange in qrycontract deserialize.");
    destination_ = v[0];
    source_ = v[1];
    symtype_ = SymbolType(stoi(v[3]));
    data_ = v[4];
}

/**
 * 从序列化字符串反序列化撤销全部订单消息
 * 格式：dest|src|msgtype|symtype|filterCondition
 * @param msgin 以 | 分隔的撤全部消息字符串
 * @throw std::out_of_range 若字段数少于 5
 * data_ 为筛选条件（如合约代码，空表示撤全部）
 */
void CancelAllMsg::deserialize(const string& msgin) {
    vector<string> v = stringsplit(msgin, SERIALIZATION_SEPARATOR);
    if (v.size() < 5)
        throw std::out_of_range("OutofRange in cancelall deserialize.");
    destination_ = v[0];
    source_ = v[1];
    symtype_ = SymbolType(stoi(v[3]));
    data_ = v[4];
}

/**
 * 将错误消息序列化为字符串
 * 包含：destination、source、msgtype、错误描述 data_、当前时间
 * @return 以 | 分隔的错误信息字符串
 */
string ErrorMsg::serialize() {
    string str =  destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_
        + SERIALIZATION_SEPARATOR + ymdhms();
    return str;
}

/**
 * 将系统信息消息序列化为字符串
 * 包含：destination、source、msgtype、信息内容 data_、当前时间
 * @return 以 | 分隔的系统信息字符串
 */
string InfoMsg::serialize() {
    string str =  destination_
        + SERIALIZATION_SEPARATOR + source_
        + SERIALIZATION_SEPARATOR + to_string(msgtype_)
        + SERIALIZATION_SEPARATOR + data_
        + SERIALIZATION_SEPARATOR + ymdhms();
    return str;
}



}  // namespace StarQuant


