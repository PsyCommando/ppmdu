#include "multithread_logger.hpp"
#include "library_wide.hpp"

namespace logging
{
    const std::string & GetLibWideLogDirectory()
    {
        return utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramLogDir);
    }
};