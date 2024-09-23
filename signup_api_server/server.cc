#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "wfrest/HttpServer.h"
#include "workflow/WFFacilities.h"
#include "wfrest/json.hpp"
#include <workflow/MySQLResult.h>

#include "token.hpp"
#include "hash.hpp"
#include "oss.hpp"
#include "amqp.hpp"
#include "user.srpc.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using wfrest::APPLICATION_URLENCODED;
using wfrest::HttpReq;
using wfrest::HttpResp;
using wfrest::HttpServer;
using wfrest::MULTIPART_FORM_DATA;
using Json = nlohmann::json;

class Cloudisk
{
public:
    Cloudisk() : _httpserver(),
                 _waitGroup(1) {}
    void start(unsigned short port)
    {
        loadStaticResources();
        loadSignupModule();
        loadSigninModule();
        loadUserInfoModule();
        loadFileUploadModule();
        loadUserFileListModule();
        loadFileDownloadModule();

        if (_httpserver.track().start(port) == 0)
        {
            _httpserver.list_routes();
            _waitGroup.wait();
            _httpserver.stop();
        }
        else
        {
            fprintf(stderr, "Cannot start server.");
        }
    }

private:
    void loadStaticResources();
    void loadSignupModule();
    void loadSigninModule();
    void loadUserInfoModule();
    void loadFileUploadModule();
    void loadUserFileListModule();
    void loadFileDownloadModule();

private:
    HttpServer _httpserver;
    WFFacilities::WaitGroup _waitGroup;
    OssUploader _uploader;
    Publisher _publisher;
};

void Cloudisk::loadStaticResources()
{
    // 等待请求的响应
    _httpserver.GET("/user/signup", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/signup.html"); });

    _httpserver.GET("/static/view/signin.html", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/signin.html"); });

    _httpserver.GET("/static/view/home.html", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/home.html"); });

    _httpserver.GET("/static/img/avatar.jpeg", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/img/avatar.jpeg"); });

    _httpserver.GET("/static/js/auth.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/js/auth.js"); });

    _httpserver.GET("/static/view/index.html", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/index.html"); });

    _httpserver.GET("/file/upload", [](const HttpReq *, HttpResp *resp)
                    { resp->File("./static/view/index.html"); });

    _httpserver.GET("/file/upload/success", [](const HttpReq *, HttpResp *resp)
                    { resp->File("./static/view/home.html"); });
    // 配置文件路由
    _httpserver.GET("/file/upload_files/bootstrap.min.css", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/bootstrap.min.css"); });

    _httpserver.GET("/file/upload_files/fileinput.min.css", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/fileinput.min.css"); });

    _httpserver.GET("/file/upload_files/jquery-3.2.1.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/jquery-3.2.1.min.js"); });

    _httpserver.GET("/file/upload_files/piexif.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/theme.js"); });

    _httpserver.GET("/file/upload_files/sortable.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/sortable.min.js"); });

    _httpserver.GET("/file/upload_files/purify.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/purify.min.js"); });

    _httpserver.GET("/file/upload_files/popper.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/popper.min.js"); });

    _httpserver.GET("/file/upload_files/bootstrap.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/bootstrap.min.js"); });

    _httpserver.GET("/file/upload_files/fileinput.min.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/fileinput.min.js"); });

    _httpserver.GET("/file/upload_files/theme.js", [](const HttpReq *req, HttpResp *resp)
                    { resp->File("./static/view/upload_files/theme.js"); });
}

void Cloudisk::loadSignupModule()
{
    _httpserver.POST("/user/signup", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
                     {
        // 解析请求
        auto &formKV = req->form_kv();
        string username = formKV["username"];
        string password = formKV["password"];

        GOOGLE_PROTOBUF_VERIFY_VERSION;
        const char *ip = "127.0.0.1";
        unsigned short port = 1412;

        UserService::SRPCClient client(ip, port);
        ReqSignup signup_req;
        signup_req.set_username(username);
        signup_req.set_password(password);
        // 创建RPC任务, 并将其加入到序列中运行
        auto rpctask = client.create_Signup_task([resp](RespSignup *response, srpc::RPCContext *ctx) {
            if(ctx->success() && response->code() == 0) {
                resp->String("SUCCESS");
            } else {
                resp->String("Signup Failed");
            }
        });

        rpctask->serialize_input(&signup_req);
        series->push_back(rpctask); });
}

