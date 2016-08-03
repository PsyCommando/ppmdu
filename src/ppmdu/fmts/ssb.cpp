#include "ssb.hpp"
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <ppmdu/pmd2/pmd2_text.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
using namespace pmd2;
using namespace std;

namespace filetypes
{
//=======================================================================================
//  Structs
//=======================================================================================
    /***********************************************************************************
        routine_entry
            Script instruction group entry
    ***********************************************************************************/
    struct routine_entry
    {
        static const size_t LEN = 3 * sizeof(uint16_t);
        uint16_t begoffset = 0;
        uint16_t type      = 0;
        uint16_t unk2      = 0;

        template<class _outit>
            _outit Write(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(begoffset,  itwriteto);
            itwriteto = utils::WriteIntToBytes(type,       itwriteto);
            itwriteto = utils::WriteIntToBytes(unk2,       itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
            _init Read(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(begoffset, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(type,      itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(unk2,      itReadfrom, itpastend );
            return itReadfrom;
        }
    };

    /***********************************************************************************
        group_bounds
            Helper to contain the bounds of a group
    ***********************************************************************************/
    struct group_bounds
    {
        size_t datoffbeg;
        size_t datoffend; //One past the end of the group
    };

    /***********************************************************************************
        ssb_data_hdr
            Header of the data block of a script.
    ***********************************************************************************/
    struct ssb_data_hdr
    {
        static const size_t LEN     = 4; //bytes
        uint16_t            datalen = 0;   //Lenght of the data block
        uint16_t            nbgrps  = 0;   //Nb of groups in the data block

        template<class _outit>
            _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(datalen,      itwriteto);
            itwriteto = utils::WriteIntToBytes(nbgrps,       itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(datalen, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(nbgrps,  itReadfrom, itpastend );
            return itReadfrom;
        }
    };

//=======================================================================================
//  Typedefs
//=======================================================================================

    /***********************************************************************************
        lblinfo
            Info on a label's position and the kind of reference
    ***********************************************************************************/
    struct lblinfo
    {
        uint16_t lblid;
        enum struct eLblTy
        {
            JumpLbl,
            CaseLbl,

            NbTy,
            Invalid,
        };
        eLblTy lblty;
    };

    typedef uint32_t                            dataoffset_t;   //Offset in bytes within the "Data" block of the script
    typedef lblinfo                             lbl_t;          //Represents a label
    typedef unordered_map<dataoffset_t, lbl_t>  lbltbl_t;       //Contains the location of all labels
    typedef vector<routine_entry>               rawroutines_t;  //Raw group entries
    typedef deque<ScriptInstruction>            rawinst_t;      //Raw instruction entries

//=======================================================================================
//  Functions for handling parameter values
//=======================================================================================

    inline int16_t Convert14bTo16b(int16_t val)
    {
        return (val >= 0x4000)? (val | 0xFFFF8000) : (val & 0x3FFF);
    }

    //!#TODO: Double check this!!
    inline int16_t Convert16bTo14b(int16_t val)
    {
        if( (val/val) < 0)
            return (val & 0x3FFF) | 0x4000;     //If the value was negative, set the negative bit
        return val & 0x3FFF;
    }

    /*
        PrepareParameterValue
            Fun_022E48AC
            Used on 16 bits words passed to some functions, to turn them into 14 bits words
            Used for a lot of parameters!!!

    */
    int16_t PrepareParameterValue( int32_t R0 )
    {
        int32_t R1 = 0;
        if( (R0 & 0x4000) != 0 )
            R1 = R0 | 0xFFFF8000;
        else
            R1 = R0 & 0x3FFF;

        if( (R0 & 0x8000) != 0 )
        {
            R0 = R1 >> 7;
            R0 = R1 + static_cast<uint32_t>(static_cast<uint32_t>(R0) >> 0x18u); //Logical shift right, so handle as unsigned values
            R0 = R0 >> 8;
        }
        else
        {
            R0 = R1;
        }
        return  static_cast<int16_t>(R0);
    }

    /*
        Fun_022E48E0 
            Used on the first parameter of the MovePositionMark among other things. 
    */
    int16_t Fun_022E48E0( int32_t R0 )
    {
        int32_t R1 = 0;
        if( (R0 & 0x4000) != 0 )
            R1 = R0 | 0xFFFF8000;
        else
            R1 = R0 & 0x3FFF;
    
        if( (R0 & 0x8000) != 0 )
            R0 = R1;
        else
            R0 = (R1 << 8);
        return static_cast<int16_t>(R0);
    }


//=======================================================================================
//  Structs
//=======================================================================================
    /*
        raw_ssb_content
            Intermediate format for the transition between the 
            data parsed from the ssb file and the compiler/decompiler.
    */
    struct raw_ssb_content
    {
        Script::consttbl_t  constantstrings;
        Script::strtblset_t strings;
        lbltbl_t                      jumpoffsets;
        rawroutines_t                 rawroutines;
        rawinst_t                     rawinstructions;
        size_t                        datablocklen;         //Length of the datahdr + group table + instructions block
    };



//=======================================================================================
//  ScriptDecompiler
//=======================================================================================
    /*
        ScriptDecompiler
            Process script data from a SSB file into a more manageable format
            with several meta-instructions.
    */
    class ScriptDecompiler
    {
    public:

        ScriptDecompiler( raw_ssb_content && content, eOpCodeVersion opver, const LanguageFilesDB & langs )
            :m_rawdata(std::forward<raw_ssb_content>(content)), 
             m_rawinst(m_rawdata.rawinstructions), 
             m_routines(m_rawdata.rawroutines), 
             m_labels(m_rawdata.jumpoffsets), 
             m_constants(m_rawdata.constantstrings), 
             m_strings(m_rawdata.strings),
             m_datablocklen(m_rawdata.datablocklen),
             m_curdataoffset(0),
             m_curroutine(0),
             m_poutroutines(nullptr), 
             m_instfinder(opver),
             m_langs(langs),
             m_escapeforxml(true),
             m_bscriptdebug(false)
        {}

        /*-----------------------------------------------------------------------------
            operator()
                Process the data into the the sequence "destseq"
        -----------------------------------------------------------------------------*/
        void operator()( Script & destseq, bool escapeforxml = true, bool bscriptdebug = false )
        {
            m_bscriptdebug      = bscriptdebug;
            m_escapeforxml      = escapeforxml;
            m_poutroutines      = std::addressof( destseq.Routines() );
            m_grptblandathdrlen = ssb_data_hdr::LEN + (routine_entry::LEN * m_routines.size());
            m_curroutine        = 0;
            m_curdataoffset     = m_grptblandathdrlen;
            PrepareRoutines();

            //Iterate the raw instructions
            auto itend = m_rawinst.end();
            for( auto iti = m_rawinst.begin(); iti != itend; )
            {
                UpdateCurrentRoutine();
                HandleLabels();
                HandleInstruction( iti, itend );
            }

            PrepareStringsAndConstants();

            //Transfer the strings and constants over.
            destseq.ConstTbl()  = std::move(m_rawdata.constantstrings);
            destseq.StrTblSet() = std::move(m_rawdata.strings);
        }

        /*-----------------------------------------------------------------------------
            operator()
                Returns the processed script sequence
        -----------------------------------------------------------------------------*/
        inline Script operator()(bool escapeforxml, bool bscriptdebug )
        {
            Script dest;
            operator()(dest,escapeforxml, bscriptdebug);
            return std::move(dest);
        }

    private:

        //Temporary object, never copied or moved!!
        ScriptDecompiler(ScriptDecompiler&&)                  = delete;
        ScriptDecompiler(const ScriptDecompiler&)             = delete;
        ScriptDecompiler& operator=(ScriptDecompiler&&)       = delete;
        ScriptDecompiler& operator=(const ScriptDecompiler&)  = delete;

        /*-----------------------------------------------------------------------------
            CurRoutine
                Helper to access the current group
        -----------------------------------------------------------------------------*/
        inline ScriptInstrGrp & CurRoutine()
        {
            return (*m_poutroutines)[m_curroutine];
        }


        /*-----------------------------------------------------------------------------
            GetNextRealRoutineBeg
                Returns the next group/routine that's not an alias of the last!
        -----------------------------------------------------------------------------*/
        inline uint16_t GetNextRealRoutineBeg( size_t cntroutine, uint16_t curbeg )
        {
            for( ; cntroutine < m_routines.size(); ++cntroutine )
            {
                uint16_t beg = m_routines[cntroutine].begoffset;
                if( curbeg < beg )
                    return beg; //Return the next non-alias routine
            }
            return static_cast<uint16_t>(m_datablocklen / ScriptWordLen);
        }

        /*-----------------------------------------------------------------------------
            PrepareRoutines
                Init destination groups, and make a list of all the end offsets 
                for each groups
        -----------------------------------------------------------------------------*/
        void PrepareRoutines()
        {
            m_routineends.reserve(m_routines.size());

            size_t   cntroutine     = 0;
            uint16_t lastrtnbeg = 0;
            uint16_t lastrtnend = 0;
            for( ; cntroutine <  m_routines.size(); ++cntroutine )
            {
                const auto & rtn = m_routines[cntroutine];
                ScriptInstrGrp iroutine;
                iroutine.type = rtn.type;
                iroutine.unk2 = rtn.unk2;

                if(lastrtnbeg == rtn.begoffset )
                {
                    iroutine.isalias = true;
                }
                else
                {
                    iroutine.isalias = false;
                    lastrtnbeg = rtn.begoffset;
                    lastrtnend = GetNextRealRoutineBeg( cntroutine, rtn.begoffset );
                }
                m_poutroutines->push_back(std::move(iroutine));
                m_routineends.push_back(lastrtnend * ScriptWordLen);
            }
        }

        /*-----------------------------------------------------------------------------
            PrepareStringsAndConstants
                This is for escaping unprintable characters in the string data we got.
        -----------------------------------------------------------------------------*/
        void PrepareStringsAndConstants()
        {
            //Escape the characters in the constants only in the japanese version!
            eGameLanguages alang = m_langs.Languages().begin()->second.GetLanguage();
            if( alang == eGameLanguages::japanese )
            {
                for( auto & conststr : m_rawdata.constantstrings )
                {
                    //Constants are special in the japanese game. They're handled as regular text, so we shouldn't escape anything Shift-JIS
                    conststr = std::move(ProcessString(conststr, alang));
                }
            }
            
            //Escape the text in the strings
            for( auto & lang : m_rawdata.strings )
            {
                assert( !lang.second.empty() );
                const StringsCatalog * pcata = m_langs.GetByLanguage(lang.first);
                assert(pcata);
                std::locale curloc(pcata->GetLocaleString());
                for( auto & str : lang.second )
                {
                    str = (std::move(ProcessString(str, lang.first, curloc)));
                }
            }
        }

        /*-----------------------------------------------------------------------------
            ProcessString
        -----------------------------------------------------------------------------*/
        std::string ProcessString( const std::string & str, eGameLanguages lang, const std::locale & loc = std::locale::classic() )
        {
            return std::move( EscapeUnprintableCharacters(str, lang != eGameLanguages::japanese, m_escapeforxml, loc) );
        }

        /*-----------------------------------------------------------------------------
            HandleLabels
                Check if there's a jump location at the current data offset.
                If there is, place a label there.
        -----------------------------------------------------------------------------*/
        inline void HandleLabels()
        {
            auto itf = m_labels.find(m_curdataoffset);
            if( itf != m_labels.end() )
            {
                ScriptInstruction label;
                label.dbg_origoffset = m_curdataoffset / ScriptWordLen;;
                label.type  = eInstructionType::MetaLabel;
                label.value = itf->second.lblid;
                //Insert label
                CurRoutine().instructions.push_back(std::move(label));
            }
        }

        /*-----------------------------------------------------------------------------
            HandleInstruction
                Handle assembling meta-informations when neccessary.
                Also increases the data offset counter depending on the
                processed instruction's size.

                - iti   : Iterator to current instruction.
                - itend : Iterator to end of list of instruction.
        -----------------------------------------------------------------------------*/
        template<typename _init>
            void HandleInstruction( _init & iti, _init & itend )
        {
            ScriptInstruction curinst  = *iti;
            OpCodeInfoWrapper codeinfo = m_instfinder.Info(curinst.value);
            curinst.dbg_origoffset     = m_curdataoffset / ScriptWordLen;

            if(m_bscriptdebug)
            {
                if( m_instfinder.Version() == eOpCodeVersion::EoS && 
                   curinst.value == static_cast<uint16_t>(eScriptOpCodesEoS::BranchDebug) )
                    curinst.parameters.front() = (curinst.parameters.front() == 0)? 1 : 0 ; //Invert the boolean
            }

            if( codeinfo.HasReturnValue() )
                HandleCaseOwningCommand(iti, itend, curinst, codeinfo);
            else if( codeinfo.IsEntityAccessor() )
                HandleAccessor(iti, itend, curinst);
            else
            {
                m_curdataoffset += GetInstructionLen(curinst);
                CurRoutine().instructions.push_back(std::move(curinst));
                ++iti;
            }

            //switch(codeinfo.Category())
            //{
            //    case eCommandCat::OpWithReturnVal:
            //    case eCommandCat::EnterAdventure:
            //    case eCommandCat::ProcSpec:
            //    case eCommandCat::Switch:
            //    {
            //        HandleCaseOwningCommand(iti, itend, curinst, codeinfo);
            //        break;
            //    }
            //    case eCommandCat::EntityAccessor:
            //    {
            //        HandleAccessor(iti, itend, curinst);
            //        break;
            //    }
            //    default:
            //    {
            //        m_curdataoffset += GetInstructionLen(curinst);
            //        CurRoutine().instructions.push_back(std::move(curinst));
            //        ++iti;
            //    }
            //};
        }

        /*-----------------------------------------------------------------------------
            HandleCaseOwningCommand
                Handle Meta-Instructions that "owns" a list of case instructions 
                immediately after itself.

                - &iti     : Reference to iterator to current instruction.
                - &itend   : Reference to iterator to end of list of instruction.
                - &curinst : Reference to currently handled instruction.
                - &curinfo : Reference to information on current instruction.
        -----------------------------------------------------------------------------*/
        template<typename _init>
            void HandleCaseOwningCommand(_init & iti, _init & itend, ScriptInstruction & curinst, const OpCodeInfoWrapper & curinfo )
        {
            ScriptInstruction & outinst = curinst;
            size_t            totalsz = GetInstructionLen(curinst);
            OpCodeInfoWrapper codeinfo;
            
            //#1 Grab all "Case" that are right after
            bool iscase = false;
            do
            {
                ++iti;
                if( iti != itend )
                {
                    codeinfo = m_instfinder.Info(iti->value);
                    //iscase   = (codeinfo.Category() == eCommandCat::Case || codeinfo.Category() == eCommandCat::Default); 
                    if(codeinfo && (iscase = codeinfo.IsReturnHandler()))
                    {
                        totalsz += GetInstructionLen(*iti);
                        outinst.subinst.push_back(std::move(*iti));
                    }
                }

            }while( iti != itend && iscase );
               
            if( !(outinst.subinst.empty()) )
                outinst.type = curinfo.GetMyInstructionType();

            CurRoutine().instructions.push_back(std::move(outinst));
            m_curdataoffset += totalsz;
        }

        /*-----------------------------------------------------------------------------
            HandleAccessor
                Handle Meta-Instructions that "owns" a single instructions 
                immediately after itself.

                - &iti     : Reference to iterator to current instruction.
                - &itend   : Reference to iterator to end of list of instruction.
                - &curinst : Reference to currently handled instruction.
        -----------------------------------------------------------------------------*/
        template<typename _init>
            void HandleAccessor(_init & iti, _init & itend, ScriptInstruction & curinst)
        {
            ScriptInstruction & outinst = curinst;
            size_t            totalsz = GetInstructionLen(curinst);
            OpCodeInfoWrapper codeinfo;

            ++iti;
            if( iti != itend )
            {
                codeinfo = m_instfinder.Info(iti->value);
                if( codeinfo && codeinfo.IsAttribute() )
                {
                    outinst.type = eInstructionType::MetaAccessor;
                    totalsz += GetInstructionLen(*iti);
                    outinst.subinst.push_back(std::move(*iti));
                    ++iti;
                }
            }
            CurRoutine().instructions.push_back(std::move(outinst));
            m_curdataoffset += totalsz;
        }

        /*-----------------------------------------------------------------------------
            GetInstructionLen
                Calculate the length of the specified instruction in bytes as if it
                was stored as raw bytes in the script file.

                - &inst : Reference to currently handled instruction.
        -----------------------------------------------------------------------------*/
        inline size_t GetInstructionLen( const ScriptInstruction & inst )
        {
            if( inst.type == eInstructionType::MetaLabel )
                return 0;

            if(m_instfinder.Info(inst.value).NbParams() == -1)
                return ScriptWordLen + (inst.parameters.size() * ScriptWordLen) + ScriptWordLen; //Add one word for the nb of parameters!
            else
                return ScriptWordLen + (inst.parameters.size() * ScriptWordLen);
        }

        /*-----------------------------------------------------------------------------
            UpdateCurrentRoutine
                Picks the instruction "group" to place the raw instrutions into,
                based on the data offset we're currently parsing an instruction at.
        -----------------------------------------------------------------------------*/
        inline void UpdateCurrentRoutine()
        {
            if( m_curdataoffset >= m_routineends[m_curroutine] && m_curroutine < (m_routineends.size()-1) )
            {
                ++m_curroutine;
                //Increment a bit more if we have alias groups
                while( (*m_poutroutines)[m_curroutine].IsAliasOfPrevGroup() && m_curroutine < m_poutroutines->size() ) 
                    ++m_curroutine;
            }
            else if( m_curroutine >= m_routineends.size() )
            {
                clog <<"ScriptDecompiler::UpdateCurrentRoutine() : Instruction is out of expected bounds!\n";
                assert(false);
            }
        }

    private:
        raw_ssb_content                       m_rawdata;
        const LanguageFilesDB               & m_langs;
        deque<ScriptInstruction>            & m_rawinst;
        const rawroutines_t                 & m_routines;
        const lbltbl_t                      & m_labels;
        const Script::consttbl_t            & m_constants;
        const Script::strtblset_t           & m_strings;
        Script::grptbl_t                    * m_poutroutines;
        size_t                                m_curroutine;           //Group we currently output into
        size_t                                m_curdataoffset;      //Offset in bytes within the Data chunk
        vector<size_t>                        m_routineends;            //The offset within the data chunk where each groups ends
        size_t                                m_datablocklen;       //Length of the data block in bytes
        OpCodeClassifier                      m_instfinder;
        size_t                                m_grptblandathdrlen;  //The offset the instructions begins at. Important for locating group starts, and jump labels
        bool                                  m_escapeforxml;
        bool                                  m_bscriptdebug;       //whether we should make it so the debug paths are unlocked by default or not.
    };

//=======================================================================================
//  SSB Parser
//=======================================================================================
    /*
        SSB_Parser
            Parse SSB files.
    */
    template<typename _Init>
        class SSB_Parser
    {
    public:
        typedef _Init                    initer;
        typedef deque<ScriptInstruction> instcnt_t;

        SSB_Parser( _Init beg, _Init end, eOpCodeVersion scrver, eGameRegion scrloc, const LanguageFilesDB & langdat )
            :m_opfinder(scrver), m_beg(beg), m_end(end), m_cur(beg), m_scrversion(scrver), m_scrRegion(scrloc), m_lblcnt(0), 
             m_nbroutines(0), m_nbconsts(0), m_nbstrs(0),m_hdrlen(0),m_datablocklen(0), m_constlutbeg(0), m_stringlutbeg(0),
            m_datahdrgrouplen(0), m_langdat(langdat)
        {}


        /*******************************************************************************
            Parse
        *******************************************************************************/
        inline Script Parse(bool parseforxml = true, bool scriptdebug = false)
        {
            return std::move(ScriptDecompiler(std::move(ParseToRaw()), m_scrversion, m_langdat)(parseforxml, scriptdebug));
        }

        /*******************************************************************************
            ParseToRaw
        *******************************************************************************/
        raw_ssb_content ParseToRaw()
        {
#ifdef _DEBUG
            m_inputsz = std::distance( m_beg, m_end );
#endif
            raw_ssb_content transit;
            m_lblcnt = 0;
            ParseHeader();
            transit.rawroutines       = std::move(ParseRoutines());
            transit.constantstrings = std::move(ParseConstants());
            transit.strings         = std::move(ParseStrings());

            ParseCode();
            transit.jumpoffsets     = std::move(m_metalabelpos);
            transit.rawinstructions = std::move(m_rawinst);
            transit.datablocklen    = m_datablocklen;
            return std::move(transit); 
        }

    private:

        /*******************************************************************************
            ParseHeader
        *******************************************************************************/
        void ParseHeader()
        {
            uint16_t scriptdatalen = 0;
            uint16_t constdatalen  = 0;

            if( m_scrRegion == eGameRegion::NorthAmerica )
            {
                ssb_header hdr;
                m_hdrlen = ssb_header::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts     = hdr.nbconst;
                m_nbstrs       = hdr.nbstrs;
                scriptdatalen  = hdr.scriptdatlen;
                constdatalen   = hdr.consttbllen;
                m_stringblksSizes.push_back( make_pair( eGameLanguages::english, hdr.strtbllen * ScriptWordLen) );
            }
            else if( m_scrRegion == eGameRegion::Europe )
            {
                ssb_header_pal hdr;
                m_hdrlen = ssb_header_pal::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts    = hdr.nbconst;
                m_nbstrs      = hdr.nbstrs;
                scriptdatalen = hdr.scriptdatlen;
                constdatalen  = hdr.consttbllen;
                m_stringblksSizes.push_back( make_pair( eGameLanguages::english, hdr.strenglen * ScriptWordLen) );
                m_stringblksSizes.push_back( make_pair( eGameLanguages::french,  hdr.strfrelen * ScriptWordLen) );
                m_stringblksSizes.push_back( make_pair( eGameLanguages::german,  hdr.strgerlen * ScriptWordLen) );
                m_stringblksSizes.push_back( make_pair( eGameLanguages::italian, hdr.stritalen * ScriptWordLen) );
                m_stringblksSizes.push_back( make_pair( eGameLanguages::spanish, hdr.strspalen * ScriptWordLen) );
            }
            else if( m_scrRegion == eGameRegion::Japan )
            {
                ssb_header hdr;
                m_hdrlen = ssb_header::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts     = hdr.nbconst;
                m_nbstrs       = hdr.nbstrs;
                scriptdatalen  = hdr.scriptdatlen;
                constdatalen   = hdr.consttbllen;
                m_stringblksSizes.push_back( make_pair( eGameLanguages::japanese, hdr.strtbllen * ScriptWordLen) );
            }
            else
            {
                cout<<"SSB_Parser::ParseHeader(): Unknown script region!!\n";
                assert(false);
            }

            //Parse Data header
            ssb_data_hdr dathdr;
            m_cur = dathdr.ReadFromContainer( m_cur, m_end );

            //Compute offsets and etc
            m_datablocklen      = (dathdr.datalen * ScriptWordLen);
            m_constlutbeg       = m_hdrlen + m_datablocklen;         //Group table is included into the datalen
            m_stringlutbeg      = m_hdrlen + (scriptdatalen * ScriptWordLen) + (constdatalen*2);
            m_nbroutines        = dathdr.nbgrps;
            m_datahdrgrouplen   = ssb_data_hdr::LEN + (m_nbroutines * routine_entry::LEN);
#ifdef _DEBUG
            m_instblocklen      = m_datablocklen - m_datahdrgrouplen;
#endif
        }

        /*******************************************************************************
            ParseRoutines
        *******************************************************************************/
        inline rawroutines_t ParseRoutines()
        {
            rawroutines_t rgrps;
            rgrps.resize(m_nbroutines);
            //Grab all routines
            for( size_t cntroutine = 0; cntroutine < m_nbroutines; ++cntroutine )
            {
                m_cur = rgrps[cntroutine].Read( m_cur, m_end );
            }
            return std::move(rgrps);
        }

        /*******************************************************************************
            ParseData
        *******************************************************************************/
        //void ParseData( size_t foffset, uint16_t curop )
        //{
        //    stringstream ss;
        //    ss << "Unexpected word/invalid instruction ID (0x" 
        //       <<hex <<uppercase <<curop <<") at offset 0x"  <<foffset <<dec <<nouppercase <<".";
        //    throw std::runtime_error(ss.str());
        //}

        /*******************************************************************************
            ParseConstants
        *******************************************************************************/
        Script::consttbl_t ParseConstants()
        {
            if( !m_nbconsts )
                return Script::consttbl_t();

            const size_t strlutlen = (m_nbstrs * ScriptWordLen); // In the file, the offset for each constants in the constant table includes the 
                                                                 // length of the string lookup table(string pointers). Here, to compensate
                                                                 // we subtract the length of the string LUT from each pointer read.
            return std::move(ParseOffsetTblAndStrings<Script::consttbl_t>( m_constlutbeg, m_constlutbeg, m_nbconsts, strlutlen ));
        }

        /*******************************************************************************
            SetupLanguageTbl
        *******************************************************************************/
        //Setup all the correct languages in the string table
        inline void SetupLanguageTbl( Script::strtblset_t & out )
        {
            for( const auto & lg : m_stringblksSizes )
                out.emplace( lg.first, std::move(Script::strtbl_t()) ); //Make the entry for this language
        }

        /*******************************************************************************
            ParseStrings
        *******************************************************************************/
        Script::strtblset_t ParseStrings()
        {
            if( !m_nbstrs )
                return Script::strtblset_t();

            Script::strtblset_t out;
            SetupLanguageTbl(out);
            //Parse the strings for any languages we have
            size_t strparseoffset = m_stringlutbeg;
            size_t begoffset      = ( m_nbconsts != 0 )? m_constlutbeg : m_stringlutbeg;

            size_t cntlang = 0;
            //for( auto & lang : out )
            for( auto &block : m_stringblksSizes )
            {
                out[block.first] = std::move( ParseOffsetTblAndStrings<Script::strtbl_t>( strparseoffset, 
                                                                                     begoffset, 
                                                                                     m_nbstrs ));
                strparseoffset += block.second; //Add the size of the last block, so we have the offset of the next table
                begoffset      += block.second;
                ++cntlang;
            }
            return std::move(out);
        }

        /*******************************************************************************
            ParseOffsetTblAndStrings
                - lutbeg     : The offset of the beginning of the lookup tables.
                - ptrbaseoff : The the offset against which we add the offsets from the lookup table.
                - nbtoparse  : The nb of entries in the lookup table.
                - offsetdiff : this value will be subtracted from every ptr read in the table. 
                               Used by the constant table, since, each 16 bits offsets in its LuT include 
                               the length of the string LuT( nbstrings * 2, even with european games ).
        *******************************************************************************/
        template<class _ContainerT>
            _ContainerT ParseOffsetTblAndStrings( size_t lutbeg, size_t ptrbaseoff, uint16_t nbtoparse, long offsetdiff=0 )
        {
            _ContainerT strings;
            //Parse regular strings here
            initer itbaseoffs = std::next( m_beg, ptrbaseoff );
            initer itlut      = std::next( m_beg, lutbeg );
            initer itlutend   = std::next( itlut, (nbtoparse * ScriptWordLen) );
            
            assert( itbaseoffs != m_end );

            //Parse string table
            size_t cntstr = 0;
            for( ; cntstr < nbtoparse && itlut != itlutend; ++cntstr )
            {
                size_t   stroffset = utils::ReadIntFromBytes<uint16_t>( itlut, itlutend ) - offsetdiff; //Offset is in bytes this time!
                initer   itstr     = std::next( itbaseoffs, stroffset ); 
                strings.push_back( std::move(utils::ReadCStrFromBytes( itstr, m_end )) );
            }

            if(cntstr != nbtoparse)
            {
                assert(false);
                stringstream sstrer;
                sstrer << "SSB_Parser::ParseOffsetTblAndStrings(): Couldn't parse all " <<nbtoparse <<" string(s)! Only parsed " <<cntstr <<" before reaching end of data!!";
                throw std::runtime_error(sstrer.str());
            }

            return std::move(strings);
        }

        /*******************************************************************************
            ParseCode
                
        *******************************************************************************/
        void ParseCode()
        {
            //Iterate once through the entire code, regardless of groups, list all jump targets, and parse all operations
            const size_t instbeg        = m_hdrlen + ssb_data_hdr::LEN + (m_nbroutines * routine_entry::LEN);
            const size_t instend        = m_hdrlen + m_datablocklen;
#ifdef _DEBUG
            assert(m_inputsz >= instend);
#endif
            initer       itcollect      = std::next( m_beg, instbeg );
            initer       itdatabeg      = std::next( m_beg, m_hdrlen );
            initer       itdataend      = std::next( m_beg, instend );
            size_t       instdataoffset = 0; //Offset relative to the beginning of the data

            while( itcollect != itdataend )
            {
                uint16_t          curop      = utils::ReadIntFromBytes<uint16_t>( itcollect, itdataend );
                OpCodeInfoWrapper opcodedata = m_opfinder.Info(curop);

                if( opcodedata )
                    ParseCommand( instdataoffset, itcollect, itdataend, curop, opcodedata );
                else
                {
                    assert(false);
                    stringstream sstr;
                    sstr <<"SSB_Parser::ParseInstructionSequence() : Unknown Opcode at absolute file offset  " <<hex <<uppercase <<instdataoffset + instbeg <<"!";
                    throw std::runtime_error(sstr.str());
                }
                instdataoffset += ScriptWordLen; //Count instructions and data. Parameters are added by the called functions as needed
            }
        }

        /*******************************************************************************
            ParseCommand 
                Read and add command to the rawinstruction table!
        *******************************************************************************/
        void ParseCommand( size_t                   & foffset, 
                           initer                   & itcur, 
                           initer                   & itendseq, 
                           uint16_t                   curop, 
                           const OpCodeInfoWrapper  & codeinfo  )
        {
            ScriptInstruction inst;
            inst.type  = eInstructionType::Command;
            inst.value = curop;
            size_t cntparam = 0;
            size_t nbparams = 0;
            size_t paramlen = 0;

            if( codeinfo.NbParams() != 0 && itcur == itendseq )
                throw std::runtime_error("SSB_Parser::ParseCommand(): Not enough data left to parse any parameters!!");

            if( codeinfo.NbParams() > 0 )
            {
                nbparams = codeinfo.NbParams();
                paramlen = cntparam * ScriptWordLen;
            }
            else if( codeinfo.NbParams() == -1  )
            {
                // -1 param instructions use the next 16bits word to indicate the amount of parameters to parse
                nbparams = PrepareParameterValue( utils::ReadIntFromBytes<int16_t>(itcur,itendseq) ); //iterator is incremented here
                paramlen = ScriptWordLen + (nbparams * ScriptWordLen);
            }

            for( ; cntparam < nbparams && itcur != itendseq; ++cntparam )
                HandleParameter(cntparam, inst, codeinfo, itcur, itendseq);

            if( cntparam != nbparams )
            {
                stringstream sstrer;
                sstrer <<"\n<!>- Found instruction with not enough bytes left to assemble all its parameters at offset 0x" <<hex <<uppercase <<foffset <<dec <<nouppercase <<"\n";
                throw std::runtime_error(sstrer.str());
            }
            else
                m_rawinst.push_back(std::move(inst));

            foffset += paramlen;
        }

        /*******************************************************************************
        *******************************************************************************/
        inline void HandleParameter(size_t                    cntparam, 
                                    ScriptInstruction       & destinst, 
                                    const OpCodeInfoWrapper & codeinfo, 
                                    initer                  & itcur, 
                                    initer                  & itendseq )
        {
            destinst.parameters.push_back( utils::ReadIntFromBytes<uint16_t>(itcur, itendseq) );
            if( cntparam < codeinfo.ParamInfo().size() )
            {
                eOpParamTypes ptype = codeinfo.ParamInfo()[cntparam].ptype;
                CheckAndMarkJumps(destinst.parameters.back(), ptype );
            }
        }

        /*******************************************************************************
        *******************************************************************************/
        //Also updates the value of the opcode if needed
        inline void CheckAndMarkJumps( uint16_t & pval, eOpParamTypes ptype )
        {
            if( ptype == eOpParamTypes::InstructionOffset )
            {
                auto empres = m_metalabelpos.try_emplace( pval * ScriptWordLen, lbl_t{ m_lblcnt, lbl_t::eLblTy::JumpLbl} );
                
                pval = empres.first->second.lblid;
                if( empres.second )//Increment only if there was a new label added!
                    ++m_lblcnt;
            }
        }


    private:
        //Iterators
        initer              m_beg;
        initer              m_cur;
        initer              m_end;

        //TargetOutput
        //Script    m_out;
        eOpCodeVersion      m_scrversion; 
        eGameRegion         m_scrRegion;

        //Offsets and lengths
        size_t              m_hdrlen;               //in bytes //Length of the SSB header for the current version + region
        size_t              m_datahdrgrouplen;      //in bytes //Length of the Data header and the group table
        size_t              m_datablocklen;         //in bytes //Length of the data header + group table + instructions block. (the constant lookup table is right after)
        size_t              m_instblocklen;         //in bytes //Length of the Instructions block only
        size_t              m_constlutbeg;          //in bytes //Start of the lookup table for the constant strings ptrs
        size_t              m_stringlutbeg;         //in bytes //Start of strings lookup table for the strings
        vector<pair<eGameLanguages,size_t>> m_stringblksSizes;      //in bytes //The lenghts of all strings blocks for each languages
        
#ifdef _DEBUG
        size_t              m_inputsz;              //in bytes //The size of the input container
#endif

        //Nb of entries
        uint16_t            m_nbstrs;
        uint16_t            m_nbconsts;
        uint16_t            m_nbroutines; 

        //Group data
        //vector<routine_entry>  m_grps;   

        //Label Assignement
        uint16_t             m_lblcnt;       //Used to assign label ids!
        lbltbl_t             m_metalabelpos; //first is position from beg of data, second is info on label

        //Instructions
        instcnt_t           m_rawinst; //Instructions are parsed into this linear container
        OpCodeClassifier    m_opfinder;
        const LanguageFilesDB & m_langdat;
    };


//=======================================================================================
//  ScriptCompiler
//=======================================================================================

    class ScriptCompiler
    {
    public:
        ScriptCompiler( const Script & source, /*eGameRegion greg,*/ eOpCodeVersion opver, const LanguageFilesDB & langs )
            :m_src(source), /*m_region(greg),*/ m_opversion(opver), m_opinfo(opver), m_langs(langs)
        {}

        raw_ssb_content Compile()
        {
            ProcessInstructions();
            UpdateAllReferences();
            ProcessStringsAndConsts();
            return std::move(m_out);
        }

    private:
        void ProcessInstructions()
        {
            //Get all labels, and compute their offsets
            size_t curdataoffset = ssb_data_hdr::LEN + (m_src.Routines().size() * routine_entry::LEN);
            m_out.rawroutines.reserve(m_src.Routines().size());

            //Build group table as we go
            //Turn meta-instructions back into simple instructions
            uint16_t lastgrpbegoffset = 0;
            for( const auto & rtn : m_src.Routines() )
            {
                HandleRoutine(rtn, curdataoffset, lastgrpbegoffset);
            }
        }

        void HandleRoutine( const ScriptInstrGrp & rtn, size_t & curoffset, uint16_t & lastrtnbeg )
        {
            routine_entry curgrp;
            curgrp.type      = rtn.type;
            curgrp.unk2      = rtn.unk2;

            if( rtn.IsAliasOfPrevGroup() )
                curgrp.begoffset = lastrtnbeg;
            else
            {
                curgrp.begoffset = curoffset / ScriptWordLen;
                for( const auto & instr : rtn )
                {
                    HandleInstruction(instr, curoffset);
                }
            }
            lastrtnbeg = curgrp.begoffset;
            m_out.rawroutines.push_back(std::move(curgrp));
        }

        template<typename _InstrType>
            void HandleSubInstructions(const _InstrType & instr, size_t & curoffset );

        template<>
            void HandleSubInstructions<ScriptBaseInstruction>(const ScriptBaseInstruction &, size_t & )
        {
                //do nothing
        }

        template<>
            void HandleSubInstructions<ScriptInstruction>(const ScriptInstruction & instr, size_t & curoffset )
        {
            for( const auto & subinst : instr.subinst )
            {
                HandleInstruction(subinst, curoffset);
            }
        }

        template<typename _InstrType>
            void HandleInstruction( const _InstrType & instr, size_t & curoffset )
        {
            switch(instr.type)
            {
                case eInstructionType::MetaCaseLabel:
                case eInstructionType::MetaLabel:
                {
                    m_labeloffsets.emplace( instr.value, curoffset );
                    break;
                }
                case eInstructionType::MetaAccessor:
                case eInstructionType::MetaReturnCases:
                case eInstructionType::MetaSwitch:
                {
                    ScriptInstruction outinst;
                    outinst.value       = instr.value;
                    outinst.type        = eInstructionType::Command;
                    outinst.parameters  = instr.parameters; 
                    curoffset+= m_opinfo.CalcInstructionLen(outinst);
                    m_out.rawinstructions.push_back(std::move(outinst));
                    
                    //Write sub-instructions
                    HandleSubInstructions(instr,curoffset);
                    break;
                }
                case eInstructionType::Command:
                default:
                {
                    m_out.rawinstructions.push_back(instr);
                    curoffset+= m_opinfo.CalcInstructionLen(instr);
                }
            }
        }

        void UpdateAllReferences()
        {
            //Replace references to label into references to offsets
            for( auto & instr : m_out.rawinstructions )
            {
                if( instr.parameters.empty() )
                    continue;

                OpCodeInfoWrapper opinfo = m_opinfo.Info(instr.value);
                assert(opinfo);
                for( size_t ip = 0; ip < instr.parameters.size(); ++ip )
                {
                    if( ip < opinfo.ParamInfo().size() && 
                        opinfo.ParamInfo()[ip].ptype == eOpParamTypes::InstructionOffset )
                    {
                        instr.parameters[ip] = m_labeloffsets.at(instr.parameters[ip]) / static_cast<uint16_t>(ScriptWordLen); //Swap labelid for offset
                    }
                }

            }
        }

        void ProcessStringsAndConsts()
        {
            m_out.constantstrings.reserve(m_src.ConstTbl().size());
            //Un-Escape the characters in the strings and constants
            for( const auto & conststr : m_src.ConstTbl() )
            {
                m_out.constantstrings.push_back(std::move(ProcessString(conststr)));
            }

            for( const auto & lang : m_src.StrTblSet() )
            {
                assert( !lang.second.empty() );
                vector<string> curstrs;
                curstrs.reserve(lang.second.size());
                for( const auto & str : lang.second )
                {
                    const StringsCatalog * pcata = m_langs.GetByLanguage(lang.first);
                    if(pcata)
                        curstrs.push_back(std::move(ProcessString(str, pcata->GetLocaleString())));
                }
                m_out.strings.emplace( lang.first, std::forward<vector<string>>(curstrs));
            }
        }

        inline std::string ProcessString(std::string str, const std::string & locstr = "" )
        {
            ReplaceEscapedSequenceTest(str);
            return  std::move(str);
        }

    private:
        const Script            & m_src;
        const LanguageFilesDB   & m_langs;
        eGameRegion               m_region;
        eOpCodeVersion            m_opversion;
        OpCodeClassifier          m_opinfo;
        
        //State
        raw_ssb_content                         m_out;
        std::unordered_map<uint16_t, uint16_t>  m_labeloffsets; //Offsets of each labels
        
    };

//=======================================================================================
//  SSB Writer
//=======================================================================================

    class SSBWriterTofile
    {
        typedef ostreambuf_iterator<char> outit_t;
    public:
        SSBWriterTofile(const pmd2::Script & scrdat, eGameRegion gloc, eOpCodeVersion opver, const LanguageFilesDB & langdat)
            :m_scrdat(scrdat), m_scrRegion(gloc), m_opversion(opver), m_langdat(langdat), m_opinfo(opver)
        {
            if( m_scrRegion == eGameRegion::NorthAmerica || m_scrRegion == eGameRegion::Japan )
                m_stringblksSizes.resize(1,0);
            else if( m_scrRegion == eGameRegion::Europe )
                m_stringblksSizes.resize(5,0);
        }

        void Write(const std::string & scriptfile)
        {
            //!MAKE SURE THE SCRIPT CONTAINS WHAT IT SHOULD HERE!!
            m_outf.open(scriptfile, ios::binary | ios::out);
            if( !m_outf )
                throw std::runtime_error("SSBWriterTofile::Write(): Couldn't open file " + scriptfile);

            m_hdrlen         = 0;
            m_datalen        = 0; 
            m_nbstrings      = 0;
            m_constoffset    = 0;
            m_constblksize   = 0;
            m_stringblockbeg = 0;

            m_compiledsrc = std::move( ScriptCompiler(m_scrdat, m_opversion, m_langdat ).Compile() );

            if( m_scrRegion == eGameRegion::NorthAmerica || m_scrRegion == eGameRegion::Japan )
                m_hdrlen = ssb_header::LEN;
            else if( m_scrRegion == eGameRegion::Europe )
                m_hdrlen = ssb_header_pal::LEN;

            outit_t oit(m_outf);
            //#1 - Reserve data header 
            std::fill_n( oit, m_hdrlen + ssb_data_hdr::LEN, 0 );
            m_datalen += ssb_data_hdr::LEN; //Add to the total length immediately

            //#2 - Reserve group table
            std::fill_n( oit, m_scrdat.Routines().size() * routine_entry::LEN, 0 );

            //#3 - Pre-Alloc/pre-calc stuff
            CalcAndVerifyNbStrings();
            m_grps.reserve( m_scrdat.Routines().size() );
            m_datalen += m_scrdat.Routines().size() * routine_entry::LEN; //Add the group entries length!!
            //BuildLabelConversionTable();

            //#4 - Write code for each groups, constants, strings
            WriteCode();
            WriteConstants();
            WriteStrings();

            //#5 - Header and group table written last, since the offsets and sizes are calculated as we go.
            m_outf.seekp(0, ios::beg);
            outit_t ithdr(m_outf);
            WriteHeader    (ithdr);
            WriteGroupTable(ithdr);
        }

    private:

        //Since we may have several string blocks to deal with, we want to make sure they're all the same size.
        void CalcAndVerifyNbStrings()
        {
            size_t siz = 0;
            for( auto & cur : m_compiledsrc.strings )
            {
                if( siz == 0 )
                    siz = cur.second.size();
                else if( cur.second.size() != siz )
                    throw std::runtime_error("SSBWriterTofile::CalcAndVerifyNbStrings(): Size mismatch in one of the languages' string table!");
            }
            m_nbstrings = siz;
        }

        //void BuildLabelConversionTable()
        //{
        //    size_t curdataoffset = 0;

        //    for( const auto & rtn : m_scrdat.Routines() )
        //    {
        //        for( const auto & inst : rtn )
        //        {
        //            if( inst.type == eInstructionType::Command || inst.type == eInstructionType::Data )
        //                curdataoffset += ScriptWordLen + (ScriptWordLen * inst.parameters.size()); //Just count those
        //            else if( inst.type == eInstructionType::MetaLabel )
        //            {
        //                m_labeltbl.emplace( std::make_pair( inst.value, curdataoffset / ScriptWordLen  ) );
        //            }
        //        }
        //    }
        //}

        void WriteHeader( outit_t & itw )
        {
#ifdef _DEBUG
                assert(m_stringblksSizes.size()>= 1);
#endif // _DEBUG
            if( m_scrRegion == eGameRegion::NorthAmerica )
            {
                ssb_header hdr;
                hdr.nbconst      = m_compiledsrc.constantstrings.size();
                hdr.nbstrs       = m_nbstrings;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                hdr.strtbllen    = TryConvertToScriptLen(m_stringblksSizes.front());
                hdr.unk1         = static_cast<uint16_t>(std::ceil(1.5f * m_nbstrings)); //Unk1 is always 1.5 times the nb of strings rounded up!
                itw = hdr.WriteToContainer(itw);
            }
            else if( m_scrRegion == eGameRegion::Europe )
            {
#ifdef _DEBUG
                assert(m_stringblksSizes.size()== 5);
#endif // _DEBUG
                ssb_header_pal hdr;
                hdr.nbconst      = m_compiledsrc.constantstrings.size();
                hdr.nbstrs       = m_nbstrings;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                if( m_nbstrings != 0 )
                {
                    hdr.strenglen = TryConvertToScriptLen(m_stringblksSizes[0]);
                    hdr.strfrelen = TryConvertToScriptLen(m_stringblksSizes[1]);
                    hdr.strgerlen = TryConvertToScriptLen(m_stringblksSizes[2]);
                    hdr.stritalen = TryConvertToScriptLen(m_stringblksSizes[3]);
                    hdr.strspalen = TryConvertToScriptLen(m_stringblksSizes[4]);
                }
                else
                {
                    hdr.strenglen = 0;
                    hdr.strfrelen = 0;
                    hdr.strgerlen = 0;
                    hdr.stritalen = 0;
                    hdr.strspalen = 0;
                }
                itw = hdr.WriteToContainer(itw);
            }
            else if( m_scrRegion == eGameRegion::Japan )
            {
                //The japanese game makes no distinction between strings and constants, and just places everything in the constant slot
                ssb_header hdr;
                hdr.nbconst      = m_compiledsrc.constantstrings.size() + m_nbstrings;
                hdr.nbstrs       = 0;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                hdr.strtbllen    = 0;
                hdr.unk1         = 0; //Unk1 seems to be completely useless, so we're putting in random junk
                itw = hdr.WriteToContainer(itw);
            }

            ssb_data_hdr dathdr;
            dathdr.nbgrps  = m_compiledsrc.rawroutines.size();

            if( m_constoffset > 0 )
                dathdr.datalen = TryConvertToScriptLen(m_constoffset - m_hdrlen); //Const offset table isn't counted in this value, so we can't use m_datalen
            else 
                dathdr.datalen = TryConvertToScriptLen(m_datalen); //If no const table, we can set this to m_datalen
            itw = dathdr.WriteToContainer(itw);
        }

        //Write the table after the data header listing all the instruction groups
        void WriteGroupTable( outit_t & itw )
        {
#ifdef _DEBUG   //!#REMOVEME: For testing
            assert(!m_compiledsrc.rawroutines.empty()); //There is always at least one group!
#endif
            for( const auto & entry : m_compiledsrc.rawroutines )
            {
                itw = entry.Write(itw);
            }
        }


        void WriteCode()
        {
            //Write the content of the group
            for( const auto & inst : m_compiledsrc.rawinstructions )
                WriteInstruction(ostreambuf_iterator<char>(m_outf), inst);
        }

        void WriteInstruction( outit_t & itw, const ScriptInstruction & inst )
        {
            if( inst.type == eInstructionType::Command  )
            {
                OpCodeInfoWrapper codeinf = m_opinfo.Info(inst.value);
                itw = utils::WriteIntToBytes( inst.value, itw );
                m_datalen += ScriptWordLen;

                //If we're a -1 instruction, append the nb of instructions!!
                if( codeinf.NbParams() == -1 )
                {
                    itw = utils::WriteIntToBytes( static_cast<uint16_t>(inst.parameters.size()), itw );
                    m_datalen += ScriptWordLen;
                }

                //Append the paramters
                for( const auto & param : inst.parameters )
                {
                    itw = utils::WriteIntToBytes( param, itw );
                    m_datalen += ScriptWordLen;
                }
            }
            //else if( inst.type == eInstructionType::Data )
            //{
            //    itw = utils::WriteIntToBytes( inst.value, itw );
            //    m_datalen += ScriptWordLen;
            //}
            else
                assert(false); //!#TODO: Error handling!!
        }


        void WriteConstants()
        {
            if( m_compiledsrc.constantstrings.empty() )
                return;
            //**The constant pointer table counts as part of the script data, but not the constant strings it points to for some weird reasons!!**
            //**Also, the offsets in the tables include the length of the string ptr table!**
            const streampos befconsttbl = m_outf.tellp();
            m_constoffset = static_cast<size_t>(befconsttbl);   //Save the location where we'll write the constant ptr table at, for the data header
            
            const uint16_t  sizcptrtbl     = m_compiledsrc.constantstrings.size() * ScriptWordLen;
            const uint16_t  szstringptrtbl = m_nbstrings * ScriptWordLen;
            m_datalen += sizcptrtbl;    //Add the length of the table to the scriptdata length value for the header
            m_constblksize = WriteTableAndStrings( m_compiledsrc.constantstrings, szstringptrtbl); //The constant strings data is not counted in datalen!




            //
            //size_t cntconst = 0;
            //const streampos befconsttbl = m_outf.tellp();
            //m_constoffset = static_cast<size_t>(befconsttbl);   //Save the location where we'll write the constant ptr table

            ////reserve table, so we can write the offsets as we go
            //const uint16_t  sizcptrtbl  = m_scrdat.ConstTbl().size() * ScriptWordLen;
            //std::fill_n( itw, sizcptrtbl, 0 );
            //m_datalen += sizcptrtbl;    //Add the length of the table to the scriptdata length value for the header

            ////Write constant strings
            //const streampos befconstdata = m_outf.tellp();
            //for( const auto & constant : m_scrdat.ConstTbl() )
            //{
            //    //Write offset in table 
            //    streampos curpos = m_outf.tellp();
            //    uint16_t curstroffset = (curpos - befconsttbl) / ScriptWordLen;

            //    m_outf.seekp( static_cast<size_t>(befconsttbl) + (cntconst * ScriptWordLen), ios::beg ); //Seek to the const ptr tbl
            //    *itw = curstroffset;            //Add offset to table
            //    m_outf.seekp( curpos, ios::beg ); //Seek back at the position we'll write the string at

            //    //write string
            //    //!#TODO: Convert escaped characters??
            //    itw = std::copy( constant.begin(), constant.end(), itw );
            //    *itw = '\0';
            //    ++itw;
            //    ++cntconst;
            //}

            ////Add some padding bytes if needed (padding is counted in the block's length)
            //utils::AppendPaddingBytes(itw, m_outf.tellp(), ScriptWordLen);

            ////Calculate the size of the constant strings data
            //m_constblksize = m_outf.tellp() - befconstdata;
        }

        /*
            WriteStrings
                Write the strings blocks
        */
        void WriteStrings()
        {
            if( m_compiledsrc.strings.empty() )
                return;

            size_t          cntstrblk       = 0;
            const streampos befstrptrs      = m_outf.tellp();
            const uint16_t  lengthconstdata = (m_compiledsrc.constantstrings.size() * ScriptWordLen) + m_constblksize; //The length of the constant ptr tbl and the constant data!
            const uint16_t  szstringptrtbl = m_nbstrings * ScriptWordLen;
            m_stringblockbeg = static_cast<size_t>(befstrptrs); //Save the starting position of the string data, for later

            if( !m_compiledsrc.strings.empty() && m_compiledsrc.strings.size() != m_stringblksSizes.size() )
            {
#ifdef _DEBUG
                assert(false);
#endif
                throw std::runtime_error("SSBWriterToFile::WriteStrings(): Mismatch in expected script string blocks to ouput!!");
            }

            for( const auto & strblk : m_compiledsrc.strings )
            {
                //Write each string blocks and save the length of the data into our table for later. 
                //**String block sizes include the ptr table!**
                m_stringblksSizes[cntstrblk] = WriteTableAndStrings( strblk.second, lengthconstdata ) + szstringptrtbl; //We need to count the offset table too!!
                ++cntstrblk;
            }
        }


        /*
            WriteTableAndStrings
                Writes a string block, either the constants' strings or strings' strings
                Returns the length in bytes of the string data, **not counting the ptr table!**
        */
        template<class _CNT_T>
            size_t WriteTableAndStrings(
                                         const _CNT_T & container,              //What contains the strings to write(std container needs begin() end() size() and const_iterator)
                                         size_t         ptrtbloffsebytes = 0 ) //Offset in **bytes** to add to all ptrs in the ptr table
        {
            size_t          cntstr     = 0;
            const streampos befptrs    = m_outf.tellp();
            const uint16_t  sizcptrtbl = (container.size() * ScriptWordLen);
            
            //Reserve pointer table so we can write there as we go
            std::fill_n( ostreambuf_iterator<char>(m_outf), sizcptrtbl, 0 );

            //Write strings
            const streampos befdata = m_outf.tellp();
            for( const auto & str : container )
            {
                //Write offset in table 
                streampos curpos = m_outf.tellp();

                m_outf.seekp( static_cast<size_t>(befptrs) + (cntstr * ScriptWordLen), ios::beg ); //Seek to the const ptr tbl
                utils::WriteIntToBytes<uint16_t>( (ptrtbloffsebytes + (curpos - befptrs)), ostreambuf_iterator<char>(m_outf) );            //Add offset to table
                m_outf.seekp( curpos, ios::beg ); //Seek back at the position we'll write the string at

                //write string
                std::copy( str.begin(), str.end(), ostreambuf_iterator<char>(m_outf) );
                m_outf.put('\0'); //Append zero
                ++cntstr;
            }
            //Add some padding bytes if needed (padding is counted in the block's length)
            utils::AppendPaddingBytes(ostreambuf_iterator<char>(m_outf), m_outf.tellp(), ScriptWordLen);

            //Return the size of the constant strings data
            return m_outf.tellp() - befdata;
        }


        /*
            TryConvertToScriptLen
                This will divide the size/offset in bytes by 2, and validate if the result too big for the 16 bits of a word. 
                Throws an exception in that case! Otherwise, just returns the value divided by 2
        */
        inline uint16_t TryConvertToScriptLen( const streampos & lengthinbytes )
        {
            const uint32_t scrlen = lengthinbytes / ScriptWordLen;
            if( scrlen > std::numeric_limits<uint16_t>::max() )
                throw std::runtime_error("SSBWriterToFile::TryConvertToScriptLen(): Constant block size exceeds the length of a 16 bits word!!");
            return static_cast<uint16_t>(scrlen);
        }

    private:
        const pmd2::Script & m_scrdat;
        uint16_t            m_hdrlen; 

        size_t              m_nbstrings;
        vector<uint16_t>    m_stringblksSizes;     //in bytes //The lenghts of all strings blocks for each languages
        uint16_t            m_constblksize;        //in bytes //The length of the constant data block

        size_t              m_datalen;             //in bytes //Length of the Data block in bytes
        size_t              m_constoffset;         //in bytes //Start of the constant block from  start of file
        size_t              m_stringblockbeg;      //in bytes //Start of strings blocks from  start of file
        vector<routine_entry> m_grps;

        unordered_map<uint16_t,uint16_t> m_labeltbl; //First is label ID, second is label offset in words

        eOpCodeVersion m_opversion; 
        eGameRegion    m_scrRegion;

        ofstream       m_outf;
        raw_ssb_content m_compiledsrc;      //Source of the compiled data
        const LanguageFilesDB & m_langdat;
        OpCodeClassifier m_opinfo;
    };

//=======================================================================================
//  Functions
//=======================================================================================

    /*
        ParseScript
    */
    pmd2::Script ParseScript(const std::string & scriptfile, eGameRegion gloc, eGameVersion gvers, const LanguageFilesDB & langdat, bool escapeforxml, bool bscriptdebug )
    {
        vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(scriptfile)) );
        eOpCodeVersion opvers = GameVersionToOpCodeVersion(gvers);

        if( opvers == eOpCodeVersion::Invalid )
            throw std::runtime_error("ParseScript(): Wrong game version!!");

        Script tmpscr = std::move( SSB_Parser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), opvers, gloc, langdat).Parse(escapeforxml, bscriptdebug) );
        tmpscr.SetName( utils::GetBaseNameOnly(scriptfile) );
        return std::move(tmpscr);
    }

    /*
        WriteScript
    */
    void WriteScript( const std::string & scriptfile, const pmd2::Script & scrdat, eGameRegion gloc, eGameVersion gvers, const LanguageFilesDB & langdata )
    {
        eOpCodeVersion opvers = GameVersionToOpCodeVersion(gvers);
        if( opvers == eOpCodeVersion::Invalid )
            throw std::runtime_error("ParseScript(): Wrong game version!!");

        SSBWriterTofile(scrdat, gloc, opvers, langdata).Write(scriptfile);
    }


    //
    //Script::strtblset_t LoadSBStrings(const std::string & scriptfile, eGameRegion gloc, eGameVersion gvers)
    //{
    //    vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(scriptfile)) );
    //    eOpCodeVersion opvers = GameVersionToOpCodeVersion(gvers);

    //    if( opvers == eOpCodeVersion::Invalid )
    //        throw std::runtime_error("ParseScript(): Wrong game version!!");

    //    return std::move( SSB_Parser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), opvers, gloc).ParseStrings() );
    //}

    ////
    //Script::consttbl_t LoadSBConstants(const std::string & scriptfile, eGameRegion gloc, eGameVersion gvers)
    //{
    //    vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(scriptfile)) );
    //    eOpCodeVersion opvers = GameVersionToOpCodeVersion(gvers);

    //    if( opvers == eOpCodeVersion::Invalid )
    //        throw std::runtime_error("ParseScript(): Wrong game version!!");

    //    return std::move( SSB_Parser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), opvers, gloc).ParseConsts() );
    //}

};
