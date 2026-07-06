import selectors
import socket
import traceback
from datetime import datetime

from utility import ConnectionInfo, ParsedCommandLineArgs, read_command_line_arguments


class TimeServer:
    def __init__(self, cmd_args: ParsedCommandLineArgs) -> None:
        self.host, self.port = cmd_args.host, cmd_args.port
        self._selector = selectors.DefaultSelector()

    def bind_to_host(self):
        self._lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._lsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._lsock.bind((self.host, self.port))
        self._lsock.listen()
        self._lsock.setblocking(False)
        print(f"Listening on {(self.host, self.port)}")
        self._selector.register(self._lsock, selectors.EVENT_READ, data=None)

    def accept_wrapper(self, lsock: socket.socket):
        conn, addr = lsock.accept()
        print(f"Accepted a connection from {addr}")
        conn.setblocking(False)
        connection_info = ConnectionInfo(addr=str(addr))
        self._selector.register(conn, selectors.EVENT_WRITE, data=connection_info)

    def service_connection(self, conn: socket.socket, data: ConnectionInfo):
        with conn:
            send_data = int(
                (
                    datetime.now() - datetime.fromisoformat("1900-01-01 00:00:00")
                ).total_seconds()
            )
            print(f"Sending time to connection {data.addr}")
            send_data = format(send_data, "032b")
            conn.sendall(send_data.encode("utf-8"))
            self._selector.unregister(conn)

    def run(self):
        while True:
            events = self._selector.select(timeout=None)
            for key, mask in events:
                try:
                    if isinstance(key.fileobj, socket.socket):
                        if key.data is None:
                            self.accept_wrapper(key.fileobj)
                        else:
                            self.service_connection(key.fileobj, key.data)
                    else:
                        raise TypeError("fileobj must be instance of socket.socket")
                except TypeError as e:
                    print(str(e))
                except Exception:
                    print(
                        f"Main: Error: Exception for {getattr(key.fileobj, "host")}:{getattr(key.fileobj, "port")}:\n"
                        f"{traceback.format_exc()}"
                    )


if __name__ == "__main__":
    cmd_args = read_command_line_arguments()
    try:
        server = TimeServer(cmd_args)
        server.bind_to_host()
        server.run()
    except KeyboardInterrupt:
        print("Caught keyboard interrupt, exiting")
