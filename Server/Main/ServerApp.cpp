#include "ServerApp.hpp"
#include "NekiraPipeUtility.hpp"


std::string NormalMessageItem::Serialize()
{
    Json jsonData;
    jsonData["taskId"] = taskId;
    jsonData["command"] = command;
    jsonData["payload"] = payload;

    return jsonData.dump();
}

void NormalMessageItem::Deserialize(const std::string& data)
{
    Json jsonData = Json::parse(data);

    if (jsonData.contains("taskId") && jsonData["taskId"].is_string())
    {
        taskId = jsonData["taskId"].get<std::string>();
    }

    if (jsonData.contains("command") && jsonData["command"].is_string())
    {
        command = jsonData["command"].get<std::string>();
    }

    if (jsonData.contains("payload"))
    {
        payload = jsonData["payload"];
    }
}

ServerApp::ServerApp(std::wstring sendPipe, std::wstring receivePipe, size_t msgBufferSize, size_t consumerCount)
    : pipeSession(sendPipe, receivePipe, msgBufferSize, consumerCount)
{}

ServerApp::~ServerApp()
{
    Stop();
}

void ServerApp::Start()
{
    PipeSession::ReceiveCallback callback = [this](std::string message) { OnReceiveMessage(std::move(message)); };
    pipeSession.SetReceiveCallback(std::move(callback));
    pipeSession.Start();
}

void ServerApp::Stop()
{
    pipeSession.Stop();
}

bool ServerApp::AsyncRequestTask(std::string command, Json payload, IRequestTask::ResponseCallback callback,
                                 DWORD& errCode, std::string& errMsg, std::chrono::seconds timeout)
{
    if (!pipeSession.IsRunning())
    {
        errCode = ERROR_INVALID_STATE;
        errMsg = "Pipe session is not running.";
        return false;
    }

    if (timeout.count() <= 0)
    {
        errCode = ERROR_INVALID_PARAMETER;
        errMsg = "Timeout must be greater than zero.";
        return false;
    }

    const auto uuid = NekiraPipeUtility::GenerateUUIDString();

    if (uuid.empty())
    {
        errCode = ERROR_INVALID_PARAMETER;
        errMsg = "Failed to generate UUID.";
        return false;
    }

    auto item = std::make_unique<NormalMessageItem>();
    item->taskId = uuid;
    item->command = std::move(command);
    item->payload = std::move(payload);

    const auto result = pipeSession.ProductMessage(std::move(item), errCode, errMsg);

    if (result)
    {
        auto task = std::make_unique<IRequestTask>(uuid);

        task->SetResponseCallback(std::move(callback));

        auto timeoutCallback = [this](IRequestTask::Type taskId) { OnTaskTimeout(taskId); };
        task->SetTimeoutCallback(std::move(timeoutCallback));

        auto* taskPtr = task.get();

        AddTask(std::move(task));
        taskPtr->StartTimeoutTimer(timeout);
    }

    return result;
}

void ServerApp::AddTask(std::unique_ptr<IRequestTask> task)
{
    const auto                    taskId = task->GetTaskId();
    std::unique_ptr<IRequestTask> oldTask;

    {
        std::lock_guard<std::mutex> lock(taskMapMutex);
        auto                        it = taskMap.find(taskId);
        if (it != taskMap.end())
        {
            oldTask = std::move(it->second);
            taskMap.erase(it);
        }
        taskMap.emplace(taskId, std::move(task));
    }

    CompleteTask(std::move(oldTask), {}, ERROR_ALREADY_EXISTS, "Replaced by a new task with the same ID.");
}

std::unique_ptr<IRequestTask> ServerApp::PopTaskById(IRequestTask::Type id)
{
    std::unique_ptr<IRequestTask> task;

    {
        std::lock_guard<std::mutex> lock(taskMapMutex);
        auto                        it = taskMap.find(id);

        if (it != taskMap.end())
        {
            task = std::move(it->second);
            taskMap.erase(it);
        }
    }

    return task;
}

void ServerApp::OnReceiveMessage(std::string message)
{
    auto item = std::make_unique<NormalMessageItem>();
    item->Deserialize(message);

    const auto taskId = item->taskId;

    auto task = PopTaskById(taskId);

    CompleteTask(std::move(task), std::move(item));
}

void ServerApp::OnTaskTimeout(IRequestTask::Type taskId)
{
    auto task = PopTaskById(taskId);

    CompleteTask(std::move(task), {}, ERROR_TIMEOUT, "Task timed out.");
}

void ServerApp::CompleteTask(std::unique_ptr<IRequestTask> task, std::unique_ptr<IMessageItem> message, DWORD errCode,
                             std::string errMsg)
{
    if (task)
    {
        task->CompleteTask(std::move(message), errCode, std::move(errMsg));
    }
}
