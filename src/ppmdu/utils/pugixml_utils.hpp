#ifndef PUGIXML_UTILS_HPP
#define PUGIXML_UTILS_HPP
/*
pugixml_utils.hpp
2015/03/26
psycommando@gmail.com
Description:
    A bunch of tools for doing repetitive things with the pugixml lib!
    Don't include in another header!!
*/
#include <string>
#include <pugixml.hpp>

namespace pugixmlutils
{
    inline void WriteCommentNode( pugi::xml_node & node, const std::string & str )
    {
        using namespace pugi;
        node.append_child( xml_node_type::node_comment ).set_value( str.c_str() );
    }

    inline void WriteCommentNode( pugi::xml_node & node, const char * str )
    {
        using namespace pugi;
        node.append_child( xml_node_type::node_comment ).set_value(str);
    }

    template<class T>
        inline void WriteNodeWithValue( pugi::xml_node & parentnode, const std::string & name, T value )
    {
        using namespace pugi;
        using namespace std;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value( to_string(value).c_str() );
    }

    template<>
        inline void WriteNodeWithValue<const char *>( pugi::xml_node & parentnode, const std::string & name, const char * value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value);
    }

    template<>
        inline void WriteNodeWithValue<const std::string &>( pugi::xml_node & parentnode, const std::string & name, const std::string & value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    template<>
        inline void WriteNodeWithValue<std::string&&>( pugi::xml_node & parentnode, const std::string & name, std::string &&value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    template<>
        inline void WriteNodeWithValue<std::string>( pugi::xml_node & parentnode, const std::string & name, std::string value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    inline pugi::xml_node AppendChildNode( pugi::xml_node & parent, const std::string & childname )
    {
        return parent.append_child( childname.c_str() );
    }
};

#endif