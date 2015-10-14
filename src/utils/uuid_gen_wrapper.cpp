#include "uuid_gen_wrapper.hpp"
#include <Poco/UUID.h>
#include <Poco/UUIDGenerator.h>

using namespace std;
using namespace Poco;

namespace utils
{
    UUIDGenerator & GetUUIDGenerator()
    {
        static UUIDGenerator s_instance;
        return s_instance;
    }

    puuid_t GenerateUUID()
    {
        return GetUUIDGenerator().createOne().variant();
    }

};