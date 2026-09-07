# framework

## AsyncLog

双缓冲异步日志。业务线程写入内存缓冲，后台线程负责批量刷盘，避免网络 IO
线程被磁盘操作阻塞。内部包含整数转字符串查表法实现与压力测试代码。

## MyMuduo

参考 Muduo 实现的网络库：epoll + 非阻塞 IO、主从 Reactor、eventfd 跨线程
唤醒、One Loop Per Thread。

## mprpc

基于 Protobuf Service 的 RPC 层：

- `MprpcChannel`：客户端调用入口，负责序列化请求并调用远程方法。
- `RpcProvider`：服务端发布与请求分发。
- `ZkClient`：ZooKeeper 临时节点注册与读取。
- `rpcheader.proto`：RPC 请求头定义。


