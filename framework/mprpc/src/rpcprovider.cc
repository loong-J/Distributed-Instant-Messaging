#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"

#include "zkclient.h"

#include <string>
#include <functional>
#include <AsyncLog/Logger.h>
#include <google/protobuf/descriptor.h>



// 提供给外部使用，发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo serviceInfo;
    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *servicesDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string serviceName = servicesDesc->name();
    int methodCount = servicesDesc->method_count();

    for (int i = 0; i < methodCount; i++)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述）
        const google::protobuf::MethodDescriptor *methodDesc = servicesDesc->method(i);
        // 获取方法的名字
        std::string methodName = methodDesc->name();
        serviceInfo.methodMap_.insert({methodName, methodDesc});
    }

    serviceInfo.service_ = service;
    serviceMap_.insert({serviceName, serviceInfo});
}

void RpcProvider::Run()
{
    // 读取配置信息
    MprpcConfig config_ = MprpcApplication::getConfig();
    // std::string ip = config_.getConfigVal("rpcserverip");
    std::string ip = "0.0.0.0";
    uint16_t port = atoi(config_.getConfigVal("rpcserverport").c_str());

    // 监听
    InetAddress listenAddr(port, ip);

    // 创建
    TcpServer server(&eventLoop_, listenAddr, "RpcServer");

    // 设置连接回调和消息回调
    server.setConnectionCallback(std::bind(&RpcProvider::onConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::onMessage, this, std::placeholders::_1,
                                        std::placeholders::_2, std::placeholders::_3));

    // 开启多线程模型
    server.setThreadNum(3);


    ZkClient zkCli;
    //  连接zkserver
    zkCli.Start();

    for(auto &sp : serviceMap_)
    {
        std::string serivepath = "/" + sp.first;
        zkCli.Create(serivepath.c_str(), nullptr, 0, 0);//持久节点

        for(auto &mp : sp.second.methodMap_)
        {
            std::string nodePath = serivepath + "/" + mp.first;
            // 存储的数据
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);

            zkCli.Create(nodePath.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL); // 创建临时节点
        }


    }









    // 启动server
    server.start();

    // 开始事件循环
    eventLoop_.loop();
}

void RpcProvider::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected())
    {
        LOG_INFO <<" new Connection: " << conn->peerAddress().toIpPort().c_str() << " --> " <<  conn->localAddress().toIpPort().c_str();
    }
    else
    {
        LOG_INFO <<"Connection disConnect:" << conn->peerAddress().toIpPort().c_str() << " --> " <<  conn->localAddress().toIpPort().c_str();
        conn->shutdown();
    }
}


void RpcProvider::onMessage(const TcpConnectionPtr &conn,
                            Buffer *buffer,
                            Timestamp time)
{
    // 0. 循环读取，处理半包问题    （虽然短连接粘包发生的概率小）
    // （循环是为了粘包，而内部的长度检查是为了半包。）
    while (buffer->readableBytes() >= 4)
    {
        // --- 步骤 1: 读取 header_size ---
        // 使用 peek() 只是查看，不移动 readerIndex
        const char *data = buffer->peek();
        
        // 读取前4个字节得到 header_size
        uint32_t header_size = 0;
        // 更加安全的二进制拷贝方式
        std::copy(data, data + 4, reinterpret_cast<char*>(&header_size));

        // --- 步骤 2: 检查 RpcHeader 是否全部到达 ---
        // 此时需要 buffer 长度 >= 4 (长度头) + header_size
        if (buffer->readableBytes() < 4 + header_size)
        {
            // 数据头还没收全，退出循环，等待下一次 onMessage
            break; 
        }

        // --- 步骤 3: 预解析 Header 以获取 args_size ---
        // 注意：这里我们还不能 retrieve(取走) 数据，因为如果 Args 没到齐，
        // 我们需要把数据留在 buffer 里等下次拼接。
        
        // 既然 header 已经都在 buffer 里了，我们将其拷贝出来尝试解析
        // 指针偏移 4 字节，读取 header_size 长度
        std::string header_str(data + 4, header_size);
        
        mrpc::RpcHeader rpcHeader;
        std::string service_name;
        std::string method_name;
        uint32_t args_size = 0;

        if (rpcHeader.ParseFromString(header_str))
        {
            service_name = rpcHeader.service_name();
            method_name = rpcHeader.method_name();
            args_size = rpcHeader.args_size();
        }
        else
        {
            // LOGERR("rpc_msg_str: %s parse error!",header_str.c_str());
            conn->shutdown();
            return;
        }

        // --- 步骤 4: 检查 Args 是否全部到达 ---
        // 总长度 = 4 (长度头) + header_size (Header) + args_size (Args)
        if (buffer->readableBytes() < 4 + header_size + args_size)
        {
            // 参数部分还没收全，退出循环，等待下一次 onMessage
            break;
        }

        // ===========================================
        // 走到这里，说明 buffer 中必然包含了一个完整的包
        // ===========================================

        // 1. 划过前4字节
        buffer->retrieve(4);
        // 2. 划过 header
        buffer->retrieve(header_size);
        // 3. 读取 args 数据 (retrieveAsString 会自动移动 buffer 指针)
        std::string arg_msg = buffer->retrieveAsString(args_size);

        // --- 此时 header_str 和 arg_msg 都准备好了，开始业务逻辑 ---
        
        // 查找 Service
        auto it = serviceMap_.find(service_name);
        if (it == serviceMap_.end())
        {
            LOG_INFO << service_name.c_str() << "is not exist!";
  
            return;
        }

        ServiceInfo serviceInfo = it->second;
        auto mit = serviceInfo.methodMap_.find(method_name);
        if (mit == serviceInfo.methodMap_.end())
        {
            LOG_INFO << service_name.c_str() << " : " << method_name.c_str()  << "is not exist!";
          
            return;
        }

        google::protobuf::Service *service = serviceInfo.service_;
        const google::protobuf::MethodDescriptor *methodDesc = mit->second;

        // 创建请求对象
        google::protobuf::Message *request = service->GetRequestPrototype(methodDesc).New();

        // 反序列化请求参数
        if (!request->ParseFromString(arg_msg))
        {
            LOG_ERROR <<" request parse error, content: " <<  arg_msg.c_str() ;
  
            return;
        }

        // 创建响应对象
        google::protobuf::Message *response = service->GetResponsePrototype(methodDesc).New();

        // 绑定回调
        // 注意：SendRpcResponse 回调中应该包含 conn->shutdown() 来实现短连接
        google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                        const TcpConnectionPtr &,
                                                                        google::protobuf::Message *>(
            this,
            &RpcProvider::sendRpcResponse,
            conn, response);

        // 执行业务
        service->CallMethod(methodDesc, nullptr, request, response, done);
        
    
    }
}

 void RpcProvider::sendRpcResponse(const TcpConnectionPtr& conn,
                        google::protobuf::Message * response)
{
    std::string response_str;
    // 序列化
    if(response->SerializeToString(&response_str))
    {
        conn->send(response_str);
    }
    else
    {
        LOG_ERROR << "serialize response_str error!";
    }
    conn->shutdown(); // 短链接
}