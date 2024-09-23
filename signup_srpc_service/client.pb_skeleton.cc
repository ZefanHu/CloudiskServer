// signup_srpc_service/client.pb_skeleton.cc

#include "user.srpc.h"			   // 包含生成的sRPC头文件，定义了UserService客户端接口
#include "workflow/WFFacilities.h" // 包含Workflow库的设施，如WaitGroup，用于同步等待

using namespace srpc; // 使用sRPC命名空间，简化代码书写

// 创建一个WaitGroup对象，用于等待异步RPC调用完成
static WFFacilities::WaitGroup wait_group(1);

// 信号处理函数，用于在接收到终止信号时结束WaitGroup的等待
void sig_handler(int signo)
{
	wait_group.done(); // 标记WaitGroup任务完成，解除等待
}

// 回调函数，当RPC调用完成时被调用
static void signup_done(RespSignup *response, srpc::RPCContext *context)
{
	// 当该函数被执行时，已经接收到来自服务器的RespSignup响应数据
	int code = response->code();				// 获取响应中的状态码
	std::string msg = response->msg();			// 获取响应中的消息
	std::cout << "code: " << code << std::endl; // 输出状态码
	std::cout << "msg: " << msg << std::endl;	// 输出消息
}

int main()
{
	// 初始化Google Protocol Buffers库，确保版本兼容
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	// 定义sRPC服务的IP地址和端口号
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;

	// 创建UserService的sRPC客户端对象，连接到指定IP和端口的服务器
	UserService::SRPCClient client(ip, port);

	// 创建并设置注册请求消息
	ReqSignup signup_req;
	signup_req.set_username("admin"); // 设置用户名
	signup_req.set_password("admin"); // 设置密码

	// 发起Signup RPC调用，传入请求消息和回调函数
	client.Signup(&signup_req, signup_done);

	// 等待回调函数执行完成，确保客户端在收到响应前不会退出
	wait_group.wait();

	// 关闭Protocol Buffers库，释放相关资源
	google::protobuf::ShutdownProtobufLibrary();
	return 0; // 程序结束
}
