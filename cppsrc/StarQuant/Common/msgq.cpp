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

#include <Common/msgq.h>
#include <Common/logger.h>
#include <Common/config.h>
#include <Common/util.h>
#include <Common/datastruct.h>
#include <string.h>
#ifdef _WIN32
#include <nanomsg/src/nn.h>
#include <nanomsg/src/pair.h>
#include <nanomsg/src/pubsub.h>
#include <nanomsg/src/pipeline.h>
#else
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>
#endif
#include <iostream>
#include <cassert>

namespace StarQuant {

// ========== CMsgq 基类 ==========

CMsgq::CMsgq(MSGQ_PROTOCOL protocol, string url) {
    protocol_ = protocol;
    url_ = url;
    logger = SQLogger::getLogger("SYS");
}

IMessenger::IMessenger() {
    logger = SQLogger::getLogger("SYS");
}

/**
 * 创建 nanomsg socket 并 bind/connect
 * @param binding true=bind（服务端），false=connect（客户端）
 */
CMsgqNanomsg::CMsgqNanomsg(MSGQ_PROTOCOL protocol, string url, bool binding)
    : CMsgq(protocol, url) {
    endpoint_ = url;
    if (protocol_ == MSGQ_PROTOCOL::PAIR) {
        sock_ = nn_socket(AF_SP, NN_PAIR);
        assert(sock_ >= 0);
        int32_t to = 100;
        assert(nn_setsockopt(sock_, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof(to)) >= 0);
        if (binding) {
            eid_ = nn_bind(sock_, endpoint_.c_str());
        } else {
            eid_ = nn_connect(sock_, endpoint_.c_str());
        }
    } else if (protocol_ == MSGQ_PROTOCOL::PUB) {
        sock_ = nn_socket(AF_SP, NN_PUB);
        assert(sock_ >= 0);
        int32_t sndbuff = 1024 * 1024 * 256;  // 256MB 发送缓冲
        assert(nn_setsockopt(sock_, NN_SOL_SOCKET, NN_SNDBUF, &sndbuff, sizeof(sndbuff)) >= 0);
        eid_ = nn_bind(sock_, endpoint_.c_str());
    } else if (protocol_ == MSGQ_PROTOCOL::SUB) {
        sock_ = nn_socket(AF_SP, NN_SUB);
        assert(sock_ >= 0);
        int32_t to = 100;  // 接收超时 100ms
        assert(nn_setsockopt(sock_, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof(to)) >= 0);
        nn_setsockopt(sock_, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);  // 订阅所有主题（空前缀）
        eid_ = nn_connect(sock_, endpoint_.c_str());  // SUB 总是 connect 到 PUB
    } else if (protocol_ == MSGQ_PROTOCOL::PUSH) {
        sock_ = nn_socket(AF_SP, NN_PUSH);
        assert(sock_ >= 0);
        eid_ = nn_connect(sock_, endpoint_.c_str());
    } else if (protocol_ == MSGQ_PROTOCOL::PULL) {
        sock_ = nn_socket(AF_SP, NN_PULL);
        assert(sock_ >= 0);
        eid_ = nn_bind(sock_, endpoint_.c_str());  // PULL 作为服务端 bind
    }

    if (eid_ < 0 || sock_ < 0) {
        LOG_ERROR(logger, "Nanomsg connect sock " << endpoint_ << " error");
    }
}

std::mutex CMsgqEMessenger::sendlock_;
std::unique_ptr<CMsgq> CMsgqEMessenger::msgq_send_;

/**
 * 构造函数：url_send 为订阅地址（PUB 端地址），用于创建 SUB 接收
 */
CMsgqEMessenger::CMsgqEMessenger(string name, string url_send) {
    name_ = name;
    if (msgq_recv_ == nullptr) {
        msgq_recv_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, url_send);
    }
}

CMsgqEMessenger::CMsgqEMessenger(string url_send) {
    if (msgq_recv_ == nullptr) {
        msgq_recv_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::SUB, url_send);
    }
}

void CMsgqEMessenger::send(shared_ptr<MsgHeader> Msg, int32_t mode) {
    string msgout = Msg->serialize();
    lock_guard<std::mutex> g(CMsgqEMessenger::sendlock_);  // 多引擎共享发送端，需加锁
    CMsgqEMessenger::msgq_send_->sendmsg(msgout, mode);
}

