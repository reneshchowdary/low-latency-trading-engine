#!/usr/bin/env python3
"""End-to-end loopback TCP test: fragmentation, order matching and client isolation."""
import socket
import subprocess
import sys
import time

binary = sys.argv[1]
port = 19000 + (int(time.time() * 1000) % 20000)
server = subprocess.Popen([binary, str(port)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
try:
    for _ in range(100):
        try:
            a = socket.create_connection(("127.0.0.1", port), timeout=0.3)
            break
        except OSError:
            if server.poll() is not None:
                raise RuntimeError("server exited before accepting connections")
            time.sleep(0.03)
    else:
        raise RuntimeError("server failed to listen")
    with a:
        a.settimeout(2)
        with socket.create_connection(("127.0.0.1", port), timeout=2) as b:
            b.settimeout(2)
            a.sendall(b"ORDER 100 SELL LI")
            a.sendall(b"MIT 101 10\n")
            assert b"ACCEPTED" in a.recv(4096)
            b.sendall(b"ORDER 101 BUY LIMIT 105 7\n")
            response = b.recv(4096)
            assert b"FILLED" in response and b"price=101 qty=7" in response, response
            a.sendall(b"TOP\n")
            assert b"ask=101" in a.recv(4096)
            b.sendall(b"CANCEL 100\n")
            assert b"CANCELLED" in b.recv(4096)
            b.sendall(b"STATS\n")
            assert b"submitted=2" in b.recv(4096)
            b.sendall(b"ORDER 102 BUY LIMIT -1 1\n")
            assert b"ERROR" in b.recv(4096)
            b.sendall(b"ORDER 103 BUY LIMIT 1000000001 1\\n")
            assert b"REJECTED risk-limit" in b.recv(4096)
    print("TCP end-to-end integration passed")
finally:
    server.terminate()
    try:
        server.wait(timeout=3)
    except subprocess.TimeoutExpired:
        server.kill()
        server.wait()
