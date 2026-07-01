#pragma once

#include <Windows.h>

namespace NekiraPipeUtility
{
struct HandleCloser final
{
    void operator()(HANDLE h) const noexcept;
};

DWORD GetPipeServerPid(const HANDLE& pipeHandle);

DWORD GetPipeClientPid(const HANDLE& pipeHandle);
} // namespace NekiraPipeUtility