/**
 * 从 SUB 接收消息，按 msgtype 反序列化为对应 MsgHeader 子类
 */
shared_ptr<MsgHeader> CMsgqEMessenger::recv(int32_t mode) {
    string msgin = msgq_recv_->recmsg(mode);
    if (msgin.empty())
        return nullptr;

    try {
        string des;
        string src;
        string stype;
        stringstream ss(msgin);
        getline(ss, des, SERIALIZATION_SEPARATOR);
        getline(ss, src, SERIALIZATION_SEPARATOR);
        getline(ss, stype, SERIALIZATION_SEPARATOR);
        MSG_TYPE mtype = MSG_TYPE(stoi(stype));
        std::shared_ptr<MsgHeader> pheader;
        switch (mtype) {
            case MSG_TYPE_ORDER:
                pheader = make_shared<OrderMsg>();
                pheader->msgtype_ = MSG_TYPE_ORDER;
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_ORDER_CTP:
                pheader = make_shared<CtpOrderMsg>();
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_ORDER_PAPER:
                pheader = make_shared<PaperOrderMsg>();
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_SUBSCRIBE_MARKET_DATA:
                pheader = make_shared<SubscribeMsg>(des, src);
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_UNSUBSCRIBE:
                pheader = make_shared<UnSubscribeMsg>(des, src);
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_ORDER_ACTION:
            case MSG_TYPE_CANCEL_ORDER:
                pheader = make_shared<OrderActionMsg>();
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_CANCEL_ALL:
                pheader = make_shared<CancelAllMsg>();
                pheader->deserialize(msgin);
                break;
            case MSG_TYPE_QRY_CONTRACT:
                pheader = make_shared<QryContractMsg>(des, src);
                pheader->deserialize(msgin);
                break;
            default:
                pheader = make_shared<MsgHeader>(des, src, mtype);
                break;
        }
        return pheader;
    }
    catch (std::exception& e) {
        LOG_ERROR(logger, e.what() << " [original msg:" << msgin << "]");
        return nullptr;
    }
    catch (...) {
        LOG_ERROR(logger, "MSGQ cannot deserialize msg:" << msgin);
        return nullptr;
    }
}


std::mutex CMsgqRMessenger::sendlock_;
std::unique_ptr<CMsgq> CMsgqRMessenger::msgq_send_;

void CMsgqRMessenger::Send(shared_ptr<MsgHeader> Msg, int32_t mode) {
    string msgout = Msg->serialize();
    lock_guard<std::mutex> g(CMsgqRMessenger::sendlock_);
    CMsgqRMessenger::msgq_send_->sendmsg(msgout, mode);
}

/**
 * 构造函数：创建 PULL 接收端，msgq_send_ 由外部在 MsgCenter 等处初始化
 */
CMsgqRMessenger::CMsgqRMessenger(string url_recv) {
    msgq_recv_ = std::make_unique<CMsgqNanomsg>(MSGQ_PROTOCOL::PULL, url_recv);
}

void CMsgqRMessenger::send(shared_ptr<MsgHeader> Msg, int32_t mode) {
    string msgout = Msg->serialize();
    lock_guard<std::mutex> g(CMsgqRMessenger::sendlock_);
    CMsgqRMessenger::msgq_send_->sendmsg(msgout, mode);
}

/**
 * 中继逻辑：从 PULL 收消息，按首字符转发
 * 若首字符为 RELAY_DESTINATION (@)：发往 CMsgqEMessenger（策略进程 SUB）
 * 否则：发往 CMsgqRMessenger（各引擎 SUB）
 */
void CMsgqRMessenger::relay() {
    string msgpull = msgq_recv_->recmsg(0);  // 非阻塞
    if (msgpull.empty())
        return;

    if (msgpull[0] == RELAY_DESTINATION) {
        lock_guard<std::mutex> g(CMsgqEMessenger::sendlock_);
        CMsgqEMessenger::msgq_send_->sendmsg(msgpull);  // 转发给策略进程
    } else {
        lock_guard<std::mutex> g(CMsgqRMessenger::sendlock_);
        CMsgqRMessenger::msgq_send_->sendmsg(msgpull);  // 转发给各引擎
    }
}





