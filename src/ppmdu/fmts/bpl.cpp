#include "bpl.hpp"

using namespace std;

namespace filetypes
{
    const ContentTy  CnTy_BPL{BPL_FileExt}; //Content ID handle

//============================================================================================
//  BPLParser
//============================================================================================

    template<class _fwdinit>
        class BPLParser
    {
        typedef _fwdinit init_t;
    public:
        BPLParser( init_t & itbeg, init_t itend)
            :m_itcur(itbeg), m_itend(itend)
        {}

        pmd2::TilesetPalette operator()()
        {
            bpl_header hdr;
            m_itcur = hdr.Read(m_itcur, m_itend);
            ParsePalettes(hdr.nbpalettes);
            ParseSecondSection(hdr.nbpalettes, hdr.unk1);
            return std::move(m_destpal);
        }

    private:
        void ParsePalettes(uint16_t nbpalettes)
        {
            for( size_t cntpal = 0; cntpal < nbpalettes; ++cntpal )
            {
                pmd2::TilesetPalette::pal_t curpal;
                //Each palettes is 15 colors in the file, so we'll just start putting in colors from the second slot in our 16 colors palette
                for( size_t cntcol = 1; cntcol < curpal.size(); ++cntcol )
                {
                    gimg::colorRGBX32 acol;
                    m_itcur = acol.ReadAsRawByte(m_itcur, m_itend);
                    curpal[cntcol] = std::move(acol);
                }
                m_destpal.mainpals.push_back(std::move(curpal));
            }
        }

        void ParseSecondSection(uint16_t nbpalettes, uint16_t unk1)
        {
            if( unk1 == 0 )
                return;

            const size_t nbentries = nbpalettes;
            for( size_t cntpal = 0; cntpal < nbentries; ++cntpal )
            {
                pmd2::TilesetPalette::dbaentry ent;
                m_itcur = utils::ReadIntFromBytes(ent.unk3, m_itcur, m_itend);
                m_itcur = utils::ReadIntFromBytes(ent.unk4, m_itcur, m_itend);
                m_destpal.dba.push_back(ent);
            }
            init_t itctablebeg = m_itcur;
            for( ;itctablebeg != m_itend ; )
            {
                gimg::colorRGBX32 acol;
                itctablebeg = acol.ReadAsRawByte(itctablebeg, m_itend);
                m_destpal.palette2.push_back( std::move(acol));
            }
            m_itcur = itctablebeg;
        }

    private:
        pmd2::TilesetPalette m_destpal;
        init_t & m_itcur;
        init_t   m_itend;
    };

//============================================================================================
//  BPLWriter
//============================================================================================

    class BPLWriter
    {
    public:
        BPLWriter()
        {
        }
    };

//============================================================================================
//  Functions
//============================================================================================
    pmd2::TilesetPalette ParseBPL(const std::string & fpath)
    {
        auto data = utils::io::ReadFileToByteVector(fpath);
        return BPLParser<decltype(data.begin())>(data.begin(), data.end())();
    }

    void WriteBPL( const std::string & destfpath, const pmd2::TilesetPalette & srcpal )
    {
    }


//========================================================================================================
//  bpl_rule
//========================================================================================================
    /*
        bpl_rule
            Rule for identifying BPL content. With the ContentTypeHandler!
    */
    class bpl_rule : public IContentHandlingRule
    {
    public:
        bpl_rule(){}
        ~bpl_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const { return CnTy_BPL; }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const                  { return m_myID; }
        virtual void              setRuleID( cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            //#TODO: Do something with this method, its just dumb and not accomplishing much right now! 
            ContentBlock cb;

            cb._startoffset          = 0;
            cb._endoffset            = std::distance( parameters._itdatabeg, parameters._itdataend );
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext )
        {
            return utils::CompareStrIgnoreCase(filext, BPL_FileExt);
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  bpl_rule_registrator
//========================================================================================================
    /*
        bpl_rule_registrator
            A small singleton that has for only task to register the bpl_rule!
    */
    RuleRegistrator<bpl_rule> RuleRegistrator<bpl_rule>::s_instance;
};
