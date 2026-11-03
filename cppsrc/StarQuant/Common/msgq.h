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

#ifndef CPPSRC_STARQUANT_COMMON_MSGQ_H_
#define CPPSRC_STARQUANT_COMMON_MSGQ_H_

#include <Common/logger.h>
#include <Common/datastruct.h>
#ifdef _WIN32
#include <nanomsg/src/nn.h>
#include <nanomsg/src/pubsub.h>
#else
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>
#endif
#include <mutex>
#include <memory>
#include <string>

using namespace std;

namespace StarQuant {

/**
 * 消息收发接口（抽象基类）
 * 子类实现：CMsgqEMessenger（引擎用）、CMsgqRMessenger（中继用）
 */
class IMessenger {
 protected:
    std::shared_ptr<SQLogger> logger;
 public:
    string name_;
    IMessenger();
    virtual ~IMessenger() {}

    virtual void send(std::shared_ptr<MsgHeader> pmsg, int32_t mode = 0) = 0;
    virtual std::shared_ptr<MsgHeader> recv(int32_t mode = 0) = 0;
    virtual void relay() = 0;  // 中继转发（仅 CMsgqRMessenger 实现）
};

/**
 * 消息队列抽象基类
 * 封装底层收发接口，支持 nanomsg/zmq 等实现
 */
class CMsgq {
 protected:
    MSGQ_PROTOCOL protocol_;   // 通信模式（PAIR/PUB/SUB/PUSH/PULL）
    string url_;               // 地址（如 tcp://localhost:55555）
    std::shared_ptr<SQLogger> logger;
 public:
    CMsgq(MSGQ_PROTOCOL protocol, string url);
    virtual ~CMsgq() {}
    virtual void sendmsg(const string& str, int32_t dontwait = 1) = 0;
    virtual void sendmsg(const char* str, int32_t dontwait = 1) = 0;
    virtual string recmsg(int32_t blockingflags = 1) = 0;  // 1=阻塞，0=非阻塞
};

/**
 * nanomsg 实现的消息队列
 */
class CMsgqNanomsg : public CMsgq {
 private:
    int32_t sock_ = -1;   // nanomsg socket 句柄
    int32_t eid_ = 0;    // bind/connect 返回的 endpoint id
    string endpoint_;
 public:
    CMsgqNanomsg(MSGQ_PROTOCOL protocol, string url, bool binding = true);
    ~CMsgqNanomsg();

    virtual void sendmsg(const string& str, int32_t dontwait = 1);
    virtual void sendmsg(const char* str, int32_t dontwait = 1);
    virtual string recmsg(int32_t blockingflags = 1);
};

/**
 * ZeroMQ 实现的消息队列（当前实现为空，仅保留接口）
 */
class CMsgqZmq : public CMsgq {
 private:
    void* context_;
    void* socket_;
    string endpoint_;
    int32_t rc_;
    char buf_[256];
 public:
    CMsgqZmq(MSGQ_PROTOCOL protocol, string url, bool binding = true);
    ~CMsgqZmq();

    virtual void sendmsg(const string& str, int32_t dontwait = 1);
    virtual void sendmsg(const char* str, int32_t dontwait = 1);
    virtual string recmsg(int32_t blockingflags = 1);
};

/**
 * 消息中心（管理全局 PUB 队列）
 */
class MsgCenter {
 public:
    static mutex msglock_;
    static std::unique_ptr<CMsgq> msgq_pub_;
    MsgCenter();
    ~MsgCenter();
};

/**
 * 引擎用消息收发器（MD/TD 引擎）
 * 接收：SUB 订阅；发送：共享 msgq_send_（PUB）
 */
class CMsgqEMessenger : public IMessenger {
 private:
    std::unique_ptr<CMsgq> msgq_recv_;  // 本实例的 SUB 接收端
 public:
    static mutex sendlock_;
    static std::unique_ptr<CMsgq> msgq_send_;  // 全局共享的 PUB 发送端

    explicit CMsgqEMessenger(string url_recv);
    CMsgqEMessenger(string name, string url_recv);
    virtual ~CMsgqEMessenger() {}

    virtual void send(shared_ptr<MsgHeader> pmsg, int32_t mode = 0);
    virtual shared_ptr<MsgHeader> recv(int32_t mode = 0);
    virtual void relay() {}  // 引擎不实现中继
};

/**
 * 中继用消息收发器（交易引擎）
 * 接收：PULL；发送：共享 msgq_send_（PUB）；relay 根据消息头转发到策略或引擎
 */
class CMsgqRMessenger : public IMessenger {
 private:
    std::unique_ptr<CMsgq> msgq_recv_;  // PULL 接收端

 public:
    explicit CMsgqRMessenger(string url_recv);
    virtual ~CMsgqRMessenger() {}

    static mutex sendlock_;
    static std::unique_ptr<CMsgq> msgq_send_;
    static void Send(std::shared_ptr<MsgHeader> pmsg, int32_t mode = 0);

    virtual void send(std::shared_ptr<MsgHeader> pmsg, int32_t mode = 0);
    virtual std::shared_ptr<MsgHeader> recv(int32_t mode = 0) { return nullptr; }  // 中继模式不直接 recv
    virtual void relay();  // 从 PULL 收消息，按 RELAY_DESTINATION 转发到策略或引擎
};

}  // namespace StarQuant

#endif  // CPPSRC_STARQUANT_COMMON_MSGQ_H_
