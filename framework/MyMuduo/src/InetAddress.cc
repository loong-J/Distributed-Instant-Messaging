#include "InetAddress.h"
#include <string.h>

InetAddress::InetAddress(uint16_t port , std::string ip )
{
    bzero(&addr_,sizeof addr_);

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); //转为网络字节
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 点分十进制转换为网络字节序的二进制数据

}
InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {}
std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);// 网络字节序的二进制数据转换为点分十进制

    return buf;
    
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}
std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}