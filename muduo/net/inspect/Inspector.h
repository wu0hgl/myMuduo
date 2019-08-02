﻿#ifndef MUDUO_NET_INSPECT_INSPECTOR_H
#define MUDUO_NET_INSPECT_INSPECTOR_H

#include <muduo/base/Mutex.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpServer.h>

#include <map>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

namespace net
{

class ProcessInspector;

// A internal inspector of the running process, usually a singleton.
class Inspector : boost::noncopyable
{
public:
    typedef std::vector<string> ArgList;
    typedef boost::function<string(HttpRequest::Method, const ArgList& args)> Callback;

    Inspector(EventLoop* loop,
              const InetAddress& httpAddr,
              const string& name);
    ~Inspector();

    // 如add("proc", "pid", ProcessInspector::pid, "print pid");
    // http://192.168.159.188:12345/proc/pid这个http请求就会相应的调用ProcessInspector::pid来处理
    void add(const string& module,  // proc
             const string& command, // pid
             const Callback& cb,    // 
             const string& help);   // print pid

private:
    typedef std::map<string, Callback>  CommandList;    // command callback
    typedef std::map<string, string>    HelpList;       // command helper

    void start();
    void onRequest(const HttpRequest& req, HttpResponse* resp);

    HttpServer  server_;
    boost::scoped_ptr<ProcessInspector> processInspector_;
    MutexLock   mutex_;
    std::map<string, CommandList>   commands_;
    std::map<string, HelpList>      helps_;
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_INSPECT_INSPECTOR_H