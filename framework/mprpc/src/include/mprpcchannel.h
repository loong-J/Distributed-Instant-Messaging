#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
class MprpcChannel : public google::protobuf::RpcChannel
{

      virtual void CallMethod(const google::protobuf::MethodDescriptor* methodDesc,
                         google::protobuf:: RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done);
};