"""
StarQuant 事件驱动引擎

基于 vn.py 风格的事件驱动框架，包含 EventEngine（事件分发、定时器）
与 BaseEngine（引擎基类）。所有策略、回测、录制等引擎均依赖此模块。
"""

from collections import defaultdict
from queue import Empty, Queue
from threading import Thread
from time import sleep
from typing import Any, Callable
from abc import ABC
from ..common.datastruct import Event
from ..common.constant import EventType

# class Event:
#     """
#     Event object consists of a type string which is used
#     by event engine for distributing event, and a data
#     object which contains the real data.
#     """

#     def __init__(self, type: str, data: Any = None):
#         """"""
#         self.type = type
#         self.data = data


# 事件处理器类型：接收 Event 无返回值
HandlerType = Callable[[Event], None]


class EventEngine:
    """
    事件引擎：按事件类型分发到已注册的处理器。

    同时按 interval 秒间隔生成定时器事件，用于定时任务。
    """

    def __init__(self, interval: int = 1):
        """
        Args:
            interval: 定时器事件间隔（秒），默认 1 秒
        """
        self._interval = interval
        self._queue = Queue()
        self._active = False
        self._thread = Thread(target=self._run)
        self._timer = Thread(target=self._run_timer)
        self._handlers = defaultdict(list)
        self._general_handlers = []

    def _run(self):
        """从队列取事件并分发给对应类型的处理器"""
        while self._active:
            try:
                event = self._queue.get(block=True, timeout=1)
                if event.event_type in self._handlers:
                    [handler(event)
                     for handler in self._handlers[event.event_type]]
            except Empty:
                pass

    # def _process(self, event: Event):
    #     """
    #     First ditribute event to those handlers registered listening
    #     to this type.

    #     Then distrubute event to those general handlers which listens
    #     to all types.
    #     """
    #     if event.type in self._handlers:
    #         [handler(event) for handler in self._handlers[event.type]]

    #     if self._general_handlers:
    #         [handler(event) for handler in self._general_handlers]

    def _run_timer(self):
        """按 interval 秒间隔休眠后生成定时器事件并投递"""
        while self._active:
            sleep(self._interval)
            event = Event(type=EventType.TIMER)
            self.put(event)

    def start(self):
        """
        Start event engine to process events and generate timer events.
        """
        self._active = True
        self._thread.start()
        self._timer.start()

    def stop(self):
        """
        Stop event engine.
        """
        self._active = False
        self._timer.join()
        self._thread.join()

    def put(self, event: Event):
        """
        Put an event object into event queue.
        """
        self._queue.put(event)

    def register(self, type: EventType, handler: HandlerType):
        """注册指定事件类型的处理器，同一处理器对同一类型只注册一次"""
        handler_list = self._handlers[type]
        if handler not in handler_list:
            handler_list.append(handler)

    def unregister(self, type: EventType, handler: HandlerType):
        """取消注册指定类型的处理器"""
        handler_list = self._handlers[type]

        if handler in handler_list:
            handler_list.remove(handler)

        if not handler_list:
            self._handlers.pop(type)

    def register_general(self, handler: HandlerType):
        """注册通用处理器，接收所有类型事件"""
        if handler not in self._general_handlers:
            self._general_handlers.append(handler)

    def unregister_general(self, handler: HandlerType):
        """
        Unregister an existing general handler function.
        """
        if handler in self._general_handlers:
            self._general_handlers.remove(handler)


class BaseEngine(ABC):
    """
    引擎抽象基类。

    持有 EventEngine，子类实现具体业务逻辑（策略、回测、录制等）。
    """

    def __init__(self, event_engine: EventEngine = None):
        """若未传入则创建默认 EventEngine"""
        if event_engine:
            self.event_engine = event_engine
        else:
            self.event_engine = EventEngine()

    def start(self):
        self.event_engine.start()

    def close(self):
        """"""
        pass
