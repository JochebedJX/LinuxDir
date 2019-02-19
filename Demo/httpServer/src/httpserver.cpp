#include <iostream>
#include "threadpool.hpp"
#include <sys/types.h>          
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "utils.hpp"

#define MAX_LISTEN 5
#define MAX_THREAD 5

//建立一个tcp服务器端程序，接收新连接
//为新连接组织一个线程池任务，添加到新线程池中
class HttpServer
{
private:
  int _serv_sock;
  //线程池对象，可以直接new一个线程池，从而来设置线程池的线程数
  ThreadPool* _tp;

private:
  //http任务的处理函数
  static bool HttpHandler(int sock)
  {
    RequestInfo info;
    HttpRequest req(sock);
    HttpResponse rsp(sock);

    //接收http头部
    if (req.RecvHttpHeader(info) == false)
    {
      goto out;
    }
    //解析http头部
    if (req.ParseHttpHeader(info) == false)
    {
      goto out;
    }
    //判断请求是否是CGI请求
    if (info.RequestIsCGI())
    {
      //若当前请求类型是CGI请求，则执行CGI响应
      rsp.CGIHandler(info);
    }
    else 
    {
      //若当前请求类型不是CGI请求，则执行文件列表或文件下载响应
      rsp.FileHandler(info);
    }
    
    close(sock);
    return true;
out:
    rsp.ErrHandler(info);
    close(sock);
    return false;
  }

public:
  HttpServer()
    : _serv_sock(-1)
    , _tp(NULL)
  {}

  //tcp服务器socket的初始化，以及线程的初始化
  bool HttpServerInit(std::string ip, int port)
  {
    _serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_serv_sock < 0)
    {
      LOG("sock error:%s\n", strerror(errno));
      return false;
    }

    sockaddr_in lst_addr;
    lst_addr.sin_family = AF_INET;
    lst_addr.sin_port = htons(port);
    lst_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    socklen_t len = sizeof(sockaddr_in);
    if ((bind(_serv_sock, (sockaddr*)&lst_addr, len) < 0))
    {
      LOG("bind error :%s\n", strerror(errno));
      close(_serv_sock);
      return false;
    }

    if (listen(_serv_sock, MAX_LISTEN) < 0)
    {
      LOG("listen errno :%s\n", strerror(errno));
      close(_serv_sock);
      return false;
    }

    _tp = new ThreadPool(MAX_THREAD);
    if (_tp == NULL)
    {
      LOG("thread pool malloc error!!\n");
      return false;
    }

    if (_tp->ThreadPoolInit() == false)
    {
      LOG("thread pool init error!!\n");
      return false;
    }
    return true;
  }

  //开始获取客户端新连接--创建任务--任务入队
  bool Start()
  {
      while (1)
      {
        sockaddr_in cli_addr;
        socklen_t len = sizeof(sockaddr_in);
        int new_sock = accept(_serv_sock, (sockaddr*)&cli_addr, &len);
        if (new_sock < 0)
        {
          LOG("accept error :%s\n", strerror(errno));
          return false;
        }
        HttpTask ht;
        ht.SetHttpTask(new_sock, HttpHandler);
        _tp->PopTask(ht);
      }

      return true;
  }
};


int main()
{
  //ThreadPool tp(5);
  return 0;
}
