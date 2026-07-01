#include "PipeSession.hpp"
#include <cassert>
#include "NekiraPipeUtility.hpp"

namespace NekiraPipeServer
{

PipeSession::PipeSession(std::wstring sendPipe, std::wstring receivePipe, size_t msgBufferSize, size_t consumerCount)
    : sendPipe(INVALID_HANDLE_VALUE)
    , receivePipe(INVALID_HANDLE_VALUE)
    , sendPipeName(std::move(sendPipe))
    , receivePipeName(std::move(receivePipe))
    , messageBufferSize(msgBufferSize)
    , consumerThreadCount(consumerCount)
{
    // 确保管道名称不为空
    assert(!sendPipeName.empty() && !receivePipeName.empty());

    messageBufferSize = (messageBufferSize == 0) ? 20 : messageBufferSize;

    consumerThreadCount = (consumerThreadCount == 0) ? 2 : consumerThreadCount;

    isStopped.store(true);
}

PipeSession::~PipeSession()
{
    Stop();
}

void PipeSession::Start()
{
    isStopped.store(false);

    // 创建消费者线程
    for (auto i = 0; i < consumerThreadCount; ++i)
    {
        std::thread consumer(&PipeSession::ConsumerThread, this);
        consumer.detach();
        threads.push_back(std::move(consumer));
    }

    // 创建重试线程
    std::thread retry(&PipeSession::RetryThread, this);
    retry.detach();
    threads.push_back(std::move(retry));

    // 创建接收管道的服务端
    receivePipe = CreateNamedPipeW(
        receivePipeName.c_str(),
        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        512, // 输出缓冲区大小
        512, // 输入缓冲区大小
        0,   // 默认超时
        nullptr
    );

    // 创建接收线程
    std::thread receive(&PipeSession::ReceiveThread, this);
    receive.detach();
}

void PipeSession::Stop()
{
    if (isStopped.exchange(true)) { return; }

    isStopped.store(true);

    // 通知所有线程退出
    notifyConsumerCond.notify_all();
    notifyRetryCond.notify_all();
    notifyReceiveCond.notify_all();

    // 其余线程可能仍然在使用 sendPipe，这里拷贝一份，然后取消所有挂起的 I/O 操作
    HANDLE sendPipeCopy = INVALID_HANDLE_VALUE;
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        sendPipeCopy = sendPipe;
    }

    if (sendPipeCopy != INVALID_HANDLE_VALUE)
    {
        CancelIoEx(sendPipeCopy, nullptr);
    }

    // 关闭所有线程
    for(auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    threads.clear();

    // 线程全部退出后，再关闭管道句柄
    if (sendPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sendPipe);
        sendPipe = INVALID_HANDLE_VALUE;
    }
    if (receivePipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(receivePipe);
        receivePipe = INVALID_HANDLE_VALUE;
    }
}

bool PipeSession::SendMessage(std::unique_ptr<IMessageItem> message, std::string& errMsg)
{
    if (!message) { return false; }

    bool bIsConnected = false;

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (messageQueue.size() >= messageBufferSize)
        {
            // 队列已满，拒绝新的消息
            errMsg = "Message Queue is full.";
            return false;
        }
        bIsConnected = (sendPipe != INVALID_HANDLE_VALUE);
        messageQueue.push_front(std::move(message));
        // 通知消费者线程有新消息
        notifyConsumerCond.notify_all();
    }

    // 如果管道未连接，通知重试线程尝试重新连接
    if (!bIsConnected)
    {
        errMsg = "Send pipe is not connected. Notifying retry thread.";
        notifyRetryCond.notify_one();
    }

    return true;
}

void PipeSession::ConsumerThread()
{
    OVERLAPPED overlapped {};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr); // 创建手动重置事件

    auto eventGuard = std::unique_ptr<std::remove_pointer_t<HANDLE>, NekiraPipeUtility::HandleCloser>(overlapped.hEvent);

    while (true)
    {
        std::unique_ptr<IMessageItem> message;
        HANDLE pipeHandle = INVALID_HANDLE_VALUE;

        {
            std::unique_lock<std::mutex> lock(stateMutex);
            notifyConsumerCond.wait(lock, [this] { return isStopped.load() || (!messageQueue.empty() && sendPipe != INVALID_HANDLE_VALUE); });

            // 如果已停止，退出线程
            if (isStopped.load()) { break; }

            pipeHandle = sendPipe;
            message = std::move(messageQueue.front());
            messageQueue.pop_front();
        }

        // 发送消息
        if (!message) { continue; }

        ResetEvent(overlapped.hEvent);

        bool bSendSuccess = false;

        const auto serializedData = message->Serialize();
        DWORD bytesWritten = 0;

        auto writeResult = WriteFile(pipeHandle, serializedData.c_str(), static_cast<DWORD>(serializedData.size()), &bytesWritten, &overlapped);

        if (writeResult)
        {
            // 同步完成
            bSendSuccess = (bytesWritten == serializedData.size());
        }
        else if (GetLastError() == ERROR_IO_PENDING)
        {
            const auto waitResult = WaitForSingleObject(overlapped.hEvent, INFINITE);

            if (waitResult == WAIT_OBJECT_0)
            {
                // 异步完成
                bSendSuccess = GetOverlappedResult(pipeHandle, &overlapped, &bytesWritten, FALSE) && (bytesWritten == serializedData.size());
            }
        }

        if (bSendSuccess) { continue; }

        // 发送失败，先检查是否因停止导致，是则退出线程
        if (isStopped.load()) { break; }

        // 非停止导致的发送失败，需要把消息重新放回队列，然后通知重试线程
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            messageQueue.push_front(std::move(message));

            // 关闭当前管道连接
            if (sendPipe == pipeHandle)
            {
                CloseHandle(sendPipe);
                sendPipe = INVALID_HANDLE_VALUE;
            }
            // 通知重试线程尝试重新连接
            notifyRetryCond.notify_one();
        }
    }
}

void PipeSession::RetryThread()
{
    constexpr auto retryInterval = std::chrono::seconds(1);

    while (true)
    {
        bool bIsConnected = false;

        {
            std::unique_lock<std::mutex> lock(stateMutex);

            notifyRetryCond.wait_for(lock, retryInterval, [this] {return isStopped.load() || sendPipe == INVALID_HANDLE_VALUE; });

            // 如果已停止，退出线程
            if (isStopped.load()) { break; }

            // 醒来后检查是否还真的需要重试连接
            if (sendPipe != INVALID_HANDLE_VALUE) { continue; }

            // 尝试连接发送管道
            sendPipe = CreateFileW(sendPipeName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

            bIsConnected = (sendPipe != INVALID_HANDLE_VALUE);
        }

        // 如果连接成功，通知消费者线程
        if (bIsConnected)
        {
            notifyConsumerCond.notify_all();
        }
    }
}

void PipeSession::ReceiveThread()
{
    while (true)
    {
        // @TODO: `等待连接 - 读取消息 - 处理消息` 循环
    }
}

} // namespace NekiraPipeServer
