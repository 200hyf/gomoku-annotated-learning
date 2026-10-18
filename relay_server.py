# ============================================================
#  五子棋注释学习版 —— 中转服务器（异地联机用）
#  作用：两台电脑不在同一局域网时，帮他们把数据互相转发
#  部署：拿到有公网 IP 的服务器上，运行  python relay_server.py
#  注意：服务器安全组/防火墙要放行 TCP 5000 端口（入站）
# ============================================================
import socket          # 网络编程（和 C++ 的 Winsock 是同一套 TCP 协议）
import threading       # 多线程：一个玩家一个线程，互不阻塞
import queue           # 线程安全的队列：每个玩家一个"收件箱"

PORT = 5000            # 监听端口，和游戏里 NET_PORT 一致（游戏里也可改，两边对上就行）

# ---- 全局数据（多线程共享，改它要先拿 lock） ----
clients = []           # 玩家连接列表：clients[0] = 先连的玩家（当主机，黑先）
                       #                 clients[1] = 后连的玩家（当客户端，白后）
queues = {}            # 连接 -> 它的"收件箱"（队列）。读线程把收到的数据放进来
stopped = {}           # 连接 -> 是否已断开（True = 断了）
lock = threading.Lock()


def reader(conn):
    """【唯一消费者】读自己连接的数据 → 放进自己的收件箱；断开则通知对方关闭"""
    while True:
        try:
            data = conn.recv(4096)        # 阻塞读（有数据就返回）
        except OSError:
            break                          # 连接异常 → 当断开
        if not data:
            break                          # 收到空 = 对方主动关闭
        queues[conn].put(data)             # 放进收件箱（等转发线程来取）
    # 自己这端断了：标记 + 主动关掉对方连接（让对方的 reader 也退出）
    stopped[conn] = True
    with lock:
        for other in clients:
            if other is not conn:
                try:
                    other.shutdown(socket.SHUT_RDWR)
                    other.close()
                except OSError:
                    pass


def pump(src, dst):
    """转发线程：把 src 收件箱里的数据，一条条原样发给 dst"""
    while True:
        try:
            data = queues[src].get(timeout=1)   # 从收件箱取一包（1 秒没有就返回）
        except queue.Empty:
            if stopped.get(src):
                break          # src 已断开且队列清空 → 转发结束
            continue            # 还没断，继续等下一包
        try:
            dst.sendall(data)   # 原样转发给另一个玩家
        except OSError:
            break                # 对方已断，转发结束


def handle(conn, addr):
    """处理一个玩家连接：发身份包 → 启动读线程 → 两人齐了再启动转发 → 断开后清理"""
    global clients
    with lock:
        if len(clients) >= 2:            # 已经有两位玩家了 → 拒绝后来者
            conn.close()
            return
        clients.append(conn)
        role = 1 if len(clients) == 1 else 2   # 第 1 个 = 主机(黑)  第 2 个 = 客户端(白)
    print(f"[+] {addr} 加入，身份={'主机(黑先)' if role == 1 else '客户端(白后)'}（{len(clients)}/2）")

    # 告诉游戏它的身份：发 4 字节消息 {op=7, col=身份, row=0, pad=0}
    # 和游戏里 NetMsg 结构完全一致，游戏收到 op==7 就知道配对成功 + 自己是黑还是白
    conn.sendall(bytes([7, role, 0, 0]))

    queues[conn] = queue.Queue()         # 给这个连接准备一个收件箱
    stopped[conn] = False
    my_reader = threading.Thread(target=reader, args=(conn,), daemon=True)
    my_reader.start()                    # 启动读线程（唯一 recv 消费者）

    if len(clients) == 2:                # 两人齐了 → 启动双向转发
        a, b = clients                   # a=主机连接  b=客户端连接
        print("[*] 配对成功！开始转发数据")
        threading.Thread(target=pump, args=(a, b), daemon=True).start()   # 主机→客户端
        threading.Thread(target=pump, args=(b, a), daemon=True).start()   # 客户端→主机

    # 等自己的读线程结束 = 自己这端断开了 → 清理列表
    # ⚠ 注意：绝不能在这里再 recv(conn)！TCP 数据"谁先收归谁"，
    #   多个线程同时 recv 同一个 socket 会互相抢数据 → 转发丢失。
    #   收数据只归 reader 线程，这里只用 join 等待断开。
    my_reader.join()

    with lock:
        if conn in clients:
            clients.remove(conn)
    print(f"[-] {addr} 断开，当前 {len(clients)}/2")


def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)   # TCP socket
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # 重启不报"端口被占用"
    srv.bind(('0.0.0.0', PORT))          # 0.0.0.0 = 监听所有网卡（公网IP、内网IP都收）
    srv.listen(2)                        # 最多排两个连接
    print(f"[*] 中转服务器已启动，端口 {PORT}，等待两个玩家…")
    while True:
        conn, addr = srv.accept()        # 等玩家连进来
        threading.Thread(target=handle, args=(conn, addr), daemon=True).start()


if __name__ == '__main__':
    main()