// ========== CMsgqNanomsg ==========

CMsgqNanomsg::~CMsgqNanomsg() {
    nn_shutdown(sock_, eid_);
    nn_close(sock_);
}

void CMsgqNanomsg::sendmsg(const string& msg, int32_t dontwait) {
    int32_t bytes = nn_send(sock_, msg.c_str(), msg.size() + 1, dontwait);  // +1 含 '\0' 结尾

    if (bytes != static_cast<int32_t>(msg.size() + 1)) {
        LOG_ERROR(logger, "Nanomsg " << endpoint_ << " send msg error, return:" << bytes);
    }
}

void CMsgqNanomsg::sendmsg(const char* msg, int32_t dontwait) {
    nn_send(sock_, msg, strlen(msg) + 1, dontwait);
}

string CMsgqNanomsg::recmsg(int32_t dontwait) {
    char* buf = nullptr;
    int32_t bytes = nn_recv(sock_, &buf, NN_MSG, dontwait);

    if (bytes > 0 && buf != nullptr) {
        string msg(buf, bytes);
        nn_freemsg(buf);  // 释放 nanomsg 分配的内存
        return msg;
    } else {
        return string();
    }
}

// ========== CMsgqZmq（当前未实现，仅保留接口） ==========

CMsgqZmq::CMsgqZmq(MSGQ_PROTOCOL protocol, string port, bool binding)
    : CMsgq(protocol, port) {
    // if (protocol_ == MSGQ_PROTOCOL::PAIR)
    // {
    // 	context_ = zmq_ctx_new();
    // 	socket_ = zmq_socket(context_, ZMQ_PAIR);

    // 	if (binding) {
    // 		endpoint_ = "tcp://*:" + port;
    // 		rc_ = zmq_bind(socket_, endpoint_.c_str());
    // 	}
    // 	else {
    // 		endpoint_ = "tcp://localhost:" + port;
    // 		rc_ = zmq_connect(socket_, endpoint_.c_str());
    // 	}			
    // }
    // else if (protocol_ == MSGQ_PROTOCOL::PUB) {
    // 	context_ = zmq_ctx_new();
    // 	socket_ = zmq_socket(context_, ZMQ_PUB);

    // 	endpoint_ = "tcp://*:" + port;
    // 	rc_ = zmq_bind(socket_, endpoint_.c_str());
    // }
    // else if (protocol_ == MSGQ_PROTOCOL::SUB) {
    // 	context_ = zmq_ctx_new();
    // 	socket_ = zmq_socket(context_, ZMQ_SUB);

    // 	endpoint_ = "tcp://localhost:" + port;
    // 	rc_ = zmq_connect(socket_, endpoint_.c_str());
    // }
}

CMsgqZmq::~CMsgqZmq() {
    // rc_ = zmq_unbind(socket_, endpoint_.c_str());
    // zmq_close(socket_);
    // zmq_ctx_shutdown(context_);
    // zmq_term(context_);
    // zmq_ctx_destroy(context_);
}

// ZMQ 需用 ZMQ_DONTWAIT 实现非阻塞，无全局 nn_term()
void CMsgqZmq::sendmsg(const string& msg, int32_t dontwait) {
    // int32_t bytes = zmq_send(socket_, msg.c_str(), msg.size() + 1, 0);
}

void CMsgqZmq::sendmsg(const char* msg, int32_t dontwait) {
    // int32_t bytes = zmq_send(socket_, msg, strlen(msg)+1, 0);
}

string CMsgqZmq::recmsg(int32_t blockingflags) {
    // int32_t bytes = zmq_recv(socket_, buf_, 1024, blockingflags);  // ZMQ_NOBLOCK

    // if (bytes > 0) {
    // 	buf_[bytes] = '\0';
    // 	string msg(buf_);
    // 	return msg;
    // }
    // else {
    // 	return string();
    // }
}

// ========== MsgCenter ==========

mutex MsgCenter::msglock_;
std::unique_ptr<CMsgq> MsgCenter::msgq_pub_;

}  // namespace StarQuant
