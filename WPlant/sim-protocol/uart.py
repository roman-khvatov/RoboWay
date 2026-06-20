from typing import *

from schedule import *

ERROR = object()

class UART:
    def __init__(self, scheduler: Client, uart_name: str):
        self.scheduler = scheduler
        # TODO!: Load setup from Config
        self.name = uart_name
        self.peer = None
        self.dir_sending = False
        self.sender_ready = Event()
        self.sender_ready.set()
        self.reader_ready = Event()

        self.data_rcvd : Optional[int|ERROR]= None
        self.when_send_rdy : Optional[int] = None

    def connect_to(self, peer_uart: Self):
        assert not self.peer
        assert not peer_uart.peer
        self.peer = peer_uart
        peer_uart.peer = self

    async def send(self, data: int, stop_bits: int):
        if not self.dir_sending:
            report_error(f"UART {self.name}: Can't send - UART is read mode")
        await self.wait_send_end()
        self.send_now(data, stop_bits)

    def send_now(self, data: int, stop_bits: int):
        if not self.dir_sending:
            report_error(f"UART {self.name}: Can't send - UART is read mode")
        if not self.sender_ready.is_set():
            report_error(f"UART {self.name}: Can't send - UART busy")
        self.sender_ready.clear()
        self.scheduler.fire_after(10+stop_bits, self.sender_ready)
        if self.peer:
            p = self.peer
            if p.reader_ready.is_set():
                report_warning(f"UART {self.peer.name} recieve overflow")
            p.reader_ready.clear()
            p.data_rcvd = data # TODO! Add here error injection and bytes lost
            self.scheduler.fire_after(11, p.reader_ready)

    async def wait_send_end(self):
        await self.scheduler.wait(event=self.sender_ready)

    async def recv(self, timeout: Optional[int]=None) -> Union[int,ERROR,None]:
        if self.dir_sending:
            report_error(f"UART {self.name}: Can't read - UART is send mode")
        result = await self.scheduler.wait(ticks=timeout, event=self.reader_ready)
        if result == WaitResult.Timeout:
            return None
        self.reader_ready.clear()      
        result = self.data_rcvd
        self.data_rcvd = None
        return result

    def set_directon(self, is_send: bool):
        " Set direction and check for direction of own UART and connected to up/down "
        self.dir_sending = is_send
        if self.peer and self.dir_sending and self.peer.dir_sending:
            report_error(f'UART {self.name} <-> {self.peer.name} write/write direction conflict')
