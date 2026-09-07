// /**
//  * Raw Epoll Benchmark Client
//  * ===========测试吞吐量===============
//  * 不依赖 muduo，只使用 Linux 原生 API
//  */
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/epoll.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <vector>
// #include <sys/time.h>
// #include <errno.h>

// #define MAX_EVENTS 1024
// #define BLOCK_SIZE 16384  // 16KB per block

// // 统计数据
// long long g_totalBytesRead = 0;
// long long g_totalMsgCount = 0;

// // 设置 Socket 为非阻塞
// int setNonBlocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     if (flags == -1) return -1;
//     return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

// // 获取当前时间（秒，double）
// double getNow() {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     return tv.tv_sec + tv.tv_usec / 1000000.0;
// }

// int main(int argc, char* argv[]) {
//     if (argc < 5) {
//         fprintf(stderr, "Usage: %s <IP> <Port> <Connections> <DurationSeconds>\n", argv[0]);
//         return 1;
//     }

//     const char* serverIp = argv[1];
//     int port = atoi(argv[2]);

//     int connections = atoi(argv[3]); // 连接数量
//     int duration = atoi(argv[4]);  // 测试时间

//     // 1. 创建 Epoll
//     int epollfd = epoll_create1(0);
//     if (epollfd == -1) { perror("epoll_create1"); return 1; }

//     struct sockaddr_in serverAddr;
//     memset(&serverAddr, 0, sizeof(serverAddr));
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_port = htons(port);
//     inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

//     // 准备发送的数据 (16KB)
//     char sendBuf[BLOCK_SIZE];
//     memset(sendBuf, 'X', BLOCK_SIZE);

//     // 2. 发起连接
//     int connectedCount = 0;
//     for (int i = 0; i < connections; ++i) {
//         int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//         if (sockfd < 0) { perror("socket"); continue; }

//         setNonBlocking(sockfd);

//         struct epoll_event ev;
//         ev.events = EPOLLIN | EPOLLOUT | EPOLLET; // 边缘触发
//         ev.data.fd = sockfd;
//         epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
//         // 非阻塞IO Connect 
//         // 1. 如果是秒连（ret==0），直接算成功。
//         // 2. 如果是正在连（EINPROGRESS），交给后面的 Epoll 去监听结果（这是最常见的情况）。
//         // 3. 如果是真报错，打印错误并关闭 socket。
//         int ret = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
//         if (ret == 0) {
//             // 立即连接成功（极少见）
//             connectedCount++;
//         } else if (errno != EINPROGRESS) {
//             perror("connect");
//             close(sockfd);
//         }
//         // 如果 EINPROGRESS，等待 EPOLLOUT
//     }
//     printf("Initiated %d connections...\n", connections);

//     struct epoll_event events[MAX_EVENTS];
//     char readBuf[BLOCK_SIZE];

//     double startTime = getNow();
//     double endTime = startTime + duration;
//     bool running = true;

//     // 3. 事件循环
//     while (running) {
//         double now = getNow();
//         if (now >= endTime) break;

//         int nfds = epoll_wait(epollfd, events, MAX_EVENTS, 100); // 100ms timeout

//         for (int i = 0; i < nfds; ++i) {
//             int sockfd = events[i].data.fd;
//             uint32_t evs = events[i].events;

//             if (evs & (EPOLLERR | EPOLLHUP)) {
//                 close(sockfd);
//                 continue;
//             }

//             // 处理可写事件 (连接建立完成 或 此时可发送数据)
//             if (evs & EPOLLOUT) {
//                 int error = 0;
//                 socklen_t len = sizeof(error);
//                 // 检查连接状态
//                 if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
//                     // 连接失败，忽略或重连
//                     continue; 
//                 }

//                 // 连接成功，发送第一个 Ping 包，启动 Ping-Pong
//                 // 注意：这里为了简化代码，假设 connection 刚建立时是第一次 EPOLLOUT
//                 // 实际上应该用一个 map 记录每个 fd 的状态
//                 // 但在这个特定的 PingPong 逻辑里，只要可写就尝试发一次(如果逻辑复杂需要状态机)
                
//                 // 这里采用简单策略：只有当我们从 socket 读不到数据时（刚连接），才主动发
//                 // 但 epoll ET 模式下 EPOLLOUT 可能会一直触发。
//                 // 更好的方式：利用 EPOLLIN 驱动。
                
//                 // 简单处理：如果是第一次连接成功，直接发送一包数据
//                 // 我们可以利用 struct epoll_event.data.ptr 来存状态，这里为了代码极简，
//                 // 我们仅仅在 EPOLLOUT 时发送数据，如果没有 pending read。
//                 // *但在 PingPong 模型中，通常是 Read 触发 Write。*
//                 // *这里仅用于 Kick off (启动)*
                
//                 // Hack: 发送一次启动包，然后把 EPOLLOUT 去掉，只监听 EPOLLIN
//                 int n = write(sockfd, sendBuf, BLOCK_SIZE);
//                 if (n > 0) {
//                     struct epoll_event ev;
//                     ev.events = EPOLLIN | EPOLLET; // 之后只监听读，读到了再写
//                     ev.data.fd = sockfd;
//                     epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev);
//                 }
//             }

//             // 处理可读事件 (服务端回显了数据)
//             if (evs & EPOLLIN) {
//                 while (true) {
//                     int n = read(sockfd, readBuf, sizeof(readBuf));
//                     if (n < 0) {
//                         if (errno == EAGAIN || errno == EWOULDBLOCK) break;
//                         close(sockfd); // 出错
//                         break;
//                     } else if (n == 0) {
//                         close(sockfd); // 对端关闭
//                         break;
//                     } else {
//                         // 读到了 n 字节
//                         g_totalBytesRead += n;
//                         g_totalMsgCount++;

//                         // Ping-Pong: 收到多少，立马发回多少
//                         // 注意：严谨的做法需要处理 write 的返回值（可能只发了一部分）
//                         // 但在本地压测场景下，内核缓冲区通常足够大，这里做简化处理
//                         int written = write(sockfd, readBuf, n);
//                         (void)written; // 消除警告
//                     }
//                 }
//             }
//         }
//     }

//     double realDuration = getNow() - startTime;
//     double mbps = (double)g_totalBytesRead / (1024 * 1024) / realDuration;

//     printf("\n=== Benchmark Result ===\n");
//     printf("Duration:       %.3f s\n", realDuration);
//     printf("Total Reads:    %lld\n", g_totalMsgCount);
//     printf("Total Bytes:    %lld bytes\n", g_totalBytesRead);
//     printf("Throughput:     %.2f MiB/s\n", mbps);
//     printf("========================\n");

//     close(epollfd);
//     return 0;
// }



// // g++ -O3 -o client client.cc










