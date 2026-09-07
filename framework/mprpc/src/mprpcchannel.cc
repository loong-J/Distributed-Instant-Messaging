#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <AsyncLog/Logger.h>
#include <unistd.h>
#include <errno.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"

#include "zkclient.h"
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* methodDesc,
                        google::protobuf:: RpcController* controller,
                        const google::protobuf::Message* request,
                        google::protobuf::Message* response,
                        google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor* sd = methodDesc->service();
    std::string service_name = sd->name(); // service_name
    std::string method_name = methodDesc->name(); // method_name
  // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }

     // 定义rpc的请求header
    mrpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }
    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4)); // header_size
    send_rpc_str += rpc_header_str; // rpcheader
    send_rpc_str += args_str; // args

    LOG_INFO << "Call " << service_name.c_str() <<" Service "<< method_name.c_str() <<" Method, header_size:"
    <<  header_size << " rpc_header_str: " << rpc_header_str.c_str() <<" , args_str: " << args_str.c_str();

     // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        LOG_ERROR << "create socket error! errno: " << errno;
        return;
    }



    ZkClient zkCli;
    //  连接zkserver
    zkCli.Start();

    std::string ipPortUrl = "/" + service_name + "/" + method_name; 

    std::string ipPort = zkCli.GetData(ipPortUrl.c_str());

    int equalPos = ipPort.find(':');
    std::string ip = ipPort.substr(0,equalPos);
    uint16_t port = atoi(ipPort.substr(equalPos+1).c_str());



    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // 转换为网络字节序
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        LOG_ERROR << "connect error! errno:  " << errno;
   
        return;
    }

     // 发送rpc请求
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        LOG_ERROR << "send error! errno: " << errno;

        return;
    }


     // 接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        
        
        LOG_ERROR << "recv error! errno: " << errno;

        return;
    }

    // 反序列化rpc调用的响应数据
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        close(clientfd);
        char errtxt[1060] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);

        LOG_ERROR << "parse error! response_str: " << recv_buf;
        return;
    }

    close(clientfd);


}