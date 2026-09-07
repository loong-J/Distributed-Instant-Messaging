#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <IP> <Port> <Count>\n", argv[0]);
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int count = atoi(argv[3]);

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    std::vector<int> sockets;
    sockets.reserve(count);

    printf("Start connecting to %s:%d, target: %d connections...\n", ip, port, count);

    for (int i = 0; i < count; ++i) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket error");
            break;
        }

        // 简单的阻塞 connect 即可，2万连接
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("connect error");
            // 出现“Cannot assign requested address”说明端口耗尽
            // 出现“Connection refused”说明服务器挂了或backlog满了
            close(sockfd);
            break;
        }

        sockets.push_back(sockfd);

        // 每连 1000 个打印一下进度，稍作休息防止瞬间压垮 Server 的 Accept 队列
        if ((i + 1) % 1000 == 0) {
            printf("Created %d connections...\n", i + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
        }
    }

    printf("Done! Established %lu connections.\n", sockets.size());
    printf("Press Ctrl+C to exit...\n");

    // 死循环，保持连接不释放
    while (true) {
        sleep(10);
    }

    return 0;
}

// g++ -o c20k c20k.cc -std=c++11