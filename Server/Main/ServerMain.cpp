#include <windows.h>
#include "NekiraPipeUtility.hpp"
#include <memory>
#include "ServerApp.hpp"

namespace
{
static constexpr auto SERVER_SEND_PIPE_PREFIX = L"\\\\.\\pipe\\NekiraPipeServerSend_";
static constexpr auto SERVER_RECEIVE_PIPE_PREFIX = L"\\\\.\\pipe\\NekiraPipeServerReceive_";

static constexpr auto SERVER_COMMAND_APPLY_EFFECT = "ApplyEffect";
static constexpr auto SERVER_COMMAND_GET_FEEDBACK = "GetFeedback";
static constexpr auto SERVER_COMMAND_REMOVE_EFFECT = "RemoveEffect";
static constexpr auto SERVER_COMMAND_EXIT = "Exit";
}

int main()
{
    auto appEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    std::unique_ptr<std::remove_pointer_t<HANDLE>, NekiraPipeUtility::HandleCloser> appEventGuard(appEvent);

    auto app = std::make_unique<ServerApp>(SERVER_SEND_PIPE_PREFIX, SERVER_RECEIVE_PIPE_PREFIX, 30, 2);

    return 0;
}
