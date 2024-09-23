// signup_srpc_service/server.pb_skeleton.cc

// TODO cryto
#include "unistd.h"				   // 包含UNIX标准函数定义
#include "user.srpc.h"			   // 包含生成的sRPC头文件，定义了UserService服务接口
#include "workflow/WFFacilities.h" // 包含Workflow库的设施，如WaitGroup，用于同步等待

// TODO
#include <workflow/MySQLMessage.h> // 包含Workflow的MySQL消息处理类
#include <workflow/MySQLResult.h>  // 包含Workflow的MySQL结果处理类
// TODO
#include <ppconsul/ppconsul.h> // 包含ppconsul库，用于与Consul服务注册中心交互

using namespace srpc; // 使用sRPC命名空间，简化代码书写
using std::cout;
using std::endl;
using std::string;

// 创建一个WaitGroup对象，用于等待异步任务完成
static WFFacilities::WaitGroup wait_group(1);

// 信号处理函数，用于在接收到终止信号时结束WaitGroup的等待
void sig_handler(int signo)
{
	wait_group.done(); // 标记WaitGroup任务完成，解除等待
}

// UserServiceServiceImpl类实现了UserService::Service接口，定义了具体的服务逻辑
class UserServiceServiceImpl : public UserService::Service
{
public:
	// 重写Signup方法，用于处理用户注册请求
	void Signup(ReqSignup *request, RespSignup *response, srpc::RPCContext *ctx) override
	{
		// 解析请求消息，获取用户名和密码
		std::string username = request->username();
		std::string password = request->password();

		// 对密码进行加密处理，这里使用了简单的crypt函数和固定盐值（实际应用中应使用更安全的加密方法）
		string salt("12345678");
		string encodedPasswd(crypt(password.c_str(), salt.c_str()));

		// 定义MySQL连接URL，包含用户名、密码和主机地址
		string mysqlurl("mysql://root:1234@localhost");

		// 创建一个MySQL异步任务，用于执行插入用户数据的SQL语句
		auto mysqlTask = WFTaskFactory::create_mysql_task(mysqlurl, 10, [response](WFMySQLTask *mysqltask)
														  {
            cout << "mysqlCallback is running" << endl; // 输出调试信息

            // 获取任务的状态和错误码
            int state = mysqltask->get_state();
            int error = mysqltask->get_error();
            cout << "state: " << state << endl;

            // 检查任务是否成功执行
            if (state != WFT_STATE_SUCCESS)
            {
                // 如果执行失败，输出错误信息并返回
                printf("error: %s\n", WFGlobal::get_error_string(state, error));
                return;
            }

            // 获取MySQL响应包
            auto mysqlresp = mysqltask->get_resp();
            if(mysqlresp->get_packet_type() == MYSQL_PACKET_ERROR){
                // 如果返回的是错误包，输出错误信息并返回
                printf("ERROR %d, %s\n", mysqlresp->get_error_code(), mysqlresp->get_error_msg().c_str());
                return;
            }

            // 使用MySQLResultCursor解析结果集
            protocol::MySQLResultCursor cursor(mysqltask->get_resp());

            // 检查插入操作是否影响了一行，表示成功
            if(cursor.get_cursor_status() == MYSQL_STATUS_OK && cursor.get_affected_rows() == 1){
                // 设置响应消息为成功
                response->set_code(0);
                response->set_msg("SUCCESS");
            }
            else
            {
                // 设置响应消息为失败
                response->set_code(-1);
                response->set_msg("failed");
            } });

		// 构造插入用户数据的SQL语句
		string sql("INSERT INTO tbl_sql.tbl_user(user_name, user_pwd, status) VALUES('");
		sql += username + "','" + encodedPasswd + "',0)";
		printf("sql:\n %s\n", sql.c_str()); // 输出SQL语句用于调试

		// 将SQL语句设置到MySQL任务请求中
		mysqlTask->get_req()->set_query(sql);

		// 将MySQL任务加入到调用上下文的任务序列中，异步执行
		ctx->get_series()->push_back(mysqlTask);
	}
};

// 定义一个定时回调函数，用于向Consul发送服务健康检查信号
void timerCallback(WFTimerTask *timerTask)
{
	printf("timerCallback is running\n"); // 输出调试信息

	using namespace ppconsul::agent;
	Agent *pagent = (Agent *)timerTask->user_data; // 获取Consul Agent对象

	pagent->servicePass("SignupService1"); // 发送服务通过信号，表示服务仍然健康

	// 创建下一个定时任务，重复发送心跳信号
	auto nextTask = WFTaskFactory::create_timer_task(5 * 1000 * 1000, timerCallback);
	nextTask->user_data = pagent;			   // 关联Consul Agent对象
	series_of(timerTask)->push_back(nextTask); // 将下一个定时任务加入到当前任务序列中
}

int main()
{
	// 初始化Google Protocol Buffers库，确保版本兼容
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	// 定义sRPC服务的端口号
	unsigned short port = 1412;

	// 创建SRPCServer对象
	SRPCServer server;

	// 创建UserService服务的实现实例
	UserServiceServiceImpl userservice_impl;

	// 将UserService服务添加到sRPC服务器
	server.add_service(&userservice_impl);

	// 启动sRPC服务器，监听指定端口
	server.start(port);

	// 使用ppconsul库与Consul服务注册中心通信
	using namespace ppconsul;
	Consul consul("127.0.0.1:8500", ppconsul::kw::dc = "dc1"); // 创建Consul客户端，指定数据中心为dc1
	agent::Agent agent(consul);								   // 创建Agent对象，用于注册服务

	// 注册SignupService1服务到Consul，包含服务名称、地址、端口和健康检查
	agent.registerService(
		agent::kw::name = "SignupService1",							 // 服务名称
		agent::kw::address = "127.0.0.1",							 // 服务地址
		agent::kw::id = "SignupService1",							 // 服务ID
		agent::kw::port = 1412,										 // 服务端口
		agent::kw::check = agent::TtlCheck(std::chrono::seconds(10)) // 设置TTL健康检查，10秒后需要发送心跳
	);

	// 发送服务通过信号，表示服务已启动且健康
	agent.servicePass("SignupService1");

	// 创建并启动定时任务，定期发送心跳信号至Consul
	auto timerTask = WFTaskFactory::create_timer_task(5 * 1000 * 1000, timerCallback); // 5秒后执行一次回调
	timerTask->user_data = &agent;													   // 关联Consul Agent对象
	timerTask->start();																   // 启动定时任务

	// 等待所有异步任务完成，确保服务器在收到终止信号前不会退出
	wait_group.wait();

	// 停止sRPC服务器
	server.stop();

	// 关闭Protocol Buffers库，释放相关资源
	google::protobuf::ShutdownProtobufLibrary();
	return 0; // 程序结束
}
