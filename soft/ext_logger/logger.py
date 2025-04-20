#!/usr/bin/python3

import threading
import codecs
import sys
import os
import os.path
from socket import *

BUF_SIZE = 4096

COLOR_B = '\x1B[44m'
COLOR_E = '\x1B[K\x1B[0m'
EOL = '\x1B[K'
COLOR_H = '\x1B[42m'

run_logger_thread = True

try:
    import msvcrt

    def getkey():
        return codecs.decode(msvcrt.getch())

    def before_exit():
        pass

except ModuleNotFoundError:
    try:
        import tty
        import termios

        org_mode_stdin = tty.setraw(sys.stdin.fileno())

        def getkey():
            return sys.stdin.read(1)

        def before_exit():
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSANOW, org_mode_stdin)


    except ModuleNotFoundError:
        run_logger_thread = False


cur_log_file = None
log_suspended = False
log_idx = 0

def start_srv(port: int) -> "socket":
    sock = create_server(("", port))
    assert sock, "Can't create TCP server"
    
    while True:
        print(f"{COLOR_B}*** Waiting for connection ...{COLOR_E}\r")
        clnt = sock.accept()[0]
        print(f"{COLOR_B}*** Connected{COLOR_E}\r")
        while True:
            data = clnt.recv(BUF_SIZE)
            if len(data) == 0:
                break
            if cur_log_file and not log_suspended:
                cur_log_file.write(data.replace(b'\n', b'\r\n'))
            print(codecs.decode(data), end='')
        print(f"\n{COLOR_B}*** Connection closed{COLOR_E}\r")


def prn_log_status(msg):
    if not cur_log_file:
        print(f"\n{COLOR_H}{msg}Log file not active{COLOR_E}\r")
    else:
        m2 = " [but suspended]" if log_suspended else ""
        print(f"\n{COLOR_H}{msg}Log file active: log{log_idx}.txt{m2}{COLOR_E}\r")

def ctrl_thread():
    global cur_log_file, log_suspended, log_idx

    while True:
        key = getkey()
        if cur_log_file:
            cur_log_file.flush()
        match key:
            case '\3': 
                if cur_log_file:
                    cur_log_file.close()
                before_exit()
                os._exit(0)
            case 'h': print(f"""\n{COLOR_H}Help{EOL}\r
h  - This help{EOL}\r
l  - Start writing to log file in current directory (or start new log file){EOL}\r
f  - Force flush of current log file (also print current status of logging){EOL}\r
s  - Suspend/restore log file writing{EOL}\r
c  - Close log file{EOL}\r
^C - Exit{COLOR_E}\r""")
            case 'l':
                log_suspended = False
                if cur_log_file:
                    cur_log_file.close()
                while True:
                    log_idx += 1
                    if not os.path.isfile(f'log{log_idx}.txt'):
                        break
                cur_log_file = open(f'log{log_idx}.txt', 'wb')
                prn_log_status('New log. ')
            case 's':
                log_suspended = not log_suspended
                prn_log_status('')
            case 'f':
                prn_log_status('Status. ')
            case 'c':
                if cur_log_file:
                    cur_log_file.close()
                    cur_log_file = None
                    prn_log_status('Closed. ')
                else:
                    prn_log_status('Not opened. ')

if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("Usage: logger.py <port-number>")
    else:
        if run_logger_thread:
            print(f"{COLOR_H}Type 'h' for help{COLOR_E}\r")
            threading.Thread(target = ctrl_thread).start()
        start_srv(int(sys.argv[1]))
