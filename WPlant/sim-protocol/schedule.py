import asyncio

__all__ = ('Event', 'WaitResult', 'Client')

from dataclasses import dataclass, field
from heapq import heappush, heappop
from enum import Enum
from typing import *

# We wrap asyncio Event to prevent raw 'await <event>.wait()' on it, because this will break our Scheduler (Tick Increment machinery)
class Event:
    def __init__(self):
        self.e = asyncio.Event()

    def set(self):
        self.e.set()

    def clear(self):
        self.e.clear()

    def is_set(self) -> bool:
        return self.e.is_set()

class WaitResult(Enum):
    Timeout = 1         # Wakeup by timeout (in 'wait()' call of Scheduler)
    Wakeup = 2          # Wakeup by external function call (by 'wakeup()' function)
    WakeupByEvent = 3   # Wakeup by Event raise (from 'wait(event)' call of Scheduler)

@dataclass
class _THClient:
    client: 'Client'
    main: Coroutine
    task: asyncio.Task = None
    waiting_tick: Optional[int] = None
    waiting_event: Optional[Event] = None
    event: asyncio.Event = None
    result: WaitResult = None

@dataclass(order=True)
class _ClientQueueItem:
    time: int
    clnt: Optional[_THClient]=field(compare=False, default=None)
    cb: Optional[Callable[[], None]] = None
    event: Optional[Event] = None

class _TimingHub:
    def __init__(self):
        self.clients : dict[object, _THClient] = {}
        self.current_tick = 0
        self.awaited_count = 0
        self.tick_limit = None
        self.queue : list[PrioritizedItem] = []

    def add_client(self, client: 'Client', main: Coroutine):
        self.clients[client] = _THClient(client, main)

    def stop(self):
        for clnt in self.clients.values():
            clnt.task.cancel()
            clnt.task = None

    def run(self):
        asyncio.run(self.main())

    async def main(self):
        async with asyncio.TaskGroup() as tg:
            for clnt in self.clients.values():
                clnt.task = tg.create_task(clnt.main)

    async def wait(self, client, my_event: asyncio.Event, ticks: Optional[int] = None, event: Optional[Event]=None) -> WaitResult:
        self._wakeup_by_internal_events()
        if event and event.e.is_set():
            return WaitResult.WakeupByEvent
        clnt = self.clients[client]
        clnt.event = my_event
        clnt.waiting_event = event
        self.awaited_count += 1
        if ticks is not None:
            assert tick
            limit = ticks + self.current_tick
            clnt.waiting_tick = limit
            heappush(self.queue, _ClientQueueItem(limit, clnt=clnt))
        else:
            clnt.waiting_tick = None
        if self.awaited_count >= len(self.clients):
            self._advance_ticks()
        await my_event
        my_event.clear()
        return clnt.result

    def call_after(self, ticks: int, cb: Callable[[], None]):
        heappush(self.queue, _ClientQueueItem(ticks + self.current_tick, cb=cb))

    def fire_after(self, ticks: int, event: Event):
        heappush(self.queue, _ClientQueueItem(ticks + self.current_tick, event=event))

    def wakeup(self, client):
        clnt = self.clients[client]
        if not clnt.event.is_set():
            clnt.result = WaitResult.Wakeup
            self.awaited_count -= 1
            clnt.event.set()
            if clnt.waiting_tick is not None:
                for item in self.queue:
                    if item.clnt == clnt:
                        item.clnt = None
                        break

    def _wakeup_by_internal_events(self):
        for clnt in self.clients.values():
            if clnt.waiting_event and clnt.waiting_event.e.is_set():
                clnt.waiting_event.e.clear()
                self.wakeup(clnt.client)
                clnt.result = WaitResult.WakeupByEvent

    def _advance_ticks(self):
        self._wakeup_by_internal_events()

        assert self.queue
        self.current_tick = self.queue[0].time
        while self.queue and self.queue[0].time == self.current_tick:
            item = heappop(self.queue)
            if item.clnt:
                self.awaited_count -= 1
                item.clnt.result = WaitResult.Timeout
                item.clnt.event.set()
            if item.cb:
                item.cb()
            if item.event:
                item.event.e.set()
        if self.tick_limit is not None and self.current_tick >= self.tick_limit:
            self.stop()

class Client:
    root = _TimingHub()

    def __init__(self, clnt_name: str, main_entry: Coroutine):
        self.name = clnt_name
        self._event = asyncio.Event()
        self.root.add_client(self, main_entry)
        self.tick_adjust = 1 # Extract from config by 'clnt_name'

    async def wait(self, ticks: Optional[int] = None, event: Optional[Event]=None) -> WaitResult:
        " Wait until 'ticks' past, or when 'wakeup()' called "
        return await self.root.wait(self, self._event, ticks=ticks*self.tick_adjust, event=event)

    @classmethod
    def call_after(cls, ticks: int, cb: Callable[[], None]):
        clr.root.call_after(ticks*self.tick_adjust, cb)

    @classmethod
    def fire_after(cls, ticks: int, event: Event):
        clr.root.fire_after(ticks*self.tick_adjust, event)

    def wakeup(self):
        self.root.wakeup(self)

    @classmethod
    def current_tick(cls) -> int:
        return cls.root.current_tick/self.tick_adjust

    @classmethod
    def set_tick_limit(cls, maximum_tick: int):
        # Move to Config!
        cls.root.tick_limit = maximum_tick

    @classmethod
    def run(cls):
        cls.root.run()

    @classmethod
    def stop(cls):
        cls.root.stop()