void Cloudisk::loadSigninModule()
{
    _httpserver.POST("/user/signin", [](const HttpReq *req, HttpResp *resp, SeriesWork *series)
                     {
        // 解析请求, 获取用户名和密码
        if(req->content_type() == APPLICATION_URLENCODED){
            auto &formKV = req->form_kv();
            string username = formKV["username"];
            string password = formKV["password"];
            cout << "username: " << username << endl;
            cout << "password: " << password << endl;
            // 对密码进行加密
            string salt = "12345678";
            string encodedPasswd = crypt(password.c_str(), "12345678");
            cout << "encodedPasswd: " << encodedPasswd << endl;
            // 访问 MySQL, 进行登录校验
            string url("mysql://root:1234@localhost");
            auto mysqlTask = WFTaskFactory::create_mysql_task(url, 1, [resp, encodedPasswd, username, salt, url, series](WFMySQLTask *task){
                // 对任务进行判断
                using namespace protocol;
                MySQLResultCursor cursor(task->get_resp());
                // 读取结果集
                vector<vector<protocol::MySQLCell>> rows;
                cursor.fetch_all(rows);
                if(rows[0][0].is_string()){
                    string result = rows[0][0].as_string();
                    if(result == encodedPasswd){
                        // 登录成功, 生成Token
                        Token token(username, salt);
                        string strToken = token.genToken();

                        // 将Token写入数据库
                        auto mysqlTask2 = WFTaskFactory::create_mysql_task(url, 1, nullptr);
                        string sql2 = "REPLACE INTO tbl_sql.tbl_user_token(user_name, user_token) VALUES('"
                        + username + "', '" + strToken + "');";
                        cout << "sql2: " << sql2 << endl;
                        mysqlTask2->get_req()->set_query(sql2);
                        series->push_back(mysqlTask2);

                        // 生成响应信息, 发送给客户端
                        Json respMsg;
                        Json data;
                        data["Token"] = strToken;
                        data["Username"] = username;
                        data["Location"] = "/static/view/home.html";
                        respMsg["data"] = data;
                        resp->String(respMsg.dump());
                    }
                    else
                    {
                        // 验证失败
                        printf("password error.\n");
                        resp->String("password error!");
                    }
                }
                else
                {
                    printf("server 500 error.\n");
                    resp->set_status_code("500");
                    resp->set_reason_phrase("Internet Service Error");
                }
                // 测试结果集
                printf("%ld rows in set.\n", rows.size());
            });
            string sql = "select user_pwd from tbl_sql.tbl_user where user_name='";
            sql += username + "' limit 1";
            cout << "sql:\n" << sql << endl;
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask);
        } });
}

void Cloudisk::loadUserInfoModule()
{
    _httpserver.GET("/user/info", [](const HttpReq *req, HttpResp *resp)
                    {
        //1. 解析请求
        string username = req->query("username");
        string token = req->query("token");
        printf("username: %s\n", username.c_str());
        printf("token: %s\n", token.c_str());
        //2. 校验token, 将请求中的token与数据库中的token比对, 如果正确就进行第三步, 错误直接响应Invalid token
        
        //3. 查询数据库，获取用户的注册时间
        string url("mysql://root:1234@localhost");
        string sql("select signup_at from tbl_sql.tbl_user where user_name = '");
        sql += username + "' limit 1;";
        cout << "sql:\n" << sql << endl;

        using namespace protocol;
        resp->MySQL(url, sql, [resp, username](MySQLResultCursor * pcursor){
            using std::vector;
            vector<vector<MySQLCell>> rows;
            pcursor->fetch_all(rows);

            if(rows[0][0].is_datetime()) {
                Json respMsg;
                Json data;
                data["Username"] = username;
                data["SignupAt"] = rows[0][0].as_datetime();
                respMsg["data"] = data;
                resp->String(respMsg.dump());
            } else {
                resp->String("User info get failed");
            }
        }); });
}

