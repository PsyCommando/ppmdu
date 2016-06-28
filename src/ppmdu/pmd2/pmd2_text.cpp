#include "pmd2_text.hpp"
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <utils/utility.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <utils/poco_wrapper.hpp>
using namespace std;

namespace pmd2
{

    void GameText::Load()
    {
        stringstream Dirname;
        Dirname << utils::TryAppendSlash(m_pmd2fsdir) << DirName_MESSAGE;// << "/" << m_gameTextFName;

        //Load each language files in the directory!
        vector<string> files = utils::ListDirContent_FilesAndDirs( Dirname.str() );

        for( const auto & afile : files )
        {
            if( utils::GetFileExtension(afile) == filetypes::TextStr_FExt )
            {
                for( unsigned int cntlang = 0; cntlang < GameLanguagesSuffixes.size(); ++cntlang )
                {
                    if( GameLanguagesSuffixes[cntlang].front() ==  utils::GetBaseNameOnly(afile).back() )
                    {
                        langstr_t mylang( std::move(filetypes::ParseTextStrFile(afile)),
                                          std::move(m_gamelang.FindAllBlockOffsets(utils::GetFilename(afile))) );

                        m_languages.emplace( static_cast<eGameLanguages>(cntlang), 
                                             std::move(mylang) );
                    }
                }

            }
        }


        //Locale sanity check
        if( m_region == eGameRegion::Europe )
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
        else if( m_region == eGameRegion::NorthAmerica )
        {
            if( m_languages.find(eGameLanguages::english) != m_languages.end() )
                clog<<"<*>- Locale sanity check confirmed that all language files for the North American game are present!\n";
            else
                throw std::runtime_error("GameText::Load(): Locale sanity check couldn't find all the expected text_*.str files!!\n");
        }
        else if( m_region == eGameRegion::Japan )
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
            fname << utils::TryAppendSlash(m_pmd2fsdir) << DirName_MESSAGE <<"/" << FName_TextPref <<"_" << GameLanguagesSuffixes[static_cast<size_t>(alang.first)] <<"." <<filetypes::TextStr_FExt;
            filetypes::WriteTextStrFile( fname.str(), alang.second.Strings() );
        }
    }

};
