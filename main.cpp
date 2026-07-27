#include "Server.hpp"
#include <cstdlib>
#include <sstream>

// ─── Argument validation ──────────────────────────────────────────────────────

static bool validPort(const std::string& portStr)
{
    std::istringstream ss(portStr);
    int port = 0;
    ss >> port;
    if (ss.fail() || !ss.eof())
        return false;
    return port >= 1 && port <= 65535;
}

static bool emptyPassword(const std::string& pass)
{
    if (pass.empty())
        return true;
    const std::string ws = " \t\n\r";
    return pass.find_first_not_of(ws) == std::string::npos;
}

// ─── Entry point ─────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    if (!validPort(argv[1])) {
        std::cerr << "Error: invalid port number" << std::endl;
        return 1;
    }
    if (emptyPassword(argv[2])) {
        std::cerr << "Error: password must not be empty or whitespace-only" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    Server server(port, argv[2]);

    if (!server.init()) {
        std::cerr << "Error: could not start the server" << std::endl;
        return 1;
    }

    server.run();
    return 0;
}


// #!/usr/bin/env python3
// """
// IRC server edge-case test harness.

// Exercises the exact class of bugs the 42 ft_irc subject warns about:
// partial/fragmented data, pipelined commands, split CRLF terminators,
// "low bandwidth" byte-by-byte delivery, oversized unterminated lines,
// abrupt disconnects, and concurrent clients.

// Usage:
//     python3 irc_test.py [host] [port] [password]

// Example:
//     python3 irc_test.py 127.0.0.1 6667 mypass
// """
// import socket
// import sys
// import time
// import threading

// HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
// PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 6667
// PASSWORD = sys.argv[3] if len(sys.argv) > 3 else None


// def connect():
//     s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
//     s.settimeout(3)
//     s.connect((HOST, PORT))
//     return s


// def recv_all(s, timeout=1.0):
//     """Drain whatever the server sends back within `timeout` seconds."""
//     s.settimeout(timeout)
//     data = b""
//     try:
//         while True:
//             chunk = s.recv(4096)
//             if not chunk:
//                 break
//             data += chunk
//     except socket.timeout:
//         pass
//     return data


// def register(s, nick, user):
//     if PASSWORD:
//         s.sendall(f"PASS {PASSWORD}\r\n".encode())
//     s.sendall(f"NICK {nick}\r\n".encode())
//     s.sendall(f"USER {user} 0 * :{user}\r\n".encode())
//     time.sleep(0.2)
//     return recv_all(s)


// def test_fragmented_command():
//     print("\n[Test] Fragmented command across multiple sends (mirrors nc Ctrl+D)")
//     s = connect()
//     register(s, "fragnick", "fraguser")
//     for piece in (b"PI", b"NG hel", b"lo\r\n"):
//         s.sendall(piece)
//         time.sleep(0.3)
//     print("  Response:", recv_all(s, timeout=2))
//     s.close()


// def test_multiple_commands_one_packet():
//     print("\n[Test] Multiple full commands in a single packet (pipelining)")
//     s = connect()
//     register(s, "pipenick", "pipeuser")
//     s.sendall(b"JOIN #test\r\nPRIVMSG #test :hello\r\nPART #test\r\n")
//     print("  Response:", recv_all(s, timeout=2))
//     s.close()


// def test_split_crlf():
//     print("\n[Test] CRLF terminator split across two sends")
//     s = connect()
//     register(s, "crlfnick", "crlfuser")
//     s.sendall(b"JOIN #test\r")
//     time.sleep(0.3)
//     s.sendall(b"\n")
//     print("  Response:", recv_all(s, timeout=2))
//     s.close()


// def test_low_bandwidth():
//     print("\n[Test] Byte-by-byte 'low bandwidth' simulation")
//     s = connect()
//     register(s, "slownick", "slowuser")
//     for b in b"PRIVMSG #test :slow message\r\n":
//         s.sendall(bytes([b]))
//         time.sleep(0.05)
//     print("  Response:", recv_all(s, timeout=3))
//     s.close()


// def test_oversized_line():
//     print("\n[Test] Very long line with no terminator (buffer growth / DoS check)")
//     s = connect()
//     register(s, "bignick", "biguser")
//     s.sendall(b"A" * 100000)
//     time.sleep(0.5)
//     try:
//         resp = recv_all(s, timeout=1)
//         print("  Response (expect empty, error, or disconnect):", resp[:200])
//     finally:
//         try:
//             s.sendall(b"\r\n")
//         except OSError:
//             pass
//         s.close()


// def test_abrupt_disconnect():
//     print("\n[Test] Abrupt disconnect mid-command")
//     s = connect()
//     register(s, "dropnick", "dropuser")
//     s.sendall(b"PRIVMSG #test :goodbye cruel wor")
//     s.close()  # no \r\n ever sent
//     print("  Closed mid-line. Check server didn't crash/hang.")


// def test_empty_lines_and_whitespace():
//     print("\n[Test] Empty lines / repeated CRLFs")
//     s = connect()
//     register(s, "wsnick", "wsuser")
//     s.sendall(b"\r\n\r\n   \r\nJOIN #test\r\n\r\n")
//     print("  Response:", recv_all(s, timeout=2))
//     s.close()


// def test_concurrent_clients(n=20):
//     print(f"\n[Test] {n} concurrent clients joining/talking at once")

//     def worker(i):
//         try:
//             s = connect()
//             register(s, f"user{i}", f"user{i}")
//             s.sendall(f"JOIN #stress\r\nPRIVMSG #stress :hi from {i}\r\n".encode())
//             recv_all(s, timeout=2)
//             s.close()
//         except Exception as e:
//             print(f"  client {i} error: {e}")

//     threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
//     for t in threads:
//         t.start()
//     for t in threads:
//         t.join()
//     print("  Done — check server stayed alive and handled all clients.")


// if __name__ == "__main__":
//     test_fragmented_command()
//     test_multiple_commands_one_packet()
//     test_split_crlf()
//     test_low_bandwidth()
//     test_oversized_line()
//     test_abrupt_disconnect()
//     test_empty_lines_and_whitespace()
//     test_concurrent_clients()
//     print("\nAll tests dispatched. Check your server's terminal for crashes, "
//           "hangs, or incorrectly parsed commands.")
