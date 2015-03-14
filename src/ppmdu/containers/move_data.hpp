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

namespace pmd2{ namespace stats
{
    static const unsigned int NbMoves_EoS = 559; 

//
//
//
    struct MoveData
    {
        uint16_t unk1   = 0;
        uint8_t  unk2   = 0;
        uint8_t  unk3   = 0;
        uint16_t unk4   = 0;
        uint16_t unk5   = 0;
        uint8_t  basePP = 0;
        uint8_t  unk6   = 0;
        uint8_t  unk7   = 0;
        uint8_t  unk8   = 0;
        uint8_t  unk9   = 0;
        uint8_t  unk10  = 0;
        uint8_t  unk11  = 0;
        uint8_t  unk12  = 0;
        uint8_t  unk13  = 0;
        uint8_t  unk14  = 0;
        uint8_t  unk15  = 0;
        uint8_t  unk16  = 0;
        uint8_t  unk17  = 0;
        uint8_t  unk18  = 0;
        uint16_t moveID = 0;
        uint16_t unk19  = 0;

        static const unsigned int DataLen = 26; //bytes
    };

//
//
//
    class MoveDB
    {
        friend class MovesXMLWriter;
        friend class MovesXMLParser;
    public:
        MoveDB(){ m_movesData.reserve(NbMoves_EoS); }

        inline std::size_t size()const { return m_movesData.size(); }
        inline bool        empty()const{ return m_movesData.empty(); }

        //The items are guaranteed to stay allocated as long as the object exists!
        //inline const MoveData & Move( uint16_t itemindex )const { return m_movesData[itemindex]; }
        //inline       MoveData & Move( uint16_t itemindex )      { return m_movesData[itemindex]; }

        inline const MoveData & operator[]( uint16_t index )const { return m_movesData[index]; }
        inline       MoveData & operator[]( uint16_t index )      { return m_movesData[index]; }

        void push_back( MoveData && move )
        {
            m_movesData.push_back( move );
        }

    private:
        std::vector<MoveData> m_movesData;
    };

};};
#endif