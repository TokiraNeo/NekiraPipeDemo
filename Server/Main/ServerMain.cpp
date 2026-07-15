#include "ServerApp.hpp"
#include "json.hpp"
#include <Windows.h>
#include <format>
#include <iostream>
#include <unordered_map>

using Json = nlohmann::json;

namespace
{
static constexpr auto SERVER_SEND_PIPE_PREFIX = L"\\\\.\\pipe\\NekiraPipeServerSend_";
static constexpr auto SERVER_RECEIVE_PIPE_PREFIX = L"\\\\.\\pipe\\NekiraPipeServerReceive_";

static constexpr auto SERVER_COMMAND_APPLY_EFFECT = "ApplyEffect";
static constexpr auto SERVER_COMMAND_GET_FEEDBACK = "GetFeedback";
static constexpr auto SERVER_COMMAND_REMOVE_EFFECT = "RemoveEffect";

} // namespace


int main()
{
    auto app = std::make_unique<ServerApp>(SERVER_SEND_PIPE_PREFIX, SERVER_RECEIVE_PIPE_PREFIX, 20, 4);
    app->Start();

    auto stepMap = std::unordered_map<const char*, Json>{
        {SERVER_COMMAND_APPLY_EFFECT, {{"effect", "Poison"}, {"duration", 10}}},
        {SERVER_COMMAND_GET_FEEDBACK, {{"effect", "Poison"}}},
        {SERVER_COMMAND_REMOVE_EFFECT, {{"effect", "Poison"}}},
    };

    for (const auto step : stepMap)
    {
        const auto command = step.first;
        const auto payload = step.second;

        DWORD       errCode;
        std::string errMsg;

        RequestWaiter waiter;

        auto waitCallback = [&waiter](std::unique_ptr<IMessageItem> message, DWORD errCode, std::string errMsg)
        { waiter.OnRequestResponse(std::move(message), errCode, std::move(errMsg)); };

        if (!app->AsyncRequestTask(command, payload, waitCallback, errCode, errMsg, std::chrono::seconds(5)))
        {
            std::cout << std::format("Send command failed. command: {}, errCode: {}, errMsg: {}\n\n", command, errCode,
                                     errMsg);
            continue;
        }

        std::cout << std::format("Send command successed: {}\n\n", command);

        waiter.Wait();

        auto message = static_cast<NormalMessageItem*>(waiter.GetMessageItem());

        if (waiter.IsSuccessed())
        {
            std::cout << std::format("Receive response. command: {}, payload: {}\n\n", command,
                                     message->payload.dump());

            continue;
        }

        std::cout << std::format("Receive response failed. command: {}, taskId: {}, errCode: {}, errMsg: {}\n\n",
                                 command, message->taskId, waiter.GetErrorCode(), waiter.GetErrorMessage());
    }

    std::cout << std::format("Server Exit.\n");
    app->Stop();

    return 0;
}
