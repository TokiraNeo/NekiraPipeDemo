#pragma once

#include "MessageItem.hpp"
#include <Windows.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <deque>
#include <string>
#include <functional>


namespace NekiraPipeServer
{

class PipeSession
{
public:
    // 接收到信息回调
    using ReceiveCallback = std::function<void(std::unique_ptr<IMessageItem>)>;

private:
    HANDLE sendPipe;
    HANDLE receivePipe;

    std::wstring sendPipeName;
    std::wstring receivePipeName;

    size_t messageBufferSize;
    size_t consumerThreadCount;

    ReceiveCallback receiveCallback;

    std::deque<std::unique_ptr<IMessageItem>> messageQueue;

    std::atomic_bool isStopped;

    // 线程池
    std::vector<std::thread> threads;

    // 互斥锁
    std::mutex stateMutex;

    // 通知消费者线程
    std::condition_variable notifyConsumerCond;

    // 通知连接重试线程
    std::condition_variable notifyRetryCond;

    // 通知接收线程
    std::condition_variable notifyReceiveCond;

public:
    PipeSession(std::wstring sendPipe, std::wstring receivePipe, size_t msgBufferSize = 20, size_t consumerCount = 2);
    ~PipeSession();

    PipeSession(const PipeSession&) = delete;
    PipeSession& operator=(const PipeSession&) = delete;

    PipeSession(PipeSession&&) noexcept = delete;
    PipeSession& operator=(PipeSession&&) noexcept = delete;

    inline void SetReceiveCallback(ReceiveCallback callback) { receiveCallback = std::move(callback); }

    void Start();
    void Stop();

    bool SendMessage(std::unique_ptr<IMessageItem> message, std::string& errMsg);

private:
    void ConsumerThread();
    void RetryThread();
    void ReceiveThread();
};
} // namespace NekiraPipeServer
