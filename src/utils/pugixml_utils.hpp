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
#include <codecvt>
#include <locale>
#include <sstream>

namespace pugixmlutils
{

    /***************************************************************************************
        WriteCommentNode
            Helper for writing comments from std::string. Also helps with clarity.
    /***************************************************************************************/
    inline void WriteCommentNode( pugi::xml_node & node, const pugi::string_t & str )
    {
        using namespace pugi;
        node.append_child( xml_node_type::node_comment ).set_value( str.c_str() );
    }

    inline void WriteCommentNode( pugi::xml_node & node, const pugi::char_t * str )
    {
        using namespace pugi;
        node.append_child( xml_node_type::node_comment ).set_value(str);
    }

    /***************************************************************************************
        WriteNodeWithValue
            Helper for converting values to text, and putting it in a new node. 
            Also helps with clarity.
    /***************************************************************************************/
    template<class T>
        inline void WriteNodeWithValue( pugi::xml_node & parentnode, const pugi::string_t & name, T value )
    {
        using namespace pugi;
        using namespace std;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value( to_string(value).c_str() );
    }

    template<>
        inline void WriteNodeWithValue<const pugi::char_t *>( pugi::xml_node & parentnode, const pugi::string_t & name, const pugi::char_t * value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value);
    }

    template<>
        inline void WriteNodeWithValue<const pugi::string_t &>( pugi::xml_node & parentnode, const pugi::string_t & name, const pugi::string_t & value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    template<>
        inline void WriteNodeWithValue<pugi::string_t&&>( pugi::xml_node & parentnode, const pugi::string_t & name, pugi::string_t &&value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    template<>
        inline void WriteNodeWithValue<pugi::string_t>( pugi::xml_node & parentnode, const pugi::string_t & name, pugi::string_t value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value(value.c_str());
    }

    template<>
        inline void WriteNodeWithValue<int8_t>( pugi::xml_node & parentnode, const pugi::string_t & name, int8_t value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value( std::to_string( static_cast<int16_t>(value) ).c_str() );
    }

    template<>
        inline void WriteNodeWithValue<uint8_t>( pugi::xml_node & parentnode, const pugi::string_t & name, uint8_t value )
    {
        using namespace pugi;
        parentnode.append_child(name.c_str()).append_child(node_pcdata).set_value( std::to_string( static_cast<uint16_t>(value) ).c_str() );
    }

    /***************************************************************************************
        AppendChildNode
            Makes appending a child node and setting its name easier, clearer with std::string.
    /***************************************************************************************/
    inline pugi::xml_node AppendChildNode( pugi::xml_node & parent, const pugi::string_t & childname )
    {
        return parent.append_child( childname.c_str() );
    }

    /***************************************************************************************
        AppendAttribute
            Makes appending an attribute and setting its value easier, clearer with std::string.
    ***************************************************************************************/
    template<class T>
        inline pugi::xml_attribute AppendAttribute( pugi::xml_node & parent, const pugi::string_t & name, T value ) 
    {
        using namespace pugi;
        xml_attribute att = parent.append_attribute(name.c_str());
#ifdef PUGIXML_WCHAR_MODE
        if( !att.set_value(std::to_wstring(value).c_str()) )
            throw std::runtime_error("pugixml couldn't set the value of attribute " + as_utf8(name) );
#else
        if( !att.set_value(std::to_string(value).c_str()) )
            throw std::runtime_error("pugixml couldn't set the value of attribute " + name);
#endif
        return std::move(att);
    }

    template<>
        inline pugi::xml_attribute AppendAttribute<const pugi::char_t*>( pugi::xml_node & parent, const pugi::string_t & name, const pugi::char_t* value )  
    {
        using namespace pugi;
        xml_attribute att = parent.append_attribute(name.c_str());

#ifdef PUGIXML_WCHAR_MODE
        if( !att.set_value(value) )
            throw std::runtime_error("pugixml couldn't set the value of attribute " + as_utf8(name) );
#else
        if( !att.set_value(value) )
            throw std::runtime_error("pugixml couldn't set the value of attribute " + name );
#endif
        return std::move(att);
    }

    template<>
        inline pugi::xml_attribute AppendAttribute<pugi::string_t>( pugi::xml_node & parent, const pugi::string_t & name, pugi::string_t value )  
    {
        using namespace pugi;
        xml_attribute att = parent.append_attribute(name.c_str());

#ifdef PUGIXML_WCHAR_MODE
        if( !att.set_value(value.c_str()) )
        {
            //std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
            throw std::runtime_error("pugixml couldn't set the value of attribute " + /*myconv.to_bytes(*/as_utf8(name)/*)*/ );
        }
#else
        if( !att.set_value(value.c_str()) )
            throw std::runtime_error("pugixml couldn't set the value of attribute " + name );
#endif
        return std::move(att);
    }


    /*
        HandleParsingError
            If there were no errors while parsing does nothing. Otherwise throws an appropriate exception!
    */
    void HandleParsingError( const pugi::xml_parse_result & result, const std::string & xmlpath )
    ;
    //{
    //    using namespace std;
    //    using namespace pugi;
    //    if( !result )
    //    {
    //        stringstream sstr;
    //        sstr << "Error, pugixml couldn't parse the XML file \"" <<xmlpath <<"\" !";

    //        switch( result.status )
    //        {
    //            case xml_parse_status::status_file_not_found:
    //            {
    //                sstr<< "The file was not found!";
    //                break;
    //            }
    //            case xml_parse_status::status_io_error:
    //            {
    //                sstr<< "An error occured while opening the file for reading!";
    //                break;
    //            }
    //            case xml_parse_status::status_no_document_element:
    //            {
    //                sstr<< "The XML file lacks a document element!";
    //                break;
    //            }
    //            case xml_parse_status::status_unrecognized_tag:
    //            {
    //                sstr<< "The tag at offset " <<result.offset <<" was unrecognized !";
    //                break;
    //            }
    //            case xml_parse_status::status_end_element_mismatch:
    //            {
    //                sstr<< "Encountered unexpected end tag at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_start_element:
    //            {
    //                sstr<< "Encountered bad start tag at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_end_element:
    //            {
    //                sstr<< "Encountered bad end tag at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_attribute:
    //            {
    //                sstr<< "Encountered bad attribute at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_cdata:
    //            {
    //                sstr<< "Encountered bad cdata at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_comment:
    //            {
    //                sstr<< "Encountered bad comment at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_doctype:
    //            {
    //                sstr<< "Encountered bad document type at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_pcdata:
    //            {
    //                sstr<< "Encountered bad pcdata at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_bad_pi:
    //            {
    //                sstr<< "Encountered bad document declaration/parsing instructions at offset " <<result.offset <<" !";
    //                break;
    //            }
    //            case xml_parse_status::status_append_invalid_root:
    //            {
    //                sstr<< "Invalid type of root node at offset " <<result.offset <<" !";
    //                break;
    //            }
    //        };

    //        throw runtime_error( sstr.str() );
    //    }
    //}
};

#endif