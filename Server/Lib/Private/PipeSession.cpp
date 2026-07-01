#include "PipeSession.hpp"
#include <cassert>

namespace NekiraPipeServer
{

PipeSession::PipeSession(std::wstring sendPipe, std::wstring receivePipe, size_t msgBufferSize, size_t consumerCount)
    : sendPipeName(std::move(sendPipe))
    , receivePipeName(std::move(receivePipe))
    , messageBufferSize(msgBufferSize)
    , consumerThreadCount(consumerCount)
{
    // 确保管道名称不为空
    assert(!sendPipeName.empty() && !receivePipeName.empty());

    messageBufferSize = (messageBufferSize <= 0) ? 20 : messageBufferSize;

    consumerThreadCount = (consumerThreadCount <= 0) ? 2 : consumerThreadCount;
}

PipeSession::~PipeSession()
{
    if (sendPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sendPipe);
        sendPipe = INVALID_HANDLE_VALUE;
    }
    if (receivePipe != INVALID_HANDLE_VALUE)
    {
        DisconnectNamedPipe(receivePipe);
        CloseHandle(receivePipe);
        receivePipe = INVALID_HANDLE_VALUE;
    }
}

void PipeSession::Start()
{
    // 创建消费者线程
    for (auto i = 0; i < consumerThreadCount; ++i)
    {
        std::thread consumer(&PipeSession::ConsumerThread, this);
        consumer.detach();
    }

    // 创建重试线程
    std::thread retry(&PipeSession::RetryThread, this);
    retry.detach();

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

bool PipeSession::SendMessage(std::unique_ptr<IMessageItem> message, std::string& errMsg)
{
    if (!message) { return false; }

    {
        std::lock_guard<std::mutex> lock(producerMutex);
        if (messageQueue.size() >= messageBufferSize)
        {
            // 队列已满，拒绝新的消息
            errMsg = "Message Queue is full.";
            return false;
        }
        messageQueue.push_front(std::move(message));
        // 通知消费者线程有新消息
        notifyConsumerCond.notify_one();
    }

    bool bIsConnected = false;
    {
        std::lock_guard<std::mutex> lock(producerMutex);
        bIsConnected = (sendPipe != INVALID_HANDLE_VALUE);
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
    while (true)
    {
        std::unique_ptr<IMessageItem> message;

        {
            std::unique_lock<std::mutex> lock(notifyConsumerMutex);
            notifyConsumerCond.wait(lock, [this] { return !messageQueue.empty() && sendPipe != INVALID_HANDLE_VALUE; });

            // @TODO: 这里还没发包成功就取出，可能会导致后续发包失败时，消息丢失。
            message = std::move(messageQueue.front());
            messageQueue.pop_front();

            if (!message) { continue; }

            const auto serializedMessage = message->Serialize();
            DWORD bytesWritten = 0;
            const auto bSendSuccess = WriteFile(sendPipe, serializedMessage.data(), static_cast<DWORD>(serializedMessage.size()), &bytesWritten, nullptr);

            if (bSendSuccess && bytesWritten == serializedMessage.size()) { continue; }

            // 发送失败，重新压回队首
            messageQueue.push_front(std::move(message));
        }
    }
}

void PipeSession::RetryThread()
{
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(notifyRetryMutex);
            notifyRetryCond.wait(lock, [this] { return sendPipe == INVALID_HANDLE_VALUE; });

            // 尝试重新连接管道
            sendPipe = CreateFileW(sendPipeName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

            // 如果连接成功，通知消费者线程继续发送消息
            if (sendPipe != INVALID_HANDLE_VALUE)
            {
                notifyConsumerCond.notify_one();
            }
            // 连接失败，间隔一段时间后重试
            else
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

void PipeSession::ReceiveThread()
{
    while (true)
    {
        // @TODO: `等待连接 -> 读取信息 -> 处理信息` 循环
        //
    }
}

} // namespace NekiraPipeServer
