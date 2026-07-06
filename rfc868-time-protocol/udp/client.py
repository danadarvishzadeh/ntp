import datetime
import socket
import traceback

from server import read_command_line_arguments
from utility import ParsedCommandLineArgs


class TimeClient:
    def __init__(self, cmd_args: ParsedCommandLineArgs) -> None:
        self.host, self.port = cmd_args.host, cmd_args.port

    def connect_to_host(self):
        self._clsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._clsocket.connect((self.host, self.port))
        self._buffer = b""

    def get_time_from_server(self):
        try:
            self.read_from_connection()
            if not self._buffer:
                raise ValueError("_buffer is empty")
            self.print_time()
        except ValueError as e:
            print(str(e))
        except Exception:
            print(f"Main: Error: Exception\n {traceback.format_exc()}")

    def read_from_connection(self):
        while True:
            data = self._clsocket.recv(1024)
            if not data:
                break
            self._buffer += data

    def print_time(self):
        total_seconds = self._buffer.decode("utf-8")
        time_difference = datetime.timedelta(seconds=int(total_seconds, 2))
        now = datetime.datetime.fromisoformat("1900-01-01 00:00:00") + time_difference
        print(now.isoformat())


if __name__ == "__main__":
    cmd_args = read_command_line_arguments()
    try:
        client = TimeClient(cmd_args)
        client.connect_to_host()
        client.get_time_from_server()
    except KeyboardInterrupt:
        print("Caught keyboard interrupt, exiting")
