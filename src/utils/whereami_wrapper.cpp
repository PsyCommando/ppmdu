#include "whereami_wrapper.hpp"
#include <whereami.h>
#include <vector>

namespace utils
{
    std::string GetPathExe()
    {
        //First dummy call the function to get the length so we can alloc
        int               length = wai_getExecutablePath(NULL, 0, NULL);
        std::vector<char> dest(length,0);
        //Then get the path
        wai_getExecutablePath(dest.data(), length, NULL);
        return std::string(dest.data());
    }

    std::string GetPathExeDirectory()
    {
        //First dummy call the function to get the length so we can alloc
        int               dirnamelen = 0; //This will contain the length of the path that's the containing directory
        int               length     = wai_getExecutablePath(NULL, 0, NULL);
        std::vector<char> dest(length,0);
        //Then get the path
        wai_getExecutablePath(dest.data(), length, &dirnamelen);
        return std::string( dest.begin(), std::next(dest.begin(),dirnamelen) );
    }
};