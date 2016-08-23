#ifndef MOVE_DATA_HPP
#define MOVE_DATA_HPP
/*
move_data.hpp
2015/03/12
psycommando@gmail.com
Description:
    Utilities for dealing with the move data from the PMD2 games.
*/
#include <cstdint>
#include <vector>
#include <ppmdu/pmd2/pmd2_text.hpp>

namespace pmd2{ namespace stats
{

//=====================================================================================
//  Constants
//=====================================================================================
    static const unsigned int NbMoves_EoS = 559; //Nb of moves stored by default in PMD2 Explorers of Sky

    enum struct eMvCat : uint8_t
    {
        Physical = 0,
        Special  = 1,
        Status   = 2,
    };

//=====================================================================================
//  Structs
//=====================================================================================
    struct MoveData
    {
        int16_t  basePower = 0;
        uint8_t  type      = 0;
        uint8_t  category  = static_cast<uint8_t>(eMvCat::Physical);
        uint16_t unk4      = 0;
        uint16_t unk5      = 0;
        uint8_t  basePP    = 0;
        uint8_t  unk6      = 0;
        uint8_t  unk7      = 0;
        uint8_t  accuracy  = 0;
        uint8_t  unk9      = 0;
        uint8_t  unk10     = 0;
        uint8_t  unk11     = 0;
        uint8_t  unk12     = 0;
        uint8_t  unk13     = 0;
        uint8_t  unk14     = 0;
        uint8_t  unk15     = 0;
        uint8_t  unk16     = 0;
        uint8_t  unk17     = 0;
        uint8_t  unk18     = 0;
        uint16_t moveID    = 0;
        uint16_t unk19     = 0;

        static const unsigned int DataLen = 26; //bytes
    };

//=====================================================================================
//  Classes
//=====================================================================================
    //#TODO: The move DB should abstract game specific details, so that we don't need 2 of them for EoS !!
    class MoveDB
    {
        friend class MovesXMLWriter;
        friend class MovesXMLParser;
    public:
        MoveDB(){ m_movesData.reserve(NbMoves_EoS); }

        inline std::size_t size()const        { return m_movesData.size(); }
        inline bool        empty()const       { return m_movesData.empty(); }
        inline void        reserve(size_t sz) { m_movesData.reserve(sz); }

        //The items are guaranteed to stay allocated as long as the object exists!
        //inline const MoveData & Move( uint16_t itemindex )const { return m_movesData[itemindex]; }
        //inline       MoveData & Move( uint16_t itemindex )      { return m_movesData[itemindex]; }

        inline const MoveData & operator[]( uint16_t index )const { return m_movesData[index]; }
        inline       MoveData & operator[]( uint16_t index )      { return m_movesData[index]; }

        inline void push_back( MoveData && move )                 { m_movesData.push_back( move ); }

    private:
        std::vector<MoveData> m_movesData;
    };


//=====================================================================================
//  Functions
//=====================================================================================
    /*
        Export move data to XML files.
    */
    void      ExportMovesToXML     ( const MoveDB                            & src1,
                                     const MoveDB                            * src2,
                                     const GameText                          * gtext,
                                     const std::string                       & destdir );

    /*
        Import move data from xml files.
    */
    void      ImportMovesFromXML   ( const std::string                  & srcdir, 
                                     MoveDB                             & out_mvdb1,
                                     MoveDB                             * out_mvdb2,
                                     GameText                           * gtext );
};};
#endif