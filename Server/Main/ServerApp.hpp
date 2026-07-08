#pragma once

#include "MessageItem.hpp"
#include "PipeSession.hpp"
#include "json.hpp"
#include <unordered_map>

using Json = nlohmann::json;

using namespace NekiraPipeServer;

class NormalMessageItem final : public IMessageItem
{
public:
    IRequestTask::Type taskId;
    std::string        command;
    Json               payload;

    std::string Serialize() override;
    void        Deserialize(const std::string& data) override;
};

class ServerApp final
{
private:
    PipeSession                                                           pipeSession;
    std::unordered_map<IRequestTask::Type, std::unique_ptr<IRequestTask>> taskMap;

    std::mutex taskMapMutex;

public:
    ServerApp(std::wstring sendPipe, std::wstring receivePipe, size_t msgBufferSize = 20, size_t consumerCount = 2);
    ~ServerApp();

    ServerApp(const ServerApp&) = delete;
    ServerApp& operator=(const ServerApp&) = delete;

    ServerApp(ServerApp&&) = delete;
    ServerApp& operator=(ServerApp&&) = delete;

    void Start();
    void Stop();

    bool AsyncRequestTask(std::string command, Json payload, IRequestTask::ResponseCallback callback, DWORD& errCode,
                          std::string& errMsg, std::chrono::seconds timeout = std::chrono::seconds(3));

private:
    void                          AddTask(std::unique_ptr<IRequestTask> task);
    std::unique_ptr<IRequestTask> PopTaskById(IRequestTask::Type id);

    void OnReceiveMessage(std::string message);

    void OnTaskTimeout(IRequestTask::Type taskId);

    void CompleteTask(std::unique_ptr<IRequestTask> task, std::unique_ptr<IMessageItem> message, DWORD errCode = 0,
                      std::string errMsg = "");
};
