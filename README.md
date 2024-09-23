# 目录

1. [开发环境](#开发环境)
2. [技术栈](#技术栈)
3. [安装与配置指南](#安装与配置指南)
   1. [安装 ppconsul](#1-安装-ppconsul)
      1. [安装必要的依赖](#11-安装必要的依赖)
      2. [下载并构建 ppconsul](#12-下载并构建-ppconsul)
   2. [Consul 部署与使用](#2-consul-部署与使用)
      1. [启动 Consul 服务器容器](#21-启动-consul-服务器容器)
      2. [获取分配的 IP 地址](#22-获取分配的-ip-地址)
      3. [启动其它两个 Consul 节点并加入集群](#23-启动其它两个-consul-节点并加入集群)
      4. [查看集群状态](#24-查看集群状态)
      5. [测试 Consul UI 和 API](#25-测试-consul-ui-和-api)
   3. [安装阿里云OSS SDK并设置OSS环境变量](#3-安装阿里云oss-sdk并设置oss环境变量)
      1. [安装必要的依赖](#31-安装必要的依赖)
      2. [设置环境变量](#32-设置环境变量)
      3. [修改 oss.hpp 文件配置](#33-修改-osshpp-文件配置)
   4. [安装并配置 Nginx](#4-安装并配置-nginx)
      1. [安装 Nginx](#41-安装-nginx)
      2. [配置 Nginx](#42-配置-nginx)
      3. [检查配置文件语法是否正确](#43-检查配置文件语法是否正确)
      4. [重新加载或重启 Nginx 服务](#44-重新加载或重启-nginx-服务)
   5. [MySQL 安装与配置](#5-mysql-安装与配置)
      1. [安装 MySQL](#51-安装-mysql)
      2. [检查 MySQL 状态](#52-检查-mysql-状态)
      3. [设置 MySQL root 用户密码](#53-设置-mysql-root-用户密码)
      4. [重新登录 MySQL](#54-重新登录-mysql)
      5. [打开 MySQL 配置文件](#55-打开-mysql-配置文件)
      6. [修改 bind-address 的值](#56-修改-bind-address-的值)
      7. [重启 MySQL 服务以应用更改](#57-重启-mysql-服务以应用更改)
      8. [验证 MySQL 是否绑定到新的地址](#58-验证-mysql-是否绑定到新的地址)
      9. [导入数据库](#59-导入数据库)
   6. [安装并配置 RabbitMQ](#6-安装并配置-rabbitmq)
      1. [启动 RabbitMQ 容器](#61-启动-rabbitmq-容器)
      2. [访问 RabbitMQ 管理界面](#62-访问-rabbitmq-管理界面)
      3. [根据 amqp.hpp 完成配置](#63-根据-amqphpp-完成配置)
   7. [安装 AMQP 的 C++ 客户端 SDK](#7-安装-amqp-的-c-客户端-sdk)
      1. [下载源码包](#71-下载源码包)
      2. [安装依赖](#72-安装依赖)
      3. [构建并安装](#73-构建并安装)
         1. [对 rabbitmq-c](#对-rabbitmq-c)
         2. [对 SimpleAmqpClient](#对-simpleamqpclient)
   8. [安装 Workflow](#8-安装-workflow)
      1. [下载源码包](#81-下载源码包)
      2. [安装依赖并构建](#82-安装依赖并构建)
   9. [安装 wfrest](#9-安装-wfrest)
      1. [下载源码包](#91-下载源码包)
      2. [构建并安装](#92-构建并安装)
   10. [安装 Protobuf](#10-安装-protobuf)
      11. [下载源码包](#101-下载源码包)
      12. [构建并安装](#102-构建并安装)
      13. [验证Protobuf](#103-验证protobuf)
   14. [安装 srpc](#11-安装-srpc)
      15. [下载源码包](#111-下载源码包)
      16. [安装依赖并构建](#112-安装依赖并构建)
4. [常见问题](#常见问题)

## 开发环境

- **操作系统**: Ubuntu 22.04
- **编程语言**: C++11
- **构建工具**: Make
- **数据库**: MySQL 8.0
- **容器化平台**: Docker

## 技术栈

- **消息队列**: RabbitMQ（使用 `SimpleAmqpClient` 库，Docker 部署）
- **对象存储服务**: 阿里云对象存储（Alibaba Cloud OSS）
- **服务注册中心**: Consul（使用 `ppconsul` 库）
- **Web框架**: wfrest（C++ 异步 Web 框架）
- **RPC机制**: sRPC（基于 Workflow 框架）

---

## 安装与配置指南

为了顺利运行本项目，请确保安装和配置必要的环境和依赖，并根据需要调整配置文件中的相关参数，以适应具体环境和需求。

### 1. 安装 ppconsul

**ppconsul** 是 Consul 的 C++ 客户端库。

#### 1.1 安装必要的依赖

```bash
sudo apt-get update && sudo apt-get install cmake libcurl4-openssl-dev libboost-all-dev
```

#### 1.2 下载并构建 ppconsul

```bash
git clone https://github.com/oliora/ppconsul.git && cd ppconsul && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

---

### 2. Consul 部署与使用

按照以下步骤部署和配置 Consul：

#### 2.1 启动 Consul 服务器容器

```bash
sudo docker pull consul:1.15.3 && sudo docker run --hostname consulsvr1 --name consul_node_1 -d -p 8500:8500 -p 8301:8301 -p 8302:8302 -p 8600:8600 consul:1.15.3 agent -server -bootstrap-expect 2 -ui -bind=0.0.0.0 -client=0.0.0.0
```

#### 2.2 获取分配的 IP 地址

```bash
sudo docker inspect --format '{{.NetworkSettings.IPAddress}}' consul_node_1
```

#### 2.3 启动其它两个 Consul 节点并加入集群

确保将上述命令中的 `<IP_ADDRESS>` 替换为实际的 IP 地址。

```bash
sudo docker run --hostname consulsvr2 --name consul_node_2 -d -p 8501:8500 consul:1.15.3 agent -server -ui -bind=0.0.0.0 -client=0.0.0.0 -join <IP_ADDRESS> && sudo docker run --hostname consulsvr3 --name consul_node_3 -d -p 8502:8500 consul:1.15.3 agent -server -ui -bind=0.0.0.0 -client=0.0.0.0 -join <IP_ADDRESS>
```

#### 2.4 查看集群状态

```bash
sudo docker exec -t consul_node_1 consul members
```

#### 2.5 测试 Consul UI 和 API

启动并测试后，您可以通过以下 URL 访问 Consul 服务：

- **UI**: [http://<Your_IP_Address>:8500/ui/dc1/services](http://<Your_IP_Address>:8500/ui/dc1/services)
- **API**: [http://<Your_IP_Address>:8500/v1/agent/services](http://<Your_IP_Address>:8500/v1/agent/services)

---

### 3. 安装阿里云OSS SDK并设置OSS环境变量

#### 3.1 安装必要的依赖

```bash
sudo apt-get install libssl-dev libcurl4-gnutls-dev
```

#### 3.2 设置环境变量

编辑 `~/.bashrc` 文件：

```bash
nano ~/.bashrc
```

在文件末尾添加以下内容：

```bash
export OSS_ACCESS_KEY_ID="your-access-key-id"
export OSS_ACCESS_KEY_SECRET="your-access-key-secret"
```

保存并关闭文件后，使其生效：

```bash
source ~/.bashrc
```

#### 3.3 修改 `oss.hpp` 文件配置

确保在 `oss.hpp` 文件中正确配置了阿里云OSS的相关信息，如 `Endpoint` 和 `BucketName`。

---

### 4. 安装并配置 Nginx

#### 4.1 安装 Nginx

```bash
sudo apt install nginx && sudo systemctl status nginx
```

#### 4.2 配置 Nginx

编辑 Nginx 配置文件：

```bash
sudo vim /etc/nginx/nginx.conf
```

将配置文件修改为以下内容：

```nginx
user root;
worker_processes  1;

events {
    worker_connections  1024;
}

http {
    #include       mime.types;
    default_type  application/octet-stream;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       8868;
        server_name  localhost;

        location / {
            # 将路径修改为服务器上传下载文件保存地址(pwd)
            root /root/myproject/tmp;
        }
    }
}
```

#### 4.3 检查配置文件语法是否正确

```bash
sudo nginx -t
```

如果输出显示配置文件语法正确：

```
nginx: the configuration file /etc/nginx/nginx.conf syntax is ok
nginx: configuration file /etc/nginx/nginx.conf test is successful
```

#### 4.4 重新加载或重启 Nginx 服务

重新加载配置：

```bash
sudo systemctl reload nginx
```

如果需要，您可以选择重启 Nginx 服务：

```bash
sudo systemctl restart nginx
```

---

### 5. MySQL 安装与配置

#### 5.1 安装 MySQL

```bash
sudo apt install -y mysql-server-8.0 libmysqlclient-dev
```

安装完成后，MySQL 服务会自动启动。

#### 5.2 检查 MySQL 状态

```bash
sudo systemctl status mysql
```

#### 5.3 设置 MySQL root 用户密码

由于安装时没有设置密码，使用以下命令登录并设置密码：

```bash
sudo mysql
```

在 MySQL 控制台中执行以下 SQL 语句，将 root 用户的密码设置为 `1234`：

```mysql
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '1234';
UPDATE mysql.user SET host='%' WHERE user='root';
FLUSH PRIVILEGES;
```

#### 5.4 重新登录 MySQL

```bash
mysql -uroot -p
```

输入密码 `1234` 进行登录。

#### 5.5 打开 MySQL 配置文件

MySQL 的 `bind-address` 通常位于配置文件 `mysqld.cnf` 中。

```bash
sudo nano /etc/mysql/mysql.conf.d/mysqld.cnf
```

**注意**：配置文件的路径可能因系统或 MySQL 版本而异。如果上面的路径不存在，可以尝试以下路径之一：

- `/etc/mysql/my.cnf`
- `/etc/my.cnf`

#### 5.6 修改 `bind-address` 的值

在打开的配置文件中，查找以下行：

```ini
bind-address = 127.0.0.1
```

根据您的需求修改 `bind-address` 的值：

- **允许仅本地访问**（默认设置）：

  ```ini
  bind-address = 127.0.0.1
  ```

- **允许所有网络接口访问**：

  ```ini
  bind-address = 0.0.0.0
  ```

#### 5.7 重启 MySQL 服务以应用更改

```bash
sudo systemctl restart mysql
```

#### 5.8 验证 MySQL 是否绑定到新的地址

您可以使用 `ss` 或 `netstat` 命令来验证 MySQL 是否正确绑定到指定的地址和端口（默认为 3306）。

使用 `ss`：

```bash
sudo ss -tuln | grep 3306
```

#### 5.9 导入数据库

运行 `tbl_sql.sql` 文件以导入数据库：

```bash
mysql -uroot -p your_database < tbl_sql.sql
```

---

### 6. 安装并配置 RabbitMQ

#### 6.1 启动 RabbitMQ 容器

```bash
sudo docker run -d --hostname rabbitsvr --name rabbit -p 5672:5672 -p 15672:15672 -p 25672:25672 -v /data/rabbitmq:/var/lib/rabbitmq rabbitmq:management
```

#### 6.2 访问 RabbitMQ 管理界面

打开浏览器访问 [http://<Your_IP_Address>:15672](http://<Your_IP_Address>:15672) 进行管理和配置。

#### 6.3 根据 `amqp.hpp` 完成配置

确保根据项目需求配置 RabbitMQ 的队列和交换机，具体配置可参考 `amqp.hpp` 文件中的实现。

---

### 7. 安装 AMQP 的 C++ 客户端 SDK

#### 7.1 下载源码包

请确保已下载以下源码包：

- `rabbitmq-c-0.11.0.tar.gz`
- `SimpleAmqpClient-2.5.1.tar.gz`

#### 7.2 安装依赖

```bash
sudo apt install libboost-dev libboost-system-dev libboost-chrono-dev
```

#### 7.3 构建并安装

##### 对于 `rabbitmq-c`：

```bash
tar -xzf rabbitmq-c-0.11.0.tar.gz && cd rabbitmq-c-0.11.0 && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

##### 对于 `SimpleAmqpClient`：

```bash
tar -xzf SimpleAmqpClient-2.5.1.tar.gz && cd SimpleAmqpClient-2.5.1 && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

---

### 8. 安装 Workflow

#### 8.1 下载源码包

请确保已下载 `workflow-0.11.3.tar.gz`。

#### 8.2 安装依赖并构建

```bash
sudo apt install cmake && tar -xzf workflow-0.11.3.tar.gz && cd workflow-0.11.3 && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

---

### 9. 安装 wfrest

#### 9.1 下载源码包

请确保已下载 `wfrest-0.9.6.tar.gz`。

#### 9.2 构建并安装

```bash
tar -xzf wfrest-0.9.6.tar.gz && cd wfrest-0.9.6 && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

---

### 10. 安装 Protobuf

#### 10.1 下载源码包

请确保已下载 `protobuf-3.20.1.tar.gz`。

#### 10.2 构建并安装

```bash
tar -xzf protobuf-3.20.1.tar.gz && cd protobuf-3.20.1 && sudo apt-get install autoconf automake libtool curl make g++ unzip && ./autogen.sh && ./configure && make && sudo make install && sudo ldconfig
```

#### 10.3 验证Protobuf

执行protoc，如果出现操作指南说明安装成功了。

---

### 11. 安装 srpc

#### 11.1 下载源码包

请确保已下载 `srpc-0.10.2.tar.gz`。

#### 11.2 安装依赖并构建

```bash
sudo apt install liblz4-dev libsnappy-dev && tar -xzf srpc-0.10.2.tar.gz && cd srpc-0.10.2 && mkdir build && cd build && cmake .. && make && sudo make install && sudo ldconfig
```

---

## 常见问题

**Q1: 为什么需要使用 `nlohmann::json`，而不是 wfrest 自带的 `Json` 类？**

**A1:** 项目中选择使用 `nlohmann::json`，是因为 wfrest 自带的 `Json` 类在处理嵌套 JSON 结构时存在一定的限制，导致JSON构造失败。

------

