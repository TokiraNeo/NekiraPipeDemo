#pragma once

#include <string>

namespace NekiraPipeServer
{
class IMessageItem
{
public:
    virtual std::string Serialize() = 0;
    virtual void        Deserialize(const std::string& data) = 0;
};
} // namespace NekiraPipeServer
