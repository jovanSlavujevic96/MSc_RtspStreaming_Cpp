#pragma once

#include <string>

namespace net 
{

class NetInterface
{
public:
    static std::string GetLocalIPAddress();
};

}
