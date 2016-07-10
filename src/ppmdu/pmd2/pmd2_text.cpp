#include "pmd2_text.hpp"
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <utils/utility.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <utils/poco_wrapper.hpp>
#include <utils/library_wide.hpp>
#include <regex>
using namespace std;

namespace pmd2
{


//
//
//
    void GameText::Load()
    {
        stringstream Dirname;
        Dirname << utils::TryAppendSlash(m_pmd2fsdir) << DirName_MESSAGE;
        vector<string> files = utils::ListDirContent_FilesAndDirs( Dirname.str() );

        //Look all the filenames in the directory to see if we have any info on them. 
        for( const auto & afile : files )
        {
            const StringsCatalog * pcata = m_conf.GetLanguageFilesDB().GetByTextFName( utils::GetFilename(afile) );
            if( pcata )
            {
                langstr_t mylang( std::move(filetypes::ParseTextStrFile(afile)),
                                  std::move( StringsCatalog(*pcata) ) );

                m_languages.emplace( pcata->GetLanguage(), 
                                     std::move(mylang) );
            }
            else
            {
                clog << "<!>- GameText::Load(): Skipped unexpected language file \"" <<afile <<"\", for current game version \"" <<m_conf.GetGameVersion().id <<"\"!\n";
            }
        }


        //Locale sanity check
        if( m_conf.GetGameVersion().region == eGameRegion::Europe )
        {
            if( m_languages.find(eGameLanguages::english) != m_languages.end() &&
                m_languages.find(eGameLanguages::french)  != m_languages.end() &&
                m_languages.find(eGameLanguages::german)  != m_languages.end() &&
                m_languages.find(eGameLanguages::italian) != m_languages.end() &&
                m_languages.find(eGameLanguages::spanish) != m_languages.end() )
            {
                clog<<"<*>- Locale sanity check confirmed that all language files for the European game are present!\n";
            }
            else
            {
                throw std::runtime_error("GameText::Load(): Locale sanity check couldn't find all the expected text_*.str files!!\n");
            }
        }
        else if( m_conf.GetGameVersion().region == eGameRegion::NorthAmerica )
        {
            if( m_languages.find(eGameLanguages::english) != m_languages.end() )
                clog<<"<*>- Locale sanity check confirmed that all language files for the North American game are present!\n";
            else
                throw std::runtime_error("GameText::Load(): Locale sanity check couldn't find all the expected text_*.str files!!\n");
        }
        else if( m_conf.GetGameVersion().region == eGameRegion::Japan )
        {
            if( m_languages.find(eGameLanguages::japanese) != m_languages.end() )
                clog<<"<*>- Locale sanity check confirmed that all language files for the Japanese game are present!\n";
            else
                throw std::runtime_error("GameText::Load(): Locale sanity check couldn't find all the expected text_*.str files!!\n");
        }
        else
        {
            clog<<"<!>- Loaded unknown locale, with " <<m_languages.size() <<" known languages files..\n";
        }
    }

    void GameText::Write() const
    {
        for( const auto & alang : m_languages )
        {
            stringstream fname;
            fname << utils::TryAppendSlash(m_pmd2fsdir) << DirName_MESSAGE <<"/" << alang.second.GetTextFName();
            filetypes::WriteTextStrFile( fname.str(), alang.second.Strings() );
        }
    }

    std::string EscapeUnprintableCharacters(const std::string & src, const std::locale & loc)
    {
        std::string out;
        out.reserve(src.size() * 2);
        for( const char c : src )
        {
            if( c == 0 )
            {
                out.push_back('\\');
                out.push_back('0');
            }
            else if( c == 10 )
            {
                out.push_back('\\');
                out.push_back('n');
            }
            else
                out.push_back(c);
        }
        out.shrink_to_fit();
        return std::move(out);
    }

    std::string & ReplaceEscapedUnprintableCharacters(std::string & src, const std::locale & loc)
    {
        //regex       escapeseq("\\\\{1}([a-zA-Z0-9]+\\b)");
        //smatch      sm;
        //std::string outstr;
        //outstr.reserve(src.size());

        //for( auto itc = src.begin(); itc != src.end();  )
        //{

        //}


        //if( regex_match(src, sm, escapeseq) )
        //{




        //    for( const auto & match : sm )
        //    {
        //        string escapedval = match.str();
        //        if( escapedval.size() > 0 )
        //        {
        //            if( escapedval.front() == 'n' )
        //                outstr.push_back(10);
        //            else if( escapedval.front() == '0' )
        //                outstr.push_back(0);
        //            else
        //                clog<<"unsuported escape sequence " <<escapedval <<"!\n";
        //        }
        //        else if( escapedval.size() > 1 && escapedval.find('x') != string::npos )
        //        {
        //            stringstream num;
        //            num <<"0" << escapedval;
        //            uint16_t val = 0;
        //            num >> hex >> val;
        //            outstr.push_back(static_cast<char>(val));
        //        }
        //        else
        //        {}

        //    }
        //    outstr.shrink_to_fit();
        //    src = outstr;
        //}
        assert(false);
        
        return src;
    }

};
