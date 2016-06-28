#ifndef PMD2_TEXT_HPP
#define PMD2_TEXT_HPP
/*
pmd2_text.hpp
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_langconf.hpp>
#include <string>
#include <vector>
#include <map>

namespace pmd2
{


//==================================================================================
//  StringAccessor
//==================================================================================
    /****************************************************************************************
            A helper for accessing strings at offsets stored within a StringCatalog.
    ****************************************************************************************/
    class StringAccessor
    {
    public:
        typedef std::vector<std::string>::iterator       iterator;
        typedef std::vector<std::string>::const_iterator const_iterator;


        StringAccessor( std::vector<std::string> && strs, StringsCatalog && strcatalog )
            :m_cata(std::move(strcatalog)),m_strings( std::move(strs) )
        {}

        inline const std::string & operator[]( size_t index )const
        {
            return m_strings.at(index);
        }

        inline size_t         size()const  { return m_strings.size();  }
        inline bool           empty()const { return m_strings.empty(); }
        inline iterator       begin()      { return m_strings.begin(); }
        inline const_iterator begin()const { return m_strings.begin(); }
        inline iterator       end()        { return m_strings.end(); }
        inline const_iterator end()const   { return m_strings.end(); }


        inline std::vector<std::string>       & Strings()      { return m_strings; }
        inline const std::vector<std::string> & Strings()const { return m_strings; }

        //
        std::string * GetStringInBlock( eStrBNames blkty, size_t index ) 
        {
            const StringsCatalog::strbounds_t * pbounds = m_cata[blkty];

            if( !pbounds )
                return nullptr;

            if( (pbounds->end - pbounds->beg) >= index )
                throw std::runtime_error("GameText::StringAccessor::GetStringInBlock(): String index out of bound for block specified!" );

            if( m_strings.size() < pbounds->end )
                throw std::runtime_error("GameText::StringAccessor::GetStringInBlock(): Mismatch between string bounds for block " + StrBlocksNames[static_cast<unsigned int>(blkty)] + ", and offset in string file! The string bounds are outside the text_*.str file that was parsed!" );

            return &(m_strings[pbounds->beg + index]);
        }

        const std::string * GetStringInBlock( eStrBNames blkty, size_t index )const
        {
            return const_cast<StringAccessor*>(this)->GetStringInBlock(blkty, index);
        }

        std::pair<iterator,iterator> GetBoundsStringsBlock( eStrBNames blk )
        {
            const StringsCatalog::strbounds_t * pbounds = m_cata[blk];
            if( !pbounds )
                return std::make_pair( end(), end() );

            iterator blkbeg = m_strings.begin() + pbounds->beg;
            iterator blkend = m_strings.begin() + pbounds->end;
            return std::make_pair( blkbeg, blkend );
        }

        std::pair<const_iterator,const_iterator> GetBoundsStringsBlock( eStrBNames blk )const
        {
            const StringsCatalog::strbounds_t * pbounds = m_cata[blk];
            if( !pbounds )
                return std::make_pair( end(), end() );

            const_iterator blkbeg = m_strings.begin() + pbounds->beg;
            const_iterator blkend = m_strings.begin() + pbounds->end;
            return std::make_pair( blkbeg, blkend );
        }

        const std::string & GetLocaleString()const
        {
            return m_cata.GetLocaleString();
        }

    private:
        StringsCatalog           m_cata;
        std::vector<std::string> m_strings;
    };


//==================================================================================
//  GameText
//==================================================================================
    /****************************************************************************************
            This loads the game strings to memory for editing and access.
            It allows writing them back to the game or export them to text files.
    ****************************************************************************************/
    class GameText
    {
    public:
        typedef StringAccessor                      langstr_t;
        typedef std::map<eGameLanguages, langstr_t> langtbl_t;
        typedef langtbl_t::iterator                 iterator;
        typedef langtbl_t::const_iterator           const_iterator;

        GameText( const std::string & pmd2fsdir, eGameVersion version, eGameRegion gamereg, const std::string & gamelangxmlfile )
            :m_pmd2fsdir(pmd2fsdir), m_version(version),m_gamelang(gamelangxmlfile, version), m_region(gamereg)
        {}

        void                Load ();
        void                Write()const;
        inline bool         AreStringsLoaded()const  { return !m_languages.empty(); }
        inline eGameRegion  GetRegion()const         { return m_region; }

        /*
            GetGameLangLoader
                Provide access to the game language loader to query more details about
                 the loaded languages.
        */
        inline GameLanguageLoader       & GetGameLangLoader()      {return m_gamelang;}
        inline const GameLanguageLoader & GetGameLangLoader()const {return m_gamelang;}


        const std::string * GetLocaleString(eGameLanguages lang)const 
        {  
            auto foundlang = m_languages.find(lang);

            if( foundlang != m_languages.end() )
                return &(foundlang->second.GetLocaleString());
            else
                return nullptr;
        }

        /*  
            GetStrings
                Returns the language for the default language for this locale.
        */
        inline langstr_t * GetStrings()
        {
            if( !m_languages.empty() )
                return (&m_languages.begin()->second);
            else
                return nullptr;
        }

        inline const langstr_t * GetStrings()const
        {
            if( !m_languages.empty() )
                return (&m_languages.begin()->second);
            else
                return nullptr;
        }

        /*
            GetStrings
                Returns the StringsAccessor for a given language.
        */
        inline langstr_t * GetStrings( eGameLanguages lang ) 
        { 
            auto foundlang = m_languages.find(lang);

            if( foundlang != m_languages.end() )
                return &(foundlang->second);
            else
                return nullptr;
        }

        inline const langstr_t * GetStrings( eGameLanguages lang )const
        { 
            auto foundlang = m_languages.find(lang);

            if( foundlang != m_languages.end() )
                return &(foundlang->second);
            else
                return nullptr;
        }


        inline size_t           size()const                 { return m_languages.size(); }
        inline size_t           NbLanguagesLoaded()const    { return size(); }
        inline iterator         begin()                     { return m_languages.begin(); }
        inline const_iterator   begin()const                { return m_languages.begin(); }
        inline iterator         end()                       { return m_languages.end(); }
        inline const_iterator   end()const                  { return m_languages.end(); }

        // IO

        /*
            ExportText
                Export text to a directory, into one text file per languages loaded.
        */
        void ExportText    ( const std::string & outdir )const;
        
        /*
            ImportText
                Import text from a directory containing text files for each languages.
        */
        void ImportText( const std::string & indir  );


    private:
        std::string                         m_pmd2fsdir;
        eGameVersion                        m_version;
        eGameRegion                         m_region;
        GameLanguageLoader                  m_gamelang;
        std::map<eGameLanguages, langstr_t> m_languages;
    };

};

#endif