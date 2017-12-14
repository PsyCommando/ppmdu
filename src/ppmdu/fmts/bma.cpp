#include "bma.hpp"

using namespace std;

namespace filetypes
{
    static const ContentTy CnTy_BMA{BMA_FileExt}; //Content ID handle

    template<class _init>
        class BMAParser
    {
        typedef _init init_t;
    public:
        BMAParser( _init & itbeg, _init itend )
            :m_itcur(itbeg), m_itend(itend)
        {}

        pmd2::TilesetBMAData operator()()
        {
            bma_header hdr;
            init_t itdatabeg = hdr.Read(m_itcur, m_itend);

            m_out.width  = hdr.width;
            m_out.height = hdr.height;
            m_out.unk1   = hdr.unk1;
            m_out.unk2   = hdr.unk2;
            m_out.unk3   = hdr.unk3;
            m_out.unk4   = hdr.unk4;
            m_out.unk5   = hdr.unk5;
            m_out.unk6   = hdr.unk6;
            m_out.unk7   = hdr.unk7;

            ProcessData(itdatabeg);

            return std::move(m_out);
        }

    private:

        void ProcessData(init_t itdatabeg)
        {
            //! #TODO
        }

    private:
        pmd2::TilesetBMAData m_out;
        init_t & m_itcur;
        init_t   m_itend;
    };

    class BMAWriter
    {
    public:
    };



    pmd2::TilesetBMAData ParseBMA(const std::string & fpath)
    {
        auto data = utils::io::ReadFileToByteVector(fpath);
        return BMAParser<decltype(data.begin())>(data.begin(), data.end())();
    }

    void WriteBMA(const std::string & destfpath, const pmd2::TilesetBMAData & bmadat)
    {
    }



//========================================================================================================
//  bma_rule
//========================================================================================================
    /*
        bma_rule
            Rule for identifying BMA content. With the ContentTypeHandler!
    */
    class bma_rule : public IContentHandlingRule
    {
    public:
        bma_rule(){}
        ~bma_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const { return CnTy_BMA; }

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
            return utils::CompareStrIgnoreCase(filext, BMA_FileExt);
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  bma_rule_registrator
//========================================================================================================
    /*
        bma_rule_registrator
            A small singleton that has for only task to register the bma_rule!
    */
    RuleRegistrator<bma_rule> RuleRegistrator<bma_rule>::s_instance;

};