#include "NekiraPipeUtility.hpp"
#include <WinBase.h>

namespace NekiraPipeUtility
{
void HandleCloser::operator()(HANDLE h) const noexcept
{
    if (h && h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
    }
}

DWORD GetPipeServerPid(const HANDLE& pipeHandle)
{
    DWORD pid = 0;
    if (pipeHandle && pipeHandle != INVALID_HANDLE_VALUE)
    {
        GetNamedPipeServerProcessId(pipeHandle, &pid);
    }
    return pid;
}

DWORD GetPipeClientPid(const HANDLE& pipeHandle)
{
    DWORD pid = 0;
    if (pipeHandle && pipeHandle != INVALID_HANDLE_VALUE)
    {
        GetNamedPipeClientProcessId(pipeHandle, &pid);
    }
    return pid;
}
} // namespace NekiraPipeUtility
