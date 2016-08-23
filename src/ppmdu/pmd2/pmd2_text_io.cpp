#include <ppmdu/pmd2/pmd2_text.hpp>
#include <utils/utility.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

namespace pmd2
{
    const std::string DEF_TextFExt = "txt";
//==================================================================================
//  GameText
//==================================================================================
    /*
        ExportText
            Export text to a directory, into one text file per languages loaded.
    */
    void GameText::ExportText( const std::string & outdir )const
    {
        if( !AreStringsLoaded() )
            throw runtime_error( "GameText::ExportText(): No string data to export !" );

        for( const auto & lang : m_languages )
        {
            stringstream outfname;
            outfname <<utils::TryAppendSlash(outdir) <<GetGameLangName(lang.first) <<"." <<DEF_TextFExt;
            utils::io::WriteTextFileLineByLine( lang.second.Strings(), outfname.str(), locale(lang.second.GetLocaleString()) );
        }
    }
        
    /*
        ImportText
            Import text from a directory containing text files for each languages.
    */
    void GameText::ImportText( const std::string & indir  )
    {
        vector<string> files = utils::ListDirContent_FilesAndDirs( indir );

        for( const auto & afile : files )
        {
            if( utils::GetFileExtension(afile) == DEF_TextFExt )
            {
                std::string    basename    = std::move( utils::GetBaseNameOnly(afile) );
                eGameLanguages glang       = StrToGameLang(basename);
                const auto   * langdetails =  m_conf.GetLanguageFilesDB().GetByLanguage( glang );

                if( glang == eGameLanguages::Invalid || !langdetails )
                {
                    clog <<"<!>- GameText::ImportText(): Ignoring unexpected language file \"" <<afile <<"\"\n";
                    continue;
                }
                clog <<"<*>- Reading file : " << afile <<"\n";
                std::vector<string> strs = utils::io::ReadTextFileLineByLine( afile, std::locale(langdetails->GetLocaleString()) );
                m_languages.insert_or_assign( glang, 
                                                std::move(StringAccessor( std::move(strs), 
                                                                        std::move( StringsCatalog(*langdetails)))
                                                        )
                                            );
            }
        }
    }

};