#include "text_str.hpp"
#include <ppmdu/utils/gfileio.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;

namespace pmd2 { namespace filetypes
{

//============================================================================================
//  Loader
//============================================================================================
    /*
        TextStrLoader
            Loads the text_*.str files used in pmd2!
    */
    class TextStrLoader
    {
    public:
        TextStrLoader( const std::string & filepath )
            :m_strFilePath(filepath)
        {}

        operator std::vector<std::string>() //#REMOVEME: Just out of curiosity I wanted to try this!
        {
            return Read();
        }

        std::vector<std::string> operator()()
        {
            return Read();
        }

        std::vector<std::string> Read()
        {
            try
            {
                m_txtstr = vector<string>(); //Ensure the vector has a valid state
                m_filedata = utils::io::ReadFileToByteVector( m_strFilePath );

                //Read pointer table
                ReadPointerTable();

                //Read all the strings
                ReadStrings();
            }
            catch( exception & e )
            {
                stringstream sstr;
                sstr << "ERROR: While parsing file \"" << m_strFilePath <<"\". Cannot load the game's text strings file! : "
                     << e.what();
                string strerror = sstr.str();
                clog << strerror <<"\n";
                throw runtime_error(strerror);
            }

            return std::move(m_txtstr);
        }

    private:

        void ReadPointerTable()
        {
            m_ptrTable.resize(0);
            auto itptrs = m_filedata.begin();

            //First get the first pointer to get the end of the ptr table!
            uint32_t endptrtbl = utils::ReadIntFromByteVector<uint32_t>( itptrs );
            m_ptrTable.push_back( endptrtbl );

            //Read all pointers
            auto itendptrs = m_filedata.begin() + endptrtbl;
            for( ; itptrs != itendptrs; )
                m_ptrTable.push_back( utils::ReadIntFromByteVector<uint32_t>( itptrs ) );
        }

        void ReadStrings()
        {
            //Allocate
            m_txtstr.resize(m_ptrTable.size());

            //Read them all
            for( unsigned int i = 0; i < m_ptrTable.size(); ++i )
            {
                m_txtstr[i] = string( reinterpret_cast<char*>(m_filedata.data() + m_ptrTable[i]) );
            }

        }

        std::string              m_strFilePath;
        std::vector<uint8_t>     m_filedata;
        std::vector<uint32_t>    m_ptrTable;
        std::vector<std::string> m_txtstr;
    };

//============================================================================================
//  Writer
//============================================================================================

    /*
        TextStrWriter
            Write a text_*.str file from the text strings specified!
    */
    class TextStrWriter
    {
    public:
        TextStrWriter( const std::vector<std::string> & textstr )
            :m_txtstr(textstr)
        {}

        void Write( const std::string & filepath )
        {
            if( ! utils::isFile( filepath ) )
            {
                stringstream sstr;
                sstr << "ERROR: File \"" <<filepath <<"\" cannot be opened for writing!";
                string errorstr = sstr.str();
                clog << errorstr <<"\n";
                throw runtime_error(errorstr);
            }

            //Re-init ptr write positon
            m_ptrTblWriteAt = 0;

            //Reserve ptr table space!
            m_fileData.resize( m_txtstr.size() * PTR_LEN );

            //Write each strings
            auto itbackins = back_inserter( m_fileData );

            for( const auto & str : m_txtstr )
            {
                utils::WriteIntToByteVector   ( m_fileData.size(), m_fileData.begin() + m_ptrTblWriteAt );  //Write string offset
                m_ptrTblWriteAt += PTR_LEN;
                utils::WriteStrToByteContainer( itbackins, str );   //Write string
            }

            //Write file
            utils::io::WriteByteVectorToFile( filepath, m_fileData );
        }

    private:
        const unsigned int               PTR_LEN = sizeof(uint32_t);
        std::vector<uint8_t>             m_fileData;
        uint32_t                         m_ptrTblWriteAt;    //Position to write current str offset at
        const std::vector<std::string> & m_txtstr;
    };

//=========================================================================================
//  Functions
//=========================================================================================
    std::vector<std::string> ParseTextStrFile( const std::string & filepath )
    {
        //Read the pointer table first
        return TextStrLoader(filepath); //lol, implicit cast operator
    }
    
    void WriteTextStrFile( const std::string & filepath, const std::vector<std::string> & text )
    {
        TextStrWriter(text).Write(filepath);
    }

};};