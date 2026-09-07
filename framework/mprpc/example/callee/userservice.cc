#include "../user.pb.h"
#include <string>
#include <iostream>
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <AsyncLog/AsyncLogging.h>
#include <AsyncLog/LogFile.h>
#include <AsyncLog/Logger.h>
class UserService : public userspace::UserServiceRpc
{
public:
    bool login(std::string name,std::string pwd){
        std::cout <<"do local service:login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id: " << id << " name: " << name << " pwd:" << pwd << std::endl;
        return false;
    }


    //重写UserServiceRpc::Login()   框架直接调用
    void Login(::google::protobuf::RpcController* controller,
                       const ::userspace::LoginRequest* request,
                       ::userspace::LoginResponse* response,
                       ::google::protobuf::Closure* done) override 
    {

        // 1. 解析业务上报的请求参数
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 2. 调用本地业务
        bool ret = login(name,pwd);
       
        // 3. 封装响应消息
        userspace::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("登录成功");

        response->set_sucess(ret);

        // 4. 执行回调操作  执行响应对象数据的序列化和网络发送（都是由框架来完成的）

        done->Run();


    }


    void Register(::google::protobuf::RpcController* controller,
                       const ::userspace::RegisterRequest* request,
                       ::userspace::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_sucess(ret);

        done->Run();
    }
private:

};

// 定义唯一的 AsyncLogging 实例（通常用智能指针或全局指针）
AsyncLogging* g_asyncLog = nullptr;

// 定义桥接函数：Logger 产生日志后，回调这个函数
void asyncOutput(const char* msg, int len) {
    if (g_asyncLog) {
        g_asyncLog->append(msg, len);
    }
}



int main(int argc, char **argv){

  // 初始化日志系统，并启动后端日志线程
    // 设置日志等级
    Logger::setLogLevel(Logger::LogLevel::DEBUG);

    // 初始化后端日志系统  名称，刷盘时间
    AsyncLogging log("MyServer", 50); 
    
    // 保存全局指针，供 asyncOutput 使用
    g_asyncLog = &log;

    // 设置 Logger 的全局输出回调
    Logger::setOutput(asyncOutput);
    
    // 启动后端日志线程
    log.start();

  // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;

}