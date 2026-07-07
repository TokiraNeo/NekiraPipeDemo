#include "MessageItem.hpp"

namespace NekiraPipeServer
{
std::string IMessageItem::Serialize()
{
    return {};
}

void IMessageItem::Deserialize(const std::string& data)
{
    (void)data;
}

IRequestTask::IRequestTask(Type id) : taskId(id)
{
    timeoutDuration = std::chrono::seconds(0);
    isCompleted.store(false);
}

IRequestTask::~IRequestTask()
{
    isCompleted.store(true);

    timeoutCond.notify_all();

    if (timeoutThread.joinable())
    {
        timeoutThread.join();
    }
}

void IRequestTask::StartTimeoutTimer(std::chrono::seconds timeout)
{
    if (!timeoutCallback) { return; }

    if (timeout.count() <= 0 || timeoutDuration.count() > 0) { return; }

    timeoutDuration = timeout;

    timeoutThread = std::thread(&IRequestTask::TimeoutThread, this);
}

void IRequestTask::CompleteTask(std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg)
{
    if (isCompleted.load()) { return; }

    isCompleted.store(true);

    timeoutCond.notify_all();

    if (responseCallback)
    {
        responseCallback(std::move(message), errCode, std::move(errMsg));
    }
}

void IRequestTask::TimeoutThread()
{
    std::unique_lock<std::mutex> lock(timeoutMutex);

    if (timeoutCond.wait_for(lock, timeoutDuration, [this] { return isCompleted.load(); }))
    {
        // 任务已完成，退出线程
        return;
    }

    // 超时，调用回调函数
    if (timeoutCallback)
    {
        timeoutCallback(taskId);
    }
}

} // namespace NekiraPipeServer
