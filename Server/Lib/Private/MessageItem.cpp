#include "MessageItem.hpp"

namespace NekiraPipeServer
{
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
    if (!timeoutCallback)
    {
        return;
    }

    if (timeout.count() <= 0 || timeoutDuration.count() > 0)
    {
        return;
    }

    timeoutDuration = timeout;

    timeoutThread = std::thread(&IRequestTask::TimeoutThread, this);
}

void IRequestTask::CompleteTask(std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg)
{
    if (isCompleted.load())
    {
        return;
    }

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

std::string IMessageItem::Serialize()
{
    return {};
}

void IMessageItem::Deserialize(const std::string& data)
{
    (void)data;
}

RequestWaiter::RequestWaiter()
    : eventHandle(CreateEventW(0, TRUE, FALSE, 0)), bSuccessed(false), messageItem(nullptr), errorCode(0),
      errorMessage("")
{}

RequestWaiter::~RequestWaiter()
{
    if (eventHandle && eventHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(eventHandle);
        eventHandle = nullptr;
    }
}

void RequestWaiter::OnRequestResponse(std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg)
{
    bSuccessed = (errCode == 0);
    messageItem = std::move(message);
    errorCode = errCode;
    errorMessage = std::move(errMsg);

    if (eventHandle && eventHandle != INVALID_HANDLE_VALUE)
    {
        SetEvent(eventHandle);
    }
}

void RequestWaiter::Wait()
{
    if (eventHandle && eventHandle != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(eventHandle, INFINITE);
    }
}

bool RequestWaiter::IsSuccessed() const
{
    return bSuccessed;
}

IMessageItem* RequestWaiter::GetMessageItem() const
{
    return messageItem.get();
}

DWORD RequestWaiter::GetErrorCode() const
{
    return errorCode;
}

std::string RequestWaiter::GetErrorMessage() const
{
    return errorMessage;
}

} // namespace NekiraPipeServer
