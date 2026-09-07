#pragma once

#include "mprpcconfig.h"
//框架基础类，负责框架的初始化操作,单例
class MprpcApplication{
public:
    static void Init(int argc, char** argv);
    static MprpcApplication& getInstance();
    
    static MprpcConfig& getConfig();

private:
    static MprpcConfig config_;
    //私有化构造函数，删除拷贝和移送构造函数
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
};