#include "mprpcapplication.h"
#include "iostream"
#include "unistd.h"
#include <AsyncLog/Logger.h>



MprpcConfig MprpcApplication::config_;//静态变量类外初始化

// ./userProvider -i configfile
void MprpcApplication::Init(int argc, char** argv)
{
    if(argc < 2)
    {
        std::cout<<"format: command -i <configfile>" << std::endl;

        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) != -1){
        switch (c)
        {
        case 'i':
            config_file = optarg;
            /* code */
            break;
        case '?':
            std::cout<<"format: command -i <configfile>" << std::endl;
            // break;
            exit(EXIT_FAILURE);
        case ':':
            std::cout<<"format: command -i <configfile>" << std::endl;
            exit(EXIT_FAILURE);
        default:
            break;

        }
    }

    //

    // std::cout << "config_file:" << config_file << std::endl;
    LOG_INFO << "start load configFile: " << config_file.c_str();
    
    // 加载配置文件
    config_.loadConfigFile(config_file);
    LOG_INFO << "rpcserverip: " <<  config_.getConfigVal("rpcserverip").c_str();
    LOG_INFO << "rpcserverport: " <<  config_.getConfigVal("rpcserverport").c_str();
    LOG_INFO << "zookeeperip: " <<  config_.getConfigVal("zookeeperip").c_str();
    LOG_INFO << "zookeeperport: " <<  config_.getConfigVal("zookeeperport").c_str();


}
MprpcApplication& MprpcApplication::getInstance()
{
    static MprpcApplication mprpcapp;
    return mprpcapp;
}


MprpcConfig& MprpcApplication::getConfig()
{
    return config_;
}