#pragma once

#include <Windows.h>

namespace NekiraPipeUtility
{
DWORD GetPipeServerPid(const HANDLE& pipeHandle);

DWORD GetPipeClientPid(const HANDLE& pipeHandle);
} // namespace NekiraPipeUtility
