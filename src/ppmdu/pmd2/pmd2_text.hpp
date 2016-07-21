#ifndef PMD2_TEXT_HPP
#define PMD2_TEXT_HPP
/*
pmd2_text.hpp
*/
#include <ppmdu/pmd2/pmd2.hpp>
//#include <ppmdu/pmd2/pmd2_langconf.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace pmd2
{
//
//
//(const std::string & src, eGameLanguages glang, const std::locale & loc)




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

        inline size_t         size()const  { return m_strings.size();  }
        inline bool           empty()const { return m_strings.empty(); }
        inline iterator       begin()      { return m_strings.begin(); }
        inline const_iterator begin()const { return m_strings.begin(); }
        inline iterator       end()        { return m_strings.end(); }
        inline const_iterator end()const   { return m_strings.end(); }

        inline const std::string              & operator[]( size_t index )const     { return m_strings.at(index); }
        inline std::string                    & operator[]( size_t index )          { return m_strings.at(index); }
        inline std::vector<std::string>       & Strings()                           { return m_strings; }
        inline const std::vector<std::string> & Strings()const                      { return m_strings; }

        std::string * GetStringIfBlockExists(eStringBlocks blk, size_t index)
        {
            StringsCatalog::iterator itf;
            if( IsBlockLoaded( blk, itf ) && 
               (itf->second.beg + index) < itf->second.end && 
               (itf->second.beg + index) < m_strings.size() )
            {
                return &(m_strings[(itf->second.beg + index)]); 
            }
            else
                return nullptr;
        }

        inline const std::string * GetStringIfBlockExists(eStringBlocks blk, size_t index)const 
        {
            return const_cast<StringAccessor*>(this)->GetStringIfBlockExists(blk,index);
        }

        /*
            IsBlockLoaded 
                Finds if a block is loaded. 
                    Optionally can return as parameter the iterator to the found block to save time!
        */
        inline bool IsBlockLoaded( eStringBlocks blk )const
        {
            return m_cata.find(blk) != m_cata.end();
        }

        inline bool IsBlockLoaded( eStringBlocks blk, StringsCatalog::iterator & out_found )
        {
            return (out_found = m_cata.find(blk)) != m_cata.end();
        }

        inline bool IsBlockLoaded( eStringBlocks blk, StringsCatalog::const_iterator & out_found )const
        {
            return const_cast<StringAccessor*>(this)->IsBlockLoaded(blk,out_found);
        }

        inline bool IsWithinBounds(eStringBlocks blk, size_t index)const
        {
            return (m_cata[blk].end - m_cata[blk].beg) < index;
        }

        //
        std::string & GetStringInBlock( eStringBlocks blkty, size_t index ) 
        {
            const strbounds_t & bounds = m_cata[blkty];
            if( !IsWithinBounds( blkty, index) )
                throw std::runtime_error("GameText::StringAccessor::GetStringInBlock(): String index out of bound for block specified!" );
            if( m_strings.size() < bounds.end )
                throw std::runtime_error("GameText::StringAccessor::GetStringInBlock(): Mismatch between string bounds for block " + StringBlocksNames[static_cast<unsigned int>(blkty)] + ", and offset in string file! The string bounds are outside the text_*.str file that was parsed!" );
            return (m_strings[bounds.beg + index]);
        }

        inline const std::string & GetStringInBlock( eStringBlocks blkty, size_t index )const
        {
            return const_cast<StringAccessor*>(this)->GetStringInBlock(blkty, index);
        }

        std::pair<iterator,iterator> GetBoundsStringsBlock( eStringBlocks blk )
        {
            iterator blkbeg = m_strings.begin() + m_cata[blk].beg;
            iterator blkend = m_strings.begin() + m_cata[blk].end;
            return std::make_pair( blkbeg, blkend );
        }

        inline std::pair<const_iterator,const_iterator> GetBoundsStringsBlock( eStringBlocks blk )const
        {
            return std::move(const_cast<StringAccessor*>(this)->GetBoundsStringsBlock(blk));
        }

        inline size_t GetNbStringsInBlock(eStringBlocks blk)const
        {
            return (m_cata[blk].end - m_cata[blk].beg);
        }

        inline const std::string & GetLocaleString()const
        {
            return m_cata.GetLocaleString();
        }

        inline const std::string & GetTextFName()const
        {
            return m_cata.GetStrFName();
        }

        inline eGameLanguages GetLanguage()const
        {
            return m_cata.GetLanguage();
        }

    private:

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
        typedef StringAccessor                                langstr_t;
        typedef std::unordered_map<eGameLanguages, langstr_t> langtbl_t;
        typedef langtbl_t::iterator                           iterator;
        typedef langtbl_t::const_iterator                     const_iterator;

        GameText( const std::string & pmd2fsdir, const ConfigLoader & conf )
            :m_pmd2fsdir(pmd2fsdir), m_conf(conf)
        {}

        // ----------------------------------
        //  Language Data IO
        // ----------------------------------
        void Load ();
        void Write()const;

        // ----------------------------------
        //  Text IO
        // ----------------------------------
        /*
            ExportText
                Export text to a directory, into one text file per languages loaded.
        */
        void ExportText( const std::string & outdir )const;
        
        /*
            ImportText
                Import text from a directory containing text files for each languages.
        */
        void ImportText( const std::string & indir );
        
        // ----------------------------------
        //  Info
        // ----------------------------------
        /*
            GetGameLangLoader
                Provide access to the game language loader to query more details about
                 the loaded languages.
        */
        inline const GameVersionInfo & GetGameVersionInfo()const { return m_conf.GetGameVersion(); }
        inline eGameRegion             GetRegion()const          { return m_conf.GetGameVersion().region; }
        const std::string            * GetLocaleString(eGameLanguages lang)const 
        {  
            auto foundlang = m_languages.find(lang);

            if( foundlang != m_languages.end() )
                return &(foundlang->second.GetLocaleString());
            else
                return nullptr;
        }
        inline bool                 AreStringsLoaded()const                { return !m_languages.empty(); }
        inline bool                 IsLangLoaded(eGameLanguages lang)const { return m_languages.find(lang) != m_languages.end(); }
        std::vector<eGameLanguages> GetLoadedLanguagesIDs()const
        {
            std::vector<eGameLanguages> dest;
            dest.reserve(m_languages.size());
            for( const auto & alang : m_languages )
                dest.push_back(alang.first);
            return std::move(dest);
        }
        inline size_t GetNbStringsInBlock(eGameLanguages lang, eStringBlocks blk)const  { return GetLang(lang)->second.GetNbStringsInBlock(blk); }
        inline size_t GetNbStrings(eGameLanguages lang)const                            { return GetLang(lang)->second.size(); }

        // ----------------------------------
        //  Stuff for Range Based Iteration
        // ----------------------------------
        inline size_t           size()const                 { return m_languages.size(); }
        inline size_t           NbLanguagesLoaded()const    { return size(); }
        inline iterator         begin()                     { return m_languages.begin(); }
        inline const_iterator   begin()const                { return m_languages.begin(); }
        inline iterator         end()                       { return m_languages.end(); }
        inline const_iterator   end()const                  { return m_languages.end(); }

        // --------------------------
        //  Default Language Access
        // --------------------------
        inline StringAccessor       & GetDefaultLanguage()          { return GetLang(GetDefaultLanguageID())->second; }
        inline const StringAccessor & GetDefaultLanguage()const     { return const_cast<GameText*>(this)->GetDefaultLanguage(); }
        inline eGameLanguages         GetDefaultLanguageID()const   { return m_conf.GetGameVersion().defaultlang; }
        
        // -----------------------------------
        //  Access a Language's String Table
        // -----------------------------------
        /*  
            GetStrings
                No params => Returns the language for the default language for this locale.
                Langname  => Returns the language specified
        */
        inline iterator             GetStrings()                            { return GetLang(GetDefaultLanguageID()); }
        inline const const_iterator GetStrings()const                       { return const_cast<GameText*>(this)->GetStrings(); }
        inline iterator             GetStrings( eGameLanguages lang )       { return m_languages.find(lang); }
        inline const const_iterator GetStrings( eGameLanguages lang )const  { return m_languages.find(lang); }

        // ------------------------
        //  String Direct Access
        // ------------------------
        /*
            GetString
                Get a string by index in a language's entire string table.
                **This assumes that the language and block exists, and will throw if it doesn't.**
        */
        inline const std::string & GetString( eGameLanguages lang, size_t index )const {return const_cast<GameText*>(this)->GetString(lang,index);}
        inline std::string       & GetString( eGameLanguages lang, size_t index )
        {
            auto itf = GetLang(lang);
                throw std::out_of_range("GameText::GetString(): String index specified is out of bounds!");
            return (itf->second[index]);
        }

        /*
            GetString
                Get a string by index in a language's string block. The index is relative to the
                string block, and not the entire string table.
                **This assumes that the language and block exists, and will throw if it doesn't.**
        */
        inline const std::string & GetString( eGameLanguages lang, eStringBlocks blk, size_t index )const {return const_cast<GameText*>(this)->GetString(lang,blk,index);}
        inline std::string       & GetString( eGameLanguages lang, eStringBlocks blk, size_t index )
        {
            auto itf = GetLang(lang);
            if( itf->second.IsWithinBounds(blk,index) )
                throw std::out_of_range("GameText::GetString(): String index specified is out of bounds of the block!");
            return itf->second.GetStringInBlock(blk,index);
        }

        /*
            TryGetString 
                Same as GetString, except this one won't throw
                exception if the string or language doesn't exist.
                It will instead return a nullptr.
        */
        inline const std::string * TryGetString( eGameLanguages lang, size_t index )const {return const_cast<GameText*>(this)->TryGetString(lang,index);}
        inline std::string       * TryGetString( eGameLanguages lang, size_t index )
        {
            auto itf = m_languages.find(lang);
            if( itf == end() || index >= itf->second.size() )
                return nullptr;
            return &(itf->second[index]);
        }

        /*
            TryGetString 
                Same as GetString, except this one won't throw
                exception if the string, string block, or language doesn't exist.
                It will instead return a nullptr.
        */
        inline const std::string * TryGetString( eGameLanguages lang, eStringBlocks blk, size_t index )const { return const_cast<GameText*>(this)->TryGetString(lang,blk,index); }
        inline std::string       * TryGetString( eGameLanguages lang, eStringBlocks blk, size_t index )
        {
            auto itf = m_languages.find(lang);
            if( itf == end() )
                return nullptr;
            return itf->second.GetStringIfBlockExists(blk,index);
        }

    private:
        inline const_iterator GetLang(eGameLanguages lang)const
        {
            const_iterator itf = m_languages.find(lang);
            if( itf == m_languages.end() )
                throw std::out_of_range("GameText::GetString(): Language specified not loaded!");
            return itf;
        }
        inline iterator GetLang(eGameLanguages lang)
        {
            iterator itf = m_languages.find(lang);
            if( itf == m_languages.end() )
                throw std::out_of_range("GameText::GetString(): Language specified not loaded!");
            return itf;
        }

    private:
        std::string          m_pmd2fsdir;
        const ConfigLoader & m_conf;
        langtbl_t            m_languages;
    };

};

#endif