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

} // namespace NekiraPipeUtility
