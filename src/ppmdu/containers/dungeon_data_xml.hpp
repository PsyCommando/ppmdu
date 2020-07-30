#ifndef DUNGEON_DATA_XML_HPP
#define DUNGEON_DATA_XML_HPP
/*
dungeon_data_xml.hpp

*/
#include <ppmdu/containers/dungeons_data.hpp>

namespace pmd2 { namespace stats {

    /*
        Writes a DungeonDB to a set of XML files.
    */
    void WriteDungeonDataToXml(const DungeonDB & db, const std::string & destpath, const pmd2::GameText * text, const ConfigLoader & conf);

    /*
        Read a set of xml files into a DungeonDB object
    */
    DungeonDB & ReadDungeonDataFromXml(const std::string & srcpath, DungeonDB & outdb, pmd2::GameText * text);
}}
#endif