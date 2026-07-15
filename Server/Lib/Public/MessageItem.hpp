#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>
#include <windows.h>

namespace NekiraPipeServer
{
class IMessageItem;
class IRequestTask
{
public:
    using Type = std::string;

    using TimeoutCallback = std::function<void(Type taskId)>;
    using ResponseCallback =
        std::function<void(std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg)>;

    IRequestTask(Type id);
    virtual ~IRequestTask();

    IRequestTask(const IRequestTask&) = delete;
    IRequestTask& operator=(const IRequestTask&) = delete;

    IRequestTask(IRequestTask&&) noexcept = delete;
    IRequestTask& operator=(IRequestTask&&) = delete;

    Type GetTaskId() const
    {
        return taskId;
    }

    void SetResponseCallback(ResponseCallback callback)
    {
        responseCallback = std::move(callback);
    }

    void SetTimeoutCallback(TimeoutCallback callback)
    {
        timeoutCallback = std::move(callback);
    }

    void StartTimeoutTimer(std::chrono::seconds timeout = std::chrono::seconds(3));

    void CompleteTask(std::unique_ptr<IMessageItem> message, DWORD errCode = 0, std::string errMsg = "");

private:
    void TimeoutThread();

private:
    Type taskId;

    ResponseCallback responseCallback;

    std::chrono::seconds timeoutDuration;
    TimeoutCallback      timeoutCallback;

    std::mutex              timeoutMutex;
    std::thread             timeoutThread;
    std::condition_variable timeoutCond;

    std::atomic_bool isCompleted;
};

class IMessageItem
{
public:
    IRequestTask::Type taskId;

    virtual ~IMessageItem() = default;
    virtual std::string Serialize();
    virtual void        Deserialize(const std::string& data);
};

class RequestWaiter final
{
private:
    HANDLE eventHandle;
    bool   bSuccessed;

    std::unique_ptr<IMessageItem> messageItem;
    DWORD                         errorCode;
    std::string                   errorMessage;

public:
    RequestWaiter();
    ~RequestWaiter();

    RequestWaiter(const RequestWaiter&) = delete;
    RequestWaiter& operator=(const RequestWaiter&) = delete;

    RequestWaiter(RequestWaiter&&) noexcept = delete;
    RequestWaiter& operator=(RequestWaiter&&) noexcept = delete;

    void OnRequestResponse(std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg);

    void Wait();

    bool IsSuccessed() const;

    IMessageItem* GetMessageItem() const;

    DWORD GetErrorCode() const;

    std::string GetErrorMessage() const;
};

} // namespace NekiraPipeServer
