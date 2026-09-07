#pragma once

#include <string>
#include <unordered_map>
class MprpcConfig
{


public:
    void loadConfigFile(std::string &config_file);
    std::string getConfigVal(const std::string &key);


private:

    std::unordered_map<std::string, std::string > config_map;



};