#include <cstdint>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "0.0.0.0");
    explicit InetAddress(const sockaddr_in &addr);
    std::string toIp() const;
    uint16_t toPort() const;
    std::string toIpPort() const; // "1270.0.0.1:8456"

    const sockaddr_in* getSockAddr() const {return &addr_;}
    
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;

};