#include <zookeeper/zookeeper.h>
#include <string>
class ZkClient{


public:
    ZkClient();
    ~ZkClient();
    void Start();// 连接Zkserver

    void Create(const char* path, const char* data, int dataLen, int state=0);// 创建节点
    std::string GetData(const char* path);
private:
    zhandle_t * zhandle_;// 客户端句柄
};