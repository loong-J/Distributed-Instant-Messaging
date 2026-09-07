#include "google/protobuf/service.h"

#include <MyMuduo/TcpServer.h>
#include <MyMuduo/TcpConnection.h>
#include <MyMuduo/EventLoop.h>

#include <unordered_map>
class RpcProvider
{

public:
    RpcProvider(){}
    ~RpcProvider(){}
    void NotifyService(google::protobuf::Service* service); // 提供给外部使用，发布rpc方法的函数接口
    void Run(); // 启动rpc服务节点 ，开始提供rpc远程网络调用，接收远程调用请求


private:
    EventLoop eventLoop_;

    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);

    void sendRpcResponse(const TcpConnectionPtr&,
                        google::protobuf::Message *);

    struct ServiceInfo
    {
        google::protobuf::Service* service_; // 服务
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> methodMap_; //  方法名 --->  方法名
    };
    
    //  服务名 --->  服务信息
    std::unordered_map<std::string, ServiceInfo> serviceMap_;
};