void Cloudisk::loadFileUploadModule()
{
    _httpserver.POST("/file/upload", [this](const HttpReq *req, HttpResp *resp, SeriesWork *series)
                     {
     //1. 解析请求
        if(req->content_type() == MULTIPART_FORM_DATA) {
            auto & form = req->form();
            auto pairKV = form["file"];
            cout << "filename: " <<  pairKV.first << endl;
            /* cout << "content: " << pairKV.second << endl; */
            string filename = form["file"].first;
            string content = form["file"].second;
            //2. 将文件内容写入本地
            mkdir("./tmp", 0755);
            string filepath = "./tmp/" + filename;
            int fd = open(filepath.c_str(), O_CREAT|O_RDWR, 0644);
            if(fd < 0) {
                perror("open");
                resp->String("upload file error");
                return;
            }
            int ret = write(fd, content.c_str(), content.size());
            close(fd);

            // TODO
            // 将上传的东西放到消息队列
            std::string ObjectName = "netdisk/" + filename;
            Json uploaderInfo;
            uploaderInfo["filePath"] = filepath;
            uploaderInfo["objectName"] = ObjectName;
            _publisher.doPublish(uploaderInfo.dump());

            //3. 重定向到上传成功的页面
            resp->headers["Location"] = "/file/upload/success";
            resp->headers["Content-Type"] = "text/html";
            resp->set_status_code("301");
            resp->set_reason_phrase("Moved Permanently");
            //4. 将文件信息更新到数据库中
            //4.1 用户名
            string username = req->query("username");
            //4.2 文件的hash值
            Hash hash(filepath);
            string filehash = hash.sha1();
            string url("mysql://root:1234@localhost");
            auto mysqlTask = WFTaskFactory::create_mysql_task(url, 1, nullptr);
            string sql = "INSERT INTO tbl_sql.tbl_user_file(user_name, file_sha1,file_size, file_name, status)VALUES('";
            sql += username + "', '" + filehash + "', " + std::to_string(content.size()) + ",'" + filename + "', 0);";
            cout << "sql:\n";
            cout << sql << endl;
            mysqlTask->get_req()->set_query(sql);
            series->push_back(mysqlTask);
        } });
}

void Cloudisk::loadUserFileListModule()
{
    _httpserver.POST("/file/query", [](const HttpReq *req, HttpResp *resp)
                     {
        //1. 解析请求
        string username = req->query("username");
        string token = req->query("token");
        auto & formKV = req->form_kv();
        string limitcnt = formKV["limit"];
        cout << "username:" << username << endl;
        cout << "token:" << token << endl;
        cout << "limitcnt:" << limitcnt << endl;
        //2. 校验token, 将请求中的token与数据库中的token比对, 如果正确就进行第三步, 错误直接响应Invalid token
        //3. 查询数据库
        string sql("select file_sha1, file_name, file_size, upload_at, last_update ");
        sql += " from tbl_sql.tbl_user_file where user_name = '" + username;
        sql += "' limit " + limitcnt + ";";
        cout << "sql:\n" << sql << endl;
        string url("mysql://root:1234@localhost");
        using namespace protocol;
        resp->MySQL(url, sql, [resp, username](MySQLResultCursor * pcursor){
            using std::vector;
            vector<vector<MySQLCell>> rows;
            pcursor->fetch_all(rows);
            if(rows.size() == 0) return;

            Json arrMsg;
            for(size_t i = 0; i < rows.size(); ++i)  {
                Json row;
                row["FileHash"] = rows[i][0].as_string();
                row["FileName"] = rows[i][1].as_string();
                row["FileSize"] = rows[i][2].as_ulonglong();
                row["UploadAt"] = rows[i][3].as_datetime();
                row["LastUpdated"] = rows[i][4].as_datetime();
                arrMsg.push_back(row);//将arrMsg当成数组来使用
            }  
            resp->String(arrMsg.dump());
        }); });
}

void Cloudisk::loadFileDownloadModule()
{
    _httpserver.GET("/file/downloadurl", [](const HttpReq *req, HttpResp *resp)
                    {
        // 解析请求
        auto &queryList = req->query_list();
        string filename = req->query("filename");
        cout << "filename: " << filename << endl;
        string filepath = "./tmp/" + filename;
        
        //将下载业务从服务器中分离出去，之后只需要产生一个下载链接就可以了
        //这要求我们还需要去部署一个下载服务器
        string downloadURL = "http://144.126.215.11:8868/" + filename;
        resp->String(downloadURL); });
}

int main()
{
    Cloudisk cloudisk;
    cloudisk.start(8888);
    return 0;
}
