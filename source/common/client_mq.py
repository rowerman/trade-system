#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
StarQuant 客户端消息队列模块

本模块基于 nanomsg 实现 GUI 客户端与交易服务端的进程间通信：
- 订阅（SUB）：接收服务端推送的行情、委托、成交等事件
- 推送（PUSH）：将客户端的查询、下单等请求发送给服务端
"""

from threading import Thread
from nanomsg import Socket, SUB, PUSH, SUB_SUBSCRIBE, SOL_SOCKET, RCVTIMEO
from datetime import datetime
import os

from .datastruct import Event


class ClientMq(object):
    """
    客户端消息队列。

    在独立线程中运行，负责：
    1. 从服务端订阅（SUB）接收事件，反序列化后投递到 UI 事件引擎
    2. 从 outgoing_queue 取出请求，通过 PUSH 发送给服务端
    """

    def __init__(self, config, ui_event_engine, outgoing_queue):
        """
        Args:
            config: 配置字典，需包含 serverpub_url、serverpull_url、serversub_url
            ui_event_engine: UI 事件引擎，用于接收并分发服务端推送的事件
            outgoing_queue: 待发送请求队列，客户端将从此队列取消息发送给服务端
        """
        self._ui_event_engine = ui_event_engine
        self._outgoing_queue = outgoing_queue
        self._config = config

        self._active = False
        self._thread = Thread(target=self._run)

    def _run(self):
        """消息循环：接收服务端事件并投递到 UI，同时发送 outgoing 队列中的请求"""
        while self._active:
            try:
                # 从服务端接收推送消息（行情、委托回报、成交回报等）
                msgin = self._recv_sock.recv(flags=0)
                msgin = msgin.decode("utf-8")
                if msgin is not None and msgin.index('|') > 0:
                    # 去除末尾可能的空字符
                    if msgin[-1] == '\0':
                        msgin = msgin[:-1]
                    if msgin[-1] == '\x00':
                        msgin = msgin[:-1]
                    m = Event()
                    m.deserialize(msgin)
                    self._ui_event_engine.put(m)
            except Exception as e:
                pass
            try:
                # 从 outgoing 队列取请求并发送给服务端（非阻塞）
                msgout = self._outgoing_queue.get(False)
                print('outgoing get msg,begin send', msgout, datetime.now())
                self._send_sock.send(msgout, flags=1)
                print('outgoing end send', msgout, datetime.now())
            except Exception as e:
                pass

    def start(self, timer=True):
        """
        启动消息队列线程。

        创建 SUB/PUSH 套接字并连接到配置中的服务端地址，
        然后启动后台线程执行 _run 消息循环。
        """
        self._recv_sock = Socket(SUB)
        self._send_sock = Socket(PUSH)
        self._monitor_sock = Socket(SUB)

        # 订阅服务端发布的消息（行情、回报等）
        self._recv_sock.connect(self._config['serverpub_url'])
        self._recv_sock.set_string_option(SUB, SUB_SUBSCRIBE, '')
        self._recv_sock.set_int_option(SOL_SOCKET, RCVTIMEO, 100)  # 100ms 超时
        # 推送请求到服务端（查询、下单等）
        self._send_sock.connect(self._config['serverpull_url'])
        # 监控/状态订阅
        self._monitor_sock.connect(self._config['serversub_url'])

        self._active = True
        if not self._thread.isAlive():
            self._thread.start()

    def stop(self):
        """
        停止消息队列线程。

        设置 _active 为 False 退出循环，并等待线程结束。
        """
        self._active = False

        if self._thread.isAlive():
            self._thread.join()
