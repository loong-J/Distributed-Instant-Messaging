#include "mprpcconfig.h"
#include <iostream>
#include <fstream>
// #include "logger.h"

/**
 *  去除字符串首尾空白字符
 */
std::string trim(std::string str)
{
    size_t start = str.find_first_not_of(' ');  //找到第一个非空字符
    if (start == std::string::npos) {
        // 字符串全是空白或为空
        return "";
    }
    size_t end = str.find_last_not_of(' ');// 找到最后一个非空白字符的位置
    return str.substr(start,end-start+1);
}


void MprpcConfig::loadConfigFile(std::string& config_file)
{
    //创建文件输入流
    std::ifstream file(config_file);
    
    if(!file.is_open()){
        // LOGERR("open confile fail");
        exit(EXIT_FAILURE);
    }
    //逐行读取
    std::string line;
    while(std::getline(file,line))
    {
        //去掉字符串的前后空格
        std::string s=trim(line);
        // std::cout << "config:" << line << std::endl;
        if(s.empty() || s[0] == '#'){
            continue;
        }
        size_t eqPos = s.find('=');
        if(eqPos == std::string::npos){
            // LOGINFO("config is invaild!");
            continue;
        }
        //0123456789        18
        //rpcip   = 127.0.0.1  
        std::string configKey = s.substr(0,eqPos);
        configKey = trim(configKey);
        std::string configVal = s.substr(eqPos+1);
        configVal = trim(configVal);
        config_map.insert({configKey,configVal});
    }

    // file.close();//析构时会自动关闭文件 RAII

}
std::string MprpcConfig::getConfigVal(const std::string &key)
{
    auto it=config_map.find(key);
    if(it != config_map.end()){
        return config_map[key];
    }
    return "";
}


