// 示例
/* g++ consumer.cc -o consumer -fno-rtti -lalibabacloud-oss-cpp-sdk -lcurl -lcrypto -lpthread -lwfrest -lworkflow -lssl -lcrypto -lcrypt -lSimpleAmqpClient */
#include "amqp.hpp"
#include "oss.hpp"
#include <iostream>
#include "wfrest/json.hpp"
using std::cout;
using std::endl;

using Json = nlohmann::json;
void test0()
{
    Consumer consumer;
    OssUploader ossUploader;
    string msg;
    while (1)
    {
        // 不断读取消息队列中的消息,解析成备份OSS的操作
        bool hasMsg = consumer.doConsume(msg);
        if (hasMsg)
        {
            Json uploaderInfo = Json::parse(msg);
            string filepath = uploaderInfo["filePath"];
            string objectName = uploaderInfo["objectName"];
            cout << "filePath:" << filepath << endl;
            cout << "objectName:" << objectName << endl;
            // 调用OssUploader的对象进行备份
            ossUploader.doUpload(filepath, objectName);
        }
        else
        {
            cout << "get message timeout" << endl;
        }
    }
}

int main()
{
    test0();
    return 0;
}
