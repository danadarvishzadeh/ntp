import sys
from dataclasses import dataclass


@dataclass
class ParsedCommandLineArgs:
    host: str
    port: int


@dataclass
class ConnectionInfo:
    addr: str


def read_command_line_arguments() -> ParsedCommandLineArgs:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <host> <port>")
        sys.exit(1)
    host, port = sys.argv[1], int(sys.argv[2])
    return ParsedCommandLineArgs(host=host, port=port)
