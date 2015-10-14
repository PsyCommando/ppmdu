#ifndef FARC_BLOB_HPP
#define FARC_BLOB_HPP
/*
farc_blob.hpp
2015/09/23
psycommando@gmail.com
Description: Utilities for dealing with FARC archives containing as subfiles a file index table and a blob containing
             several files.
*/
#include <vector>
#include <cstdint>
#include <string>
#include <utility>
#include <map>

namespace filetypes
{

    /*
        DoesFARCContainsDataBlob
            Returns true if the FARC file contains an index table and a blob !
    */
    bool DoesFARCContainsDataBlob( const std::string & file );

    /*
        ReadFileIndexTblAndBlobFromFARC
            Reads to memory a copy of the file index table, and the data blob.
    */
    //std::pair< std::map<std::string, uint32_t>, 
    //           std::vector<uint8_t> 
    //         > ReadFileIndexTblAndBlobFromFARC( const std::string & file );

};

#endif