#include "NekiraPipeUtility.hpp"
#include "uuid.h"
#include <WinBase.h>
#include <cassert>


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

uuids::uuid GenerateUUID()
{
    std::random_device rd;

    auto seed_data = std::array<int, std::mt19937::state_size>{};

    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));

    std::mt19937 generator(seq);

    uuids::uuid_random_generator gen{generator};

    const auto id = gen();

    assert(!id.is_nil());
    assert(id.as_bytes().size() == 16);
    assert(id.version() == uuids::uuid_version::random_number_based);
    assert(id.variant() == uuids::uuid_variant::rfc);

    return id;
}

std::string GenerateUUIDString()
{
    const auto id = GenerateUUID();
    return uuids::to_string(id);
}

std::wstring UTF8ToWString(const std::string& str)
{
    if (str.empty())
    {
        return std::wstring();
    }

    auto expected = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        str.size(),
        nullptr,
        0
    );

    if (expected == 0)
    {
        return std::wstring();
    }

    std::vector<wchar_t> buffer(expected, '\0');
    auto coverted = MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        str.size(),
        buffer.data(),
        expected
    );

    if (coverted != expected)
    {
        return std::wstring();
    }

    return std::wstring(buffer.data(), coverted);
}

} // namespace NekiraPipeUtility
