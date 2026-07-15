#pragma once

#include <Windows.h>
#include <string>

namespace uuids
{
class uuid;
} // namespace uuids

namespace NekiraPipeUtility
{
struct HandleCloser final
{
    void operator()(HANDLE h) const noexcept;
};

DWORD GetPipeServerPid(const HANDLE& pipeHandle);

DWORD GetPipeClientPid(const HANDLE& pipeHandle);

uuids::uuid GenerateUUID();

std::string GenerateUUIDString();

std::wstring UTF8ToWString(const std::string& str);
} // namespace NekiraPipeUtility
