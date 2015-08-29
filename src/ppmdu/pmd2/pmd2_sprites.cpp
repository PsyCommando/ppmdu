#include "pmd2_sprites.hpp"
#include <utils/utility.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/pmd2/sprite_rle.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>

using namespace ::std;
using namespace ::pmd2;
using namespace ::utils;
using namespace ::filetypes;
//using filetypes::e_ContentType;
//using filetypes::cntRID_t;
//using filetypes::ContentBlock;
//using filetypes::RuleRegistrator;
//using filetypes::CContentHandler;
//using filetypes::analysis_parameter;


namespace pmd2
{ 
    namespace graphics
    {
    //========================================================================================================
    // Utility Functions
    //========================================================================================================
        /*
            spr_varstats
                Struct that contains statistics about variables..

                **TODO**: Make something generic, not only for DataBlockI entries!!
        */
        class spr_varstats
        {
            /*
                datablock_i_sum
                    Utility type for storing the sum of all datablock entries
            */
            struct datablock_i_sum
            {
                int32_t sum_unk0;
                int32_t sum_index;
                int32_t sum_val0;
                int32_t sum_val1;
                int32_t sum_val2;
                int32_t sum_val3;

                datablock_i_sum()
                {
                    sum_unk0  = 0;
                    sum_index = 0;
                    sum_val0  = 0;
                    sum_val1  = 0;
                    sum_val2  = 0;
                    sum_val3  = 0;
                }

                datablock_i_sum & operator+=( const datablock_i_entry & other )
                {
                    this->sum_unk0  += static_cast<int32_t>(other.Unk0);
                    this->sum_index += static_cast<int32_t>(other.Index);
                    this->sum_val0  += static_cast<int32_t>(other.Val0);
                    this->sum_val1  += static_cast<int32_t>(other.Val1);
                    this->sum_val2  += static_cast<int32_t>(other.Val2);
                    this->sum_val3  += static_cast<int32_t>(other.Val3);

                    return *this;
                }

                datablock_i_entry operator/( int32_t other )const
                {
                    datablock_i_entry result;

                    result.Unk0  = this->sum_unk0   / other;
                    result.Index = this->sum_index  / other;
                    result.Val0  = this->sum_val0   / other;
                    result.Val1  = this->sum_val1   / other;
                    result.Val2  = this->sum_val2   / other;
                    result.Val3  = this->sum_val3   / other;

                    return std::move(result);
                }
            };

        public:

            spr_varstats()
                :m_nbentriesanalysed(0)
            {

            }

            void operator()( vector<datablock_i_entry>::const_iterator itbeg, vector<datablock_i_entry>::const_iterator itend )
            {
                for( auto it = itbeg; it != itend; ++it )
                {
                    (*this)(*it);
                }
            }

            void operator()( const datablock_i_entry & anentry )
            {
                //Do the sum, min, and max
                m_sum += anentry;
                m_max.AssignMembersIfGreaterThan( anentry );
                m_min.AssignMembersIfSmallerThan( anentry );

                //note our occurence
                handleOccurence( anentry );

                ++m_nbentriesanalysed;
            }
            
            /*
                WriteStatistics
                    Write a report on values found inside this.
            */
            string WriteStatistics()
            {
                stringstream strs;

                sortOccurencesByNbOfOccurences(m_occurences_unk0);
                sortOccurencesByNbOfOccurences(m_occurences_index);
                sortOccurencesByNbOfOccurences(m_occurences_val0);
                sortOccurencesByNbOfOccurences(m_occurences_val1);
                sortOccurencesByNbOfOccurences(m_occurences_val2);
                sortOccurencesByNbOfOccurences(m_occurences_val3);

                strs <<"Datablock I Data Range:\n"
                     <<"-----------------------------\n"
                     <<"    Max:\n"
                     <<m_max.toString(8)
                     <<"    Min:\n"
                     <<m_min.toString(8)
                     <<"    Average:\n"
                     <<calcAverage().toString(8)
                     <<"\n";

                strs <<"Datablock I Data Occurences:\n"
                     <<"-----------------------------\n"
                     <<"\n-> Unk0:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_unk0)
                     <<"\n-> Index:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_index)
                     <<"\n-> Val0:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val0)
                     <<"\n-> Val1:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val1)
                     <<"\n-> Val2:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val2)
                     <<"\n-> Val3:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val3)
                     <<"\n\n" ;

                return strs.str();
            }

            /*
                calcAverage
                    Return the average value for each values of a datablock_i_entry
            */
            datablock_i_entry calcAverage()const
            {
                //cerr << "Calc Average Sum unk0 : " <<m_sum.sum_unk0 <<" NB entries : " <<m_nbentriesanalysed <<"\n";
                if( m_nbentriesanalysed > 0 )
                {
                    return m_sum / m_nbentriesanalysed;
                }
                else
                {
                    assert(false); //HEY, don't do that please..
                    return datablock_i_entry();
                }
            }

            datablock_i_entry getMins()const
            {
                return m_min;
            }

            datablock_i_entry getMaxes()const
            {
                return m_max;
            }


        private:

            /*
                occurencesToString
                    Writes a list of occurences to a string, from most common, to least common.
            */
            template< class T >
                string occurencesToString( const vector< pair< int, T > > & occurvec )const
            {
                stringstream strs;

                for( const pair< int, T > & anoccurence : occurvec )
                    strs <<setfill(' ') <<setw(4) <<anoccurence.second <<" has occured " <<setfill(' ') <<setw(4) <<anoccurence.first <<" time(s)!\n";

                return strs.str();
            }

            /*
                sortOccurencesByValue
            */
            template< class T >
                void sortOccurencesByValue( vector< pair< int, T > > & occurvec )
            {
                typedef pair< int, T > occur_t;
                static auto lambdaisgreaterthan = []( const occur_t & elem1, const occur_t & elem2 )->bool
                {
                    return elem1.second > elem2.second;
                };

                std::sort( occurvec.begin(), occurvec.end(), lambdaisgreaterthan );
            }


            /*
                sortOccurencesByNbOfOccurences
            */
            template< class T >
                void sortOccurencesByNbOfOccurences( vector< pair< int, T > > & occurvec )
            {
                typedef pair< int, T > occur_t;
                static auto lambdaisgreaterthan = []( const occur_t & elem1, const occur_t & elem2 )->bool
                {
                    return elem1.first > elem2.first;
                };

                std::sort( occurvec.begin(), occurvec.end(), lambdaisgreaterthan );
            }

            /*
                findoccurence
            */
            template<class T>
               typename vector<pair<int, typename T> >::iterator findoccurence( vector< pair<int, T> > & vecoccur, T lookingfor )
            {
                return std::find_if( vecoccur.begin(), vecoccur.end(), [&lookingfor]( pair<int, T> & thatoccur ){ return lookingfor == thatoccur.second; } );
            }

            /*
                incrementOccurenceCounterIfExistAddIfNot
            */
            template<class T>
                void incrementOccurenceCounterIfExistAddIfNot( vector< pair<int, T> > & vecoccur, T newoccur )
            {
                typedef pair< int, T > occur_t;
                auto found  = findoccurence( vecoccur, newoccur );

                //If we found it, increment its occurence counter
                if( found != vecoccur.end() )
                    ++(found->first);
                else
                {
                    //Insert the entry so that its inserted before the first larger value found
                    auto foundinspos = std::find_if( vecoccur.begin(), 
                                                     vecoccur.end(), 
                                                     [&newoccur](occur_t& aoccur)
                                                     {
                                                         return aoccur.second > newoccur;
                                                     } );

                    //Handle 2 special cases to avoid trying to move the iterator out of range
                    if( foundinspos != vecoccur.end() && foundinspos != vecoccur.begin() )
                    {
                        unsigned int insertposfrombeg = (distance(vecoccur.begin(), foundinspos) - 1);
                        foundinspos = vecoccur.begin();
                        advance( foundinspos, insertposfrombeg );
                    }

                    vecoccur.insert( foundinspos, make_pair<int, T>( 1, std::move(newoccur)) );
                }
            }

            /*
                handleOccurence
                    Handles each occurence of a value for the variables we got. If its a new value
                    increment our occurence counter for that value. If not add it to the occurence list !
            */
            void handleOccurence( const datablock_i_entry & anoccurence )
            {
                //find id already in there
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_unk0, anoccurence.Unk0 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_index, anoccurence.Index );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val0, anoccurence.Val0 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val1, anoccurence.Val1 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val2, anoccurence.Val2 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val3, anoccurence.Val3 );
            }

            //average, maximum, and minimum for all values in the datablock
            datablock_i_entry m_max,
                              m_min;
            datablock_i_sum   m_sum;
            uint32_t          m_nbentriesanalysed; //Contain the current nubmer of entries we've ran the "operator()" on this far

            //List of occurences all values
            vector< pair< int, decltype(datablock_i_entry::Unk0) > >  m_occurences_unk0;
            vector< pair< int, decltype(datablock_i_entry::Index)> >  m_occurences_index;
            vector< pair< int, decltype(datablock_i_entry::Val0) > >  m_occurences_val0;
            vector< pair< int, decltype(datablock_i_entry::Val1) > >  m_occurences_val1;
            vector< pair< int, decltype(datablock_i_entry::Val2) > >  m_occurences_val2;
            vector< pair< int, decltype(datablock_i_entry::Val3) > >  m_occurences_val3;
        };



        /*
            spr_dbs_varstats
                Struct that contains statistics about variables..

                **TODO**: Make something generic, not only for DataBlockI entries!!
        */
        class spr_dbs_varstats
        {
            /*
                datablock_s_sum
                    Utility type for storing the sum of all datablock entries
            */
            struct datablock_s_sum
            {
                int32_t sum_id;
                int32_t sum_val0;
                int32_t sum_val1;
                int32_t sum_val2;
                int32_t sum_val3;

                datablock_s_sum()
                {
                    sum_id    = 0;
                    sum_val0  = 0;
                    sum_val1  = 0;
                    sum_val2  = 0;
                    sum_val3  = 0;
                }

                datablock_s_sum & operator+=( const datablock_s_entry & other )
                {
                    this->sum_id  += static_cast<int32_t>(other.id);
                    this->sum_val0  += static_cast<int32_t>(other.val0);
                    this->sum_val1  += static_cast<int32_t>(other.val1);
                    this->sum_val2  += static_cast<int32_t>(other.val2);
                    this->sum_val3  += static_cast<int32_t>(other.val3);

                    return *this;
                }

                datablock_s_entry operator/( int32_t other )const
                {
                    datablock_s_entry result;

                    result.id = this->sum_id   / other;
                    result.val0  = this->sum_val0   / other;
                    result.val1  = this->sum_val1   / other;
                    result.val2  = this->sum_val2   / other;
                    result.val3  = this->sum_val3   / other;

                    return std::move(result);
                }
            };

        public:

            spr_dbs_varstats()
                :m_nbentriesanalysed(0)
            {

            }

            void operator()( vector<datablock_s_entry>::const_iterator itbeg, vector<datablock_s_entry>::const_iterator itend )
            {
                for( auto it = itbeg; it != itend; ++it )
                {
                    (*this)(*it);
                }
            }

            void operator()( const datablock_s_entry & anentry )
            {
                //Do the sum, min, and max
                m_sum += anentry;
                m_max.AssignMembersIfGreaterThan( anentry );
                m_min.AssignMembersIfSmallerThan( anentry );

                //note our occurence
                handleOccurence( anentry );

                ++m_nbentriesanalysed;
            }
            
            /*
                WriteStatistics
                    Write a report on values found inside this.
            */
            string WriteStatistics()
            {
                stringstream strs;

                sortOccurencesByNbOfOccurences(m_occurences_id);
                sortOccurencesByNbOfOccurences(m_occurences_val0);
                sortOccurencesByNbOfOccurences(m_occurences_val1);
                sortOccurencesByNbOfOccurences(m_occurences_val2);
                sortOccurencesByNbOfOccurences(m_occurences_val3);

                strs <<"Datablock S Data Range:\n"
                     <<"-----------------------------\n"
                     <<"    Max:\n"
                     <<m_max.toString(8)
                     <<"    Min:\n"
                     <<m_min.toString(8)
                     <<"    Average:\n"
                     <<calcAverage().toString(8)
                     <<"\n";

                strs <<"Datablock S Data Occurences:\n"
                     <<"-----------------------------\n"
                     <<"\n-> ID:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_id)
                     <<"\n-> Val0:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val0)
                     <<"\n-> Val1:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val1)
                     <<"\n-> Val2:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val2)
                     <<"\n-> Val3:\n"
                     <<"-----------\n"
                     <<occurencesToString(m_occurences_val3)
                     <<"\n\n" ;

                return strs.str();
            }

            /*
                calcAverage
                    Return the average value for each values of a datablock_i_entry
            */
            datablock_s_entry calcAverage()const
            {
                //cerr << "Calc Average Sum unk0 : " <<m_sum.sum_unk0 <<" NB entries : " <<m_nbentriesanalysed <<"\n";
                if( m_nbentriesanalysed > 0 )
                {
                    return m_sum / m_nbentriesanalysed;
                }
                else
                {
                    assert(false); //HEY, don't do that please..
                    return datablock_s_entry();
                }
            }

            datablock_s_entry getMins()const
            {
                return m_min;
            }

            datablock_s_entry getMaxes()const
            {
                return m_max;
            }


        private:

            /*
                occurencesToString
                    Writes a list of occurences to a string, from most common, to least common.
            */
            template< class T >
                string occurencesToString( const vector< pair< int, T > > & occurvec )const
            {
                stringstream strs;

                for( const pair< int, T > & anoccurence : occurvec )
                    strs <<"0x" <<setfill('0') <<setw(4)  <<hex  <<uppercase <<static_cast<uint32_t>(anoccurence.second) <<nouppercase <<dec <<" has occured " <<setfill(' ') <<setw(4) <<anoccurence.first <<" time(s)!\n";

                return strs.str();
            }

            /*
                sortOccurencesByValue
            */
            template< class T >
                void sortOccurencesByValue( vector< pair< int, T > > & occurvec )
            {
                typedef pair< int, T > occur_t;
                static auto lambdaisgreaterthan = []( const occur_t & elem1, const occur_t & elem2 )->bool
                {
                    return elem1.second > elem2.second;
                };

                std::sort( occurvec.begin(), occurvec.end(), lambdaisgreaterthan );
            }


            /*
                sortOccurencesByNbOfOccurences
            */
            template< class T >
                void sortOccurencesByNbOfOccurences( vector< pair< int, T > > & occurvec )
            {
                typedef pair< int, T > occur_t;
                static auto lambdaisgreaterthan = []( const occur_t & elem1, const occur_t & elem2 )->bool
                {
                    return elem1.first > elem2.first;
                };

                std::sort( occurvec.begin(), occurvec.end(), lambdaisgreaterthan );
            }

            /*
                findoccurence
            */
            template<class T>
               typename vector<pair<int, typename T> >::iterator findoccurence( vector< pair<int, T> > & vecoccur, T lookingfor )
            {
                return std::find_if( vecoccur.begin(), vecoccur.end(), [&lookingfor]( pair<int, T> & thatoccur ){ return lookingfor == thatoccur.second; } );
            }

            /*
                incrementOccurenceCounterIfExistAddIfNot
            */
            template<class T>
                void incrementOccurenceCounterIfExistAddIfNot( vector< pair<int, T> > & vecoccur, T newoccur )
            {
                typedef pair< int, T > occur_t;
                auto found  = findoccurence( vecoccur, newoccur );

                //If we found it, increment its occurence counter
                if( found != vecoccur.end() )
                    ++(found->first);
                else
                {
                    //Insert the entry so that its inserted before the first larger value found
                    auto foundinspos = std::find_if( vecoccur.begin(), 
                                                     vecoccur.end(), 
                                                     [&newoccur](occur_t& aoccur)
                                                     {
                                                         return aoccur.second > newoccur;
                                                     } );

                    //Handle 2 special cases to avoid trying to move the iterator out of range
                    if( foundinspos != vecoccur.end() && foundinspos != vecoccur.begin() )
                    {
                        unsigned int insertposfrombeg = (distance(vecoccur.begin(), foundinspos) - 1);
                        foundinspos = vecoccur.begin();
                        advance( foundinspos, insertposfrombeg );
                    }

                    vecoccur.insert( foundinspos, make_pair<int, T>( 1, std::move(newoccur)) );
                }
            }

            /*
                handleOccurence
                    Handles each occurence of a value for the variables we got. If its a new value
                    increment our occurence counter for that value. If not add it to the occurence list !
            */
            void handleOccurence( const datablock_s_entry & anoccurence )
            {
                //find id already in there
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_id, anoccurence.id );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val0, anoccurence.val0 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val1, anoccurence.val1 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val2, anoccurence.val2 );
                incrementOccurenceCounterIfExistAddIfNot( m_occurences_val3, anoccurence.val3 );
            }

            //average, maximum, and minimum for all values in the datablock
            datablock_s_entry m_max,
                              m_min;
            datablock_s_sum   m_sum;
            uint32_t          m_nbentriesanalysed; //Contain the current nubmer of entries we've ran the "operator()" on this far

            //List of occurences all values
            vector< pair< int, decltype(datablock_s_entry::id) > >  m_occurences_id;
            vector< pair< int, decltype(datablock_s_entry::val0) > >  m_occurences_val0;
            vector< pair< int, decltype(datablock_s_entry::val1) > >  m_occurences_val1;
            vector< pair< int, decltype(datablock_s_entry::val2) > >  m_occurences_val2;
            vector< pair< int, decltype(datablock_s_entry::val3) > >  m_occurences_val3;
        };

    //========================================================================================================
    // sprite_info_data
    //========================================================================================================

        //uint8_t & sprite_info_data::operator[](unsigned int index)
        //{
        //    if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&ptr_ptrstable_e)[index];
        //    else if( index < 8 )
        //        return reinterpret_cast<uint8_t*>(&ptr_offset_f)[index-4];
        //    else if( index < 12 )
        //        return reinterpret_cast<uint8_t*>(&ptr_offset_g)[index-8];
        //    else if( index < 14 )
        //        return reinterpret_cast<uint8_t*>(&nb_blocks_in_offset_g)[index-12];
        //    else if( index < 16 )
        //        return reinterpret_cast<uint8_t*>(&nb_entries_offset_e)[index-14];
        //    else if( index < 18 )
        //        return reinterpret_cast<uint8_t*>(&unknown1)[index-16];
        //    else if( index < 20 )
        //        return reinterpret_cast<uint8_t*>(&unknown2)[index-18];
        //    else if( index < 22 )
        //        return reinterpret_cast<uint8_t*>(&unknown3)[index-20];
        //    else if( index < 24 )
        //        return reinterpret_cast<uint8_t*>(&unknown4)[index-22];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & sprite_info_data::operator[](unsigned int index)const
        //{
        //    return (*const_cast<sprite_info_data*>(this))[index];
        //}

        std::vector<uint8_t>::iterator sprite_info_data::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptr_ptrstable_e,       itwriteto );
            itwriteto = utils::WriteIntToByteVector( ptr_offset_f,          itwriteto );
            itwriteto = utils::WriteIntToByteVector( ptr_offset_g,          itwriteto );
            itwriteto = utils::WriteIntToByteVector( nb_blocks_in_offset_g, itwriteto );
            itwriteto = utils::WriteIntToByteVector( nb_entries_offset_e,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown1,              itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown2,              itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown3,              itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown4,              itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator sprite_info_data::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            ptr_ptrstable_e       = utils::ReadIntFromByteVector<decltype(ptr_ptrstable_e)>      (itReadfrom);
            ptr_offset_f          = utils::ReadIntFromByteVector<decltype(ptr_offset_f)>         (itReadfrom);
            ptr_offset_g          = utils::ReadIntFromByteVector<decltype(ptr_offset_g)>         (itReadfrom);
            nb_blocks_in_offset_g = utils::ReadIntFromByteVector<decltype(nb_blocks_in_offset_g)>(itReadfrom);
            nb_entries_offset_e   = utils::ReadIntFromByteVector<decltype(nb_entries_offset_e)>  (itReadfrom);
            unknown1              = utils::ReadIntFromByteVector<decltype(unknown1)>             (itReadfrom);
            unknown2              = utils::ReadIntFromByteVector<decltype(unknown2)>             (itReadfrom);
            unknown3              = utils::ReadIntFromByteVector<decltype(unknown3)>             (itReadfrom);
            unknown4              = utils::ReadIntFromByteVector<decltype(unknown4)>             (itReadfrom);
            return itReadfrom;
        }

        std::string sprite_info_data::toString()const
        {
            stringstream strs;
            strs <<setfill('.') <<setw(24) <<left 
                 <<"Offset table E"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_ptrstable_e <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Offset table F"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_offset_f    <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Offset table G"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_offset_g    <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"NbEntries table G" <<": "   <<right <<dec          << nb_blocks_in_offset_g           <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Unknown"           <<": "   <<right <<dec          << nb_entries_offset_e             <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Unknown #1"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown1        <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Unknown #2"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown2        <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Unknown #3"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown3        <<nouppercase <<"\n"

                 <<setfill('.') <<setw(24) <<left 
                 <<"Unknown #4"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown4        <<nouppercase <<"\n";
            return strs.str();
        }

    //========================================================================================================
    // sprite_frame_data
    //========================================================================================================
        //uint8_t & sprite_frame_data::operator[](unsigned int index)
        //{
        //    if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&ptr_frm_ptrs_table)[index];
        //    else if( index < 8 )
        //        return reinterpret_cast<uint8_t*>(&ptrPal)[index-4];
        //    else if( index < 10 )
        //        return reinterpret_cast<uint8_t*>(&unkn_1)[index-8];
        //    else if( index < 12 )
        //        return reinterpret_cast<uint8_t*>(&unkn_2)[index-10];
        //    else if( index < 14 )
        //        return reinterpret_cast<uint8_t*>(&unkn_3)[index-12];
        //    else if( index < 16 )
        //        return reinterpret_cast<uint8_t*>(&nbImgsTblPtr)[index-14];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & sprite_frame_data::operator[](unsigned int index)const
        //{
        //    return (*const_cast<sprite_frame_data*>(this))[index];
        //}

        std::vector<uint8_t>::iterator sprite_frame_data::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptr_frm_ptrs_table,     itwriteto );
            itwriteto = utils::WriteIntToByteVector( ptrPal,            itwriteto );
            itwriteto = utils::WriteIntToByteVector( unkn_1,                 itwriteto );
            itwriteto = utils::WriteIntToByteVector( unkn_2,                 itwriteto );
            itwriteto = utils::WriteIntToByteVector( unkn_3,                 itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbImgsTblPtr, itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator sprite_frame_data::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            ptr_frm_ptrs_table     = utils::ReadIntFromByteVector<decltype(ptr_frm_ptrs_table)>    (itReadfrom);
            ptrPal            = utils::ReadIntFromByteVector<decltype(ptrPal)>           (itReadfrom);
            unkn_1                 = utils::ReadIntFromByteVector<decltype(unkn_1)>                (itReadfrom);
            unkn_2                 = utils::ReadIntFromByteVector<decltype(unkn_2)>                (itReadfrom);
            unkn_3                 = utils::ReadIntFromByteVector<decltype(unkn_3)>                (itReadfrom);
            nbImgsTblPtr = utils::ReadIntFromByteVector<decltype(nbImgsTblPtr)>(itReadfrom);
            return itReadfrom;
        }

        std::string sprite_frame_data::toString()const
        {
            stringstream strs;
            strs <<setfill('.') <<setw(24) <<left <<"Offset frame ptr table" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_frm_ptrs_table <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Offset palette end"     <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptrPal        <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Unknown #1"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_1             <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Unknown #2"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_2             <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Unknown #3"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_3             <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Nb frames"              <<": "   <<right        <<dec     <<nbImgsTblPtr           <<"\n";
            return strs.str();
        }

    //========================================================================================================
    // sprite_data_header
    //========================================================================================================
        //uint8_t& sprite_data_header::operator[](unsigned int pos)
        //{
        //    if( pos < 4 )
        //        return reinterpret_cast<uint8_t*>(&spr_ptr_info)[pos];
        //    else if( pos < 8 )
        //        return reinterpret_cast<uint8_t*>(&spr_ptr_frames)[pos-4];
        //    else if( pos < 10 )
        //        return reinterpret_cast<uint8_t*>(&unknown0)[pos-8];
        //    else if( pos < DATA_LEN )
        //        return reinterpret_cast<uint8_t*>(&unknown1)[pos-10];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & sprite_data_header::operator[](unsigned int index)const
        //{
        //    return (*const_cast<sprite_data_header*>(this))[index];
        //}

        std::vector<uint8_t>::iterator sprite_data_header::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( spr_ptr_info,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( spr_ptr_frames, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown0,       itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown1,       itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator sprite_data_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            spr_ptr_info   = utils::ReadIntFromByteVector<decltype(spr_ptr_info)>  (itReadfrom);
            spr_ptr_frames = utils::ReadIntFromByteVector<decltype(spr_ptr_frames)>(itReadfrom);
            unknown0       = utils::ReadIntFromByteVector<decltype(unknown0)>      (itReadfrom);
            unknown1       = utils::ReadIntFromByteVector<decltype(unknown1)>      (itReadfrom);

            return itReadfrom;
        }

        std::string sprite_data_header::toString()const
        {
            stringstream strs;
            //<<setfill('.') <<setw(24) <<left 
            strs <<setfill('.') <<setw(24) <<left <<"Offset img data" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<spr_ptr_frames <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Offset unk data" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<spr_ptr_info   <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Unknown value0"  <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unknown0       <<nouppercase <<"\n"
                 <<setfill('.') <<setw(24) <<left <<"Unknown value1"  <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unknown1       <<nouppercase <<"\n";
            return strs.str();
        }

    //========================================================================================================
    // datablock_i_entry
    //========================================================================================================
        //uint8_t & datablock_i_entry::operator[](unsigned int index)
        //{
        //    if( index < 2 )
        //        return reinterpret_cast<uint8_t*>(&Unk0)[index];
        //    else if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&Index)[index-2];
        //    else if( index < 6 )
        //        return reinterpret_cast<uint8_t*>(&Val0)[index-4];
        //    else if( index < 8 )
        //        return reinterpret_cast<uint8_t*>(&Val1)[index-6];
        //    else if( index < 10 )
        //        return reinterpret_cast<uint8_t*>(&Val2)[index-8];
        //    else if( index < MY_SIZE )
        //        return reinterpret_cast<uint8_t*>(&Val3)[index-10];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & datablock_i_entry::operator[](unsigned int index)const
        //{
        //    return (*const_cast<datablock_i_entry*>(this))[index];
        //}

        std::vector<uint8_t>::iterator datablock_i_entry::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( Unk0,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( Index, itwriteto );
            itwriteto = utils::WriteIntToByteVector( Val0,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( Val1,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( Val2,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( Val3,  itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator datablock_i_entry::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            Unk0  = utils::ReadIntFromByteVector<decltype(Unk0)> (itReadfrom);
            Index = utils::ReadIntFromByteVector<decltype(Index)>(itReadfrom);
            Val0  = utils::ReadIntFromByteVector<decltype(Val0)> (itReadfrom);
            Val1  = utils::ReadIntFromByteVector<decltype(Val1)> (itReadfrom);
            Val2  = utils::ReadIntFromByteVector<decltype(Val2)> (itReadfrom);
            Val3  = utils::ReadIntFromByteVector<decltype(Val3)> (itReadfrom);
            return itReadfrom;
        }

        std::string datablock_i_entry::toString( unsigned int indent )const
        {
            const string myind(indent, ' ' );
            stringstream strs;
            strs <<myind <<setfill('.') <<setw(10) <<left <<"Unknown" <<": 0x" <<right <<hex <<setfill('0') <<setw(4) <<uppercase <<Unk0  <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(10) <<left <<"Index?"  <<": 0x" <<right <<hex <<setfill('0') <<setw(4) <<uppercase <<Index <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(10) <<left <<"Value 0" <<": "   <<right <<dec <<setfill(' ') <<setw(6) <<Val0 
                    <<" (0x" <<setfill('0') <<setw(4) <<hex <<static_cast<uint16_t>(Val0) <<")\n"
                 <<myind <<setfill('.') <<setw(10) <<left <<"Value 1" <<": "   <<right <<dec <<setfill(' ') <<setw(6) <<Val1
                    <<" (0x" <<setfill('0') <<setw(4) <<hex <<static_cast<uint16_t>(Val1) <<")\n"
                 <<myind <<setfill('.') <<setw(10) <<left <<"Value 2" <<": "   <<right <<dec <<setfill(' ') <<setw(6) <<Val2
                    <<" (0x" <<setfill('0') <<setw(4) <<hex <<static_cast<uint16_t>(Val2) <<")\n"
                 <<myind <<setfill('.') <<setw(10) <<left <<"Value 3" <<": "   <<right <<dec <<setfill(' ') <<setw(6) <<Val3
                    <<" (0x" <<setfill('0') <<setw(4) <<hex <<static_cast<uint16_t>(Val3) <<")\n";
            return strs.str();
        }

        datablock_i_entry & datablock_i_entry::operator+=( const datablock_i_entry & other )
        {
            this->Index += other.Index;
            this->Unk0  += other.Unk0;
            this->Val0  += other.Val0;
            this->Val1  += other.Val1;
            this->Val2  += other.Val2;
            this->Val3  += other.Val3; 
            return *this;
        }

        datablock_i_entry & datablock_i_entry::AssignMembersIfGreaterThan( const datablock_i_entry & other )
        {
            if(this->Index < other.Index)
                this->Index = other.Index;

            if( this->Unk0 < other.Unk0 )
                this->Unk0 = other.Unk0;

            if( this->Val0 < other.Val0 )
                this->Val0 = other.Val0;

            if( this->Val1 < other.Val1 )
                this->Val1 = other.Val1;

            if( this->Val2 < other.Val2 )
                this->Val2 = other.Val2;

            if( this->Val3 < other.Val3 )
                this->Val3 = other.Val3; 

            return *this;
        }

        datablock_i_entry & datablock_i_entry::AssignMembersIfSmallerThan( const datablock_i_entry & other )
        {
            if(this->Index > other.Index)
                this->Index = other.Index;

            if(this->Unk0 > other.Unk0)
                this->Unk0 = other.Unk0;

            if(this->Val0 > other.Val0)
                this->Val0 = other.Val0;

            if(this->Val1 > other.Val1)
                this->Val1 = other.Val1;

            if(this->Val2 > other.Val2)
                this->Val2 = other.Val2;

            if(this->Val3 > other.Val3)
                this->Val3 = other.Val3; 

            return *this;
        }

        datablock_i_entry datablock_i_entry::operator/( int16_t other )const
        {
            datablock_i_entry result;

            result.Unk0  = this->Unk0  / other;
            result.Index = this->Index / other;
            result.Val0  = this->Val0  / other;
            result.Val1  = this->Val1  / other;
            result.Val2  = this->Val2  / other;
            result.Val3  = this->Val3  / other;

            return result;
        }

    //========================================================================================================
    // datablock_g_entry
    //========================================================================================================

        //uint8_t &  datablock_g_entry::operator[](unsigned int index)
        //{
        //    if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&ptrtoarray)[index];
        //    else if( index < 8 )
        //        return reinterpret_cast<uint8_t*>(&szofarray)[index-4];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t &  datablock_g_entry::operator[](unsigned int index)const
        //{
        //    return (*const_cast<datablock_g_entry*>(this))[index];
        //}

        std::vector<uint8_t>::iterator datablock_g_entry::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptrtoarray,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( szofarray,   itwriteto );

            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator datablock_g_entry::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            ptrtoarray = utils::ReadIntFromByteVector<decltype(ptrtoarray)>(itReadfrom);
            szofarray  = utils::ReadIntFromByteVector<decltype(szofarray)> (itReadfrom);
            return itReadfrom;
        }

        string datablock_g_entry::toString()const
        {
            stringstream strs;
            strs << "(Ptr: 0x" <<hex <<setfill('0') <<setw(8) <<ptrtoarray <<", Size: " <<dec <<szofarray <<")\n";
            return strs.str();
        }

    //========================================================================================================
    // datablock_s_entry
    //========================================================================================================
        
        //uint8_t & datablock_s_entry::operator[](unsigned int index)
        //{
        //    if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&id)[index];
        //    else if( index < 5 )
        //        return val0;
        //    else if( index < 6 )
        //        return val1;
        //    else if( index < 7 )
        //        return val2;
        //    else if( index < 8 )
        //        return val3;
        //    else if (index < 10)
        //        return reinterpret_cast<uint8_t*>(&endofentry)[index-8];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & datablock_s_entry::operator[](unsigned int index)const
        //{
        //    return (*const_cast<datablock_s_entry*>(this))[index];
        //}

        std::vector<uint8_t>::iterator datablock_s_entry::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( id,  itwriteto );
            *itwriteto = val0;
            ++itwriteto;
            *itwriteto = val1;
            ++itwriteto;
            *itwriteto = val2;
            ++itwriteto;
            *itwriteto = val3;
            ++itwriteto;
            itwriteto = utils::WriteIntToByteVector( endofentry,  itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator datablock_s_entry::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            id = utils::ReadIntFromByteVector<decltype(id)>(itReadfrom);
            val0 = *itReadfrom;
            ++itReadfrom;
            val1 = *itReadfrom;
            ++itReadfrom;
            val2 = *itReadfrom;
            ++itReadfrom;
            val3 = *itReadfrom;
            ++itReadfrom;
            endofentry  = utils::ReadIntFromByteVector<decltype(endofentry)> (itReadfrom);
            return itReadfrom;
        }

        std::string datablock_s_entry::toString(unsigned int indent)const
        {
            const string myind(indent, ' ' );
            stringstream strs;

            strs <<myind <<setfill('.') <<setw(10) <<left 
                 <<"ID/Index"   <<": 0x" <<right <<setfill('0') <<setw(8) <<uppercase <<hex <<id <<nouppercase      <<"\n"

                 <<myind <<setfill('.') <<setw(10) <<left 
                 <<"Val 0"      <<": 0x" <<right <<setfill('0') <<setw(2) <<uppercase <<hex <<static_cast<uint16_t>(val0) <<nouppercase <<"\n"

                 <<myind <<setfill('.') <<setw(10) <<left 
                 <<"Val 1"      <<": 0x" <<right <<setfill('0') <<setw(2) <<uppercase <<hex <<static_cast<uint16_t>(val1) <<nouppercase <<"\n"

                 <<myind <<setfill('.') <<setw(10) <<left 
                 <<"Val 2"      <<": 0x" <<right <<setfill('0') <<setw(2) <<uppercase <<hex <<static_cast<uint16_t>(val2) <<nouppercase <<"\n"

                 <<myind <<setfill('.') <<setw(10) <<left 
                 <<"Val 3"      <<": 0x" <<right <<setfill('0') <<setw(2) <<uppercase <<hex <<static_cast<uint16_t>(val3) <<nouppercase <<"\n"

                 <<myind <<setfill('.') <<setw(10) <<left 
                 <<"EndOfEntry" <<": 0x" <<right <<setfill('0') <<setw(4) <<uppercase <<hex <<endofentry <<"\n";

            return strs.str();
        }

        datablock_s_entry & datablock_s_entry::operator+=( const datablock_s_entry & other )
        {
            this->id += other.id;
            this->val0  += other.val0;
            this->val1  += other.val1;
            this->val2  += other.val2;
            this->val3  += other.val3; 
            return *this;
        }

        datablock_s_entry & datablock_s_entry::AssignMembersIfGreaterThan( const datablock_s_entry & other )
        {
            if(this->id < other.id)
                this->id = other.id;

            if( this->val0 < other.val0 )
                this->val0 = other.val0;

            if( this->val1 < other.val1 )
                this->val1 = other.val1;

            if( this->val2 < other.val2 )
                this->val2 = other.val2;

            if( this->val3 < other.val3 )
                this->val3 = other.val3; 

            return *this;
        }

        datablock_s_entry & datablock_s_entry::AssignMembersIfSmallerThan( const datablock_s_entry & other )
        {
            if(this->id > other.id)
                this->id = other.id;

            if(this->val0 > other.val0)
                this->val0 = other.val0;

            if(this->val1 > other.val1)
                this->val1 = other.val1;

            if(this->val2 > other.val2)
                this->val2 = other.val2;

            if(this->val3 > other.val3)
                this->val3 = other.val3; 

            return *this;
        }

        datablock_s_entry datablock_s_entry::operator/( int16_t other )const
        {
            datablock_s_entry result;
            result.id = this->id / other;
            result.val0  = this->val0  / other;
            result.val1  = this->val1  / other;
            result.val2  = this->val2  / other;
            result.val3  = this->val3  / other;

            return result;
        }

        void datablock_s_entry::reset()
        {
            id         = 0;
            val0       = 0;
            val1       = 0;
            val2       = 0;
            val3       = 0;
            endofentry = 0;
        }

    //========================================================================================================
    // datablock_f_entry
    //========================================================================================================

        //uint8_t & datablock_f_entry::operator[](unsigned int index)
        //{
        //    if( index < 2 )
        //        return reinterpret_cast<uint8_t*>(&val0)[index];
        //    else if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&val1)[index-2];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & datablock_f_entry::operator[](unsigned int index)const
        //{
        //    return (*const_cast<datablock_f_entry*>(this))[index];
        //}

        std::vector<uint8_t>::iterator datablock_f_entry::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( val0,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( val1,  itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator datablock_f_entry::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            val0 = utils::ReadIntFromByteVector<decltype(val0)>(itReadfrom);
            val1 = utils::ReadIntFromByteVector<decltype(val1)>(itReadfrom);
            return itReadfrom;
        }

        std::string datablock_f_entry::toString( unsigned int indent )const
        {
            const string myind(indent, ' ' );
            stringstream strs;

            strs <<myind <<setfill(' ') <<setw(3) <<val0 <<", " <<setfill(' ') <<setw(3) <<val1<<"\n";

            return strs.str();
        }

        void datablock_f_entry::reset()
        {
            val0 = 0;
            val1 = 0;
        }


    //========================================================================================================
    // palette_fmtinf
    //========================================================================================================
        //uint8_t & palette_fmtinf::operator[](unsigned int index)
        //{
        //    if( index < 4 )
        //        return reinterpret_cast<uint8_t*>(&ptrpalbeg)[index];
        //    else if( index < 6 )
        //        return reinterpret_cast<uint8_t*>(&unknown0)[index-4];
        //    else if( index < 8 )
        //        return reinterpret_cast<uint8_t*>(&unknown1)[index-6];
        //    else if( index < 10 )
        //        return reinterpret_cast<uint8_t*>(&unknown2)[index-8];
        //    else if( index < 12 )
        //        return reinterpret_cast<uint8_t*>(&unknown3)[index-10];
        //    else if( index < 16 )
        //        return reinterpret_cast<uint8_t*>(&endofdata)[index-12];
        //    else
        //        return *reinterpret_cast<uint8_t*>(0); //Crash please
        //}

        //const uint8_t & palette_fmtinf::operator[](unsigned int index)const
        //{
        //    return (*const_cast<palette_fmtinf*>(this))[index];
        //}

        std::vector<uint8_t>::iterator palette_fmtinf::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptrpalbeg, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown0,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown1,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown2,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown3,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( endofdata, itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator palette_fmtinf::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            ptrpalbeg = utils::ReadIntFromByteVector<decltype(ptrpalbeg)>(itReadfrom);
            unknown0  = utils::ReadIntFromByteVector<decltype(unknown0)> (itReadfrom);
            unknown1  = utils::ReadIntFromByteVector<decltype(unknown1)> (itReadfrom);
            unknown2  = utils::ReadIntFromByteVector<decltype(unknown2)> (itReadfrom);
            unknown3  = utils::ReadIntFromByteVector<decltype(unknown3)> (itReadfrom);
            endofdata = utils::ReadIntFromByteVector<decltype(endofdata)>(itReadfrom);
            return itReadfrom;
        }

        std::string palette_fmtinf::toString( unsigned int indent )const
        {
            const string myind(indent, ' ' );
            stringstream strs;

            strs <<myind <<setfill('.') <<setw(12) <<left <<"Ptr PAL_BEG"<<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase <<ptrpalbeg <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(12) <<left <<"Unknown0"   <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase <<unknown0  <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(12) <<left <<"Unknown1"   <<": "   <<right <<dec                          <<unknown1  <<"\n"
                 <<myind <<setfill('.') <<setw(12) <<left <<"Unknown2"   <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase <<unknown2  <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(12) <<left <<"Unknown3"   <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase <<unknown3  <<nouppercase <<"\n"
                 <<myind <<setfill('.') <<setw(12) <<left <<"EndOfData"  <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase <<endofdata <<nouppercase <<"\n";

            return strs.str();
        }

        void palette_fmtinf::reset()
        {
            ptrpalbeg = 0;
            unknown0  = 0;
            unknown1  = 0;
            unknown2  = 0;
            unknown3  = 0;
            endofdata = 0;
        }

    //========================================================================================================
    // SpriteUnkRefTable
    //========================================================================================================

        SpriteUnkRefTable::SpriteUnkRefTable( unsigned int toreserveG, unsigned int toreserveH, unsigned int toreserveI )
        {
            m_datablockG.reserve(toreserveG);
            m_datablockH.reserve(toreserveH);
            m_datablockI.reserve(toreserveI);
        }


        //Push Null Entry
        // Appends a null entry properly represented in both Datablock G and H
        unsigned int SpriteUnkRefTable::pushnullrefs()
        {
            m_datablockG.push_back( datablock_g_entry() );
            m_datablockH.push_back( vector<uint32_t>() );
            return m_datablockG.size()-1;
        }

        unsigned int SpriteUnkRefTable::pushrefs( const std::vector<uint32_t> & datablockH_Array )
        {
            assert( !datablockH_Array.empty() ); // Don't push empty vectors! Use "pushnullrefs()" instead, less crap to pass around!

            if( !datablockH_Array.empty() )
            {
                //Make the entry in dbG
                m_datablockG.push_back( datablock_g_entry( m_datablockH.size(), datablockH_Array.size() ) ); // make the new entry point to (lastindexH + 1)
                m_datablockH.push_back( datablockH_Array );
            }
            else
                pushnullrefs();

            return m_datablockG.size()-1;
        }

        void SpriteUnkRefTable::moveInData( std::vector<std::vector<datablock_i_entry> > && datablocki )
        {
            m_datablockI = datablocki; //Move assignement operator
        }

        void SpriteUnkRefTable::moveInReferences( std::vector<std::vector<uint32_t> > && datablockh )
        {
            m_datablockH = datablockh; //Move assignement operator

            //Build our G datablock table !
            m_datablockG.resize( m_datablockH.size() );
            for( unsigned int i = 0; i < m_datablockG.size(); ++i )
            {
                if( !m_datablockH[i].empty() )
                {
                    m_datablockG[i].szofarray  = m_datablockH[i].size();
                    m_datablockG[i].ptrtoarray = i;
                }
                else
                {
                    m_datablockG[i].ptrtoarray = 0;
                    m_datablockG[i].szofarray  = 0;
                }
            }
        }

        const datablock_g_entry              & SpriteUnkRefTable::getDatablockGEntry( unsigned int index )const
        {
            return m_datablockG[index];
        }
        const std::vector<uint32_t>          & SpriteUnkRefTable::getDatablockHArray( unsigned int index )const
        {
            return m_datablockH[index];
        }
        const std::vector<datablock_i_entry> & SpriteUnkRefTable::getDatablockIArray( unsigned int index )const
        {
            return m_datablockI[index];
        }

        unsigned int SpriteUnkRefTable::getSizeDatablockG()const
        {
            return m_datablockG.size();
        }
        unsigned int SpriteUnkRefTable::getNbArraysDatablockH()const
        {
            return m_datablockH.size();
        }
        unsigned int SpriteUnkRefTable::getNbArraysDatablockI()const
        {
            return m_datablockI.size();
        }


        std::string SpriteUnkRefTable::WriteReport()const
        {
            stringstream strs;
            spr_varstats stats;

            strs <<"-------------------------\nDatablock G,H,I Details:\n-------------------------\n"
                 <<"Total Nb Entries Table G : " <<m_datablockG.size() <<"\n\n";

            for( decltype(m_datablockG.size()) i = 0; i < m_datablockG.size(); ++i )
            {
                auto currentgptr = m_datablockG[i].ptrtoarray;
                strs <<"G-" <<setw(4) <<setfill('0') <<i <<"-" <<m_datablockG[i].toString();

                if( m_datablockG[i].szofarray != 0 )
                {
                    for( decltype(m_datablockH[currentgptr].size()) h = 0; h < m_datablockH[currentgptr].size(); ++h )
                    {
                        auto curhptr = m_datablockH[currentgptr][h];
                        strs <<"        H-" <<h <<"-(Ptr: " <<m_datablockH[currentgptr][h] <<")\n";

                        for( decltype(m_datablockI[curhptr].size()) cpti = 0; cpti < m_datablockI[curhptr].size(); ++cpti )
                        {
                            strs <<"                I-" <<cpti <<"- Content:\n"
                                 <<m_datablockI[curhptr][cpti].toString( 20 );

                            //Compile statistics
                            stats(m_datablockI[curhptr][cpti]);
                        }
                    }
                }
                else
                {
                    strs <<"        H-0-(Ptr: NullEntry )\n"
                         <<"                -\n";
                }

            }

            strs <<"\n-------------------------\nDatablock I Analysis:\n-------------------------\n\n"
                 <<stats.WriteStatistics()
                 <<"\n";

            return strs.str();
        }

    //========================================================================================================
    // sprite_parser
    //========================================================================================================

        sprite_parser::sprite_parser( CCharSpriteData & out_asprite )
            :m_pCurSpriteOut(&out_asprite), m_isUsingIterator(false), m_pReport(nullptr), m_offsetFirstFrameBeg(0), m_offsetH(0)
        {
        }

        sprite_parser::sprite_parser( std::vector<CCharSpriteData>::iterator itspritesbeg, std::vector<CCharSpriteData>::iterator itspritesend )
            :m_pCurSpriteOut(&(*itspritesbeg)), m_itCurSprite(itspritesbeg), m_itSpritesEnd(itspritesend), m_isUsingIterator(true), m_pReport(nullptr), m_offsetFirstFrameBeg(0), m_offsetH(0)
        {
        }

        void sprite_parser::operator()( vector<uint8_t>::const_iterator itbegdata, vector<uint8_t>::const_iterator itenddata )
        {
            if( m_pCurSpriteOut == nullptr )
            {
                assert(false); //Still calling "operator()" even after the output is out of bounds !
                cerr << "!- Error : Tried to call sprite_parser::operator(), even though output is out of bound !\n";
                throw exception();
            }

            //#0 - Set our variables
            m_itbegdata = itbegdata;
            m_itcurdata = m_itbegdata;
            m_itenddata = itenddata;
            m_framepointers.resize(0);
            m_offsetH = 0;

            //#1 - Read the SIR0 header
            ReadSIR0Header();

            //#2 - Read the sprite subheader
            ReadEntireSpriteSubHeader();

            //#3 - Read the palette
            ReadPalette();

            //#4 - Read the frame pointers to get all the frames afterwards
            ReadAllFramePointers();

            //#5 - Get all the frames, decode them and put them into the output sprite data
            ReadAndDecodeAllFrames();

            //#6 - Read datablock G
            ReadDatablockG();

            //#7 - Read datablock F
            ReadDatablockF();

            //#8 - Read datablock E
            ReadDatablockE();

            //#TODO: Expend on this when needed !

            //Make a report if we need to
            if( m_pReport != nullptr )
            {
                (*m_pReport) = "SIR0 Header :\n------------------------\n" +
                               m_sir0header.toString() +
                               "\nSprite Sub-Header :\n------------------------\n" + 
                               m_subheader.toString() + 
                               "\nSprite Info Block :\n------------------------\n" +
                               m_sprinf.toString() + 
                               "\nSprite Frame Data Block :\n------------------------\n" +
                               m_sprfrmdat.toString();
            }

            //Increment if we need to
            if( m_isUsingIterator )
            {
                ++m_itCurSprite;
                if( m_itCurSprite != m_itSpritesEnd )
                {
                    m_pCurSpriteOut = &(*m_itCurSprite);
                }
                else
                {
                    m_pCurSpriteOut = nullptr;
                }
                m_pReport = nullptr;
            }
        }

        void sprite_parser::operator()( vector<uint8_t>::const_iterator itbegdata, vector<uint8_t>::const_iterator itenddata, std::string & report )
        {
            m_pReport = &report;
            (*this)( itbegdata, itenddata );
        }

        void sprite_parser::operator()( const std::pair<vector<uint8_t>::const_iterator, vector<uint8_t>::const_iterator> & apair )
        {
            (*this)( apair.first, apair.second );
        }

        //Methods
        void sprite_parser::ReadSIR0Header()
        {
            m_sir0header.ReadFromContainer( m_itcurdata );
        }

        void sprite_parser::ReadEntireSpriteSubHeader()
        {
            //#1 - Read the subheader
            m_subheader.ReadFromContainer( m_itbegdata + m_sir0header.subheaderptr );

            //#2 - Read the sprite info
            m_sprinf.ReadFromContainer( m_itbegdata + m_subheader.spr_ptr_info );

            //#3 - Read the sprite frame data
            m_sprfrmdat.ReadFromContainer( m_itbegdata + m_subheader.spr_ptr_frames );
        }

        void sprite_parser::ReadPalette()
        {
            //#1 - Calculate the size of the palette
            uint32_t           offsetPalBeg = ReadIntFromByteVector<uint32_t>( (m_itbegdata + m_sprfrmdat.ptrPal) );
            uint32_t           nbcolorspal  = ( m_sprfrmdat.ptrPal - offsetPalBeg ) / 4;
            vector<colRGB24>   mypalette( nbcolorspal );
            rgbx32_parser      theparser( mypalette.begin() );
            palette_fmtinf     palfmt;

            //#1.5 - Read the palette's format info:
            palfmt.ReadFromContainer( m_itbegdata + m_sprfrmdat.ptrPal );
            m_pCurSpriteOut->m_palfmt = palfmt;

            //2 - Read it
            std::for_each( (m_itbegdata + offsetPalBeg), 
                           (m_itbegdata + m_sprfrmdat.ptrPal), //The pointer points at the end of the palette
                           theparser );
            m_pCurSpriteOut->setPalette(mypalette);
        }

        void sprite_parser::ReadAllFramePointers()
        {
            //#1 - Calculate the amount of frames, and grow our vector
            uint32_t nbframes = m_sprfrmdat.nbImgsTblPtr;
            m_framepointers.resize( nbframes );

            //Make some handy iterators
            auto itfrm    = m_itbegdata + m_sprfrmdat.ptr_frm_ptrs_table;
            auto itfrmend = itfrm + (nbframes * 4);
            auto itout    = m_framepointers.begin();

            //Do a check to signal if the nb of ptr in the sub-header is invalid
            uint32_t actualnbframes = (m_subheader.spr_ptr_info - m_sprfrmdat.ptr_frm_ptrs_table) / 4u;
            if( actualnbframes != nbframes )
            {
                assert(false);
                cerr << "!-WARNING: Unexpected amount of frames in sprite sub-header!! Correcting, and attempting to continue!\n";
                nbframes = actualnbframes;
            }
            //Do a check to signal if a vector was allocated properly
            if( m_framepointers.size() == 0 )
            {
                assert(false);
                cerr << "!-WARNING: Memory allocation failure!!\n";
            }

            //#2 - Read each pointers and fill up the table
            while( itfrm != itfrmend )
            {
                uint32_t ptr = utils::ReadIntFromByteVector<uint32_t>(itfrm);
                //for( unsigned int shifter = 0; shifter < 4; ++shifter, ++itfrm )
                //{
                //    uint32_t temp = *itfrm; //just to be sure we don't overflow
                //    ptr |= ( temp << (8u * shifter) & 0xFFu << (8u * shifter) ); //Apply a mask to clean up any thrash from shift operators
                //}
                (*itout) =  ptr;
                ++itout;
            }
        }

        void sprite_parser::ReadAndDecodeAllFrames()
        {
            if(m_framepointers.empty())
            {
                assert(false);
                cerr << "!-WARNING: No frame data in sprite!! Skipping decoding!\n";
                return;
            }

            //#1 - Ensure the output is large enough
            m_pCurSpriteOut->getAllFrames().resize( m_framepointers.size() );

            //#2 - Iterate on all our frame pointers and run our functor
            compression::rle_decoder decoder( m_pCurSpriteOut->getAllFrames().begin(),
                                              m_pCurSpriteOut->getAllFrames().end() );

            for( const auto & pointer : m_framepointers )
            {
                //Get the first frame's frm_beg
                if( m_offsetFirstFrameBeg == 0 ) 
                    m_offsetFirstFrameBeg = decoder( m_itbegdata, m_itbegdata + pointer );
                else
                    decoder( m_itbegdata, m_itbegdata + pointer );
            }
        }

        //INCREMENTS THE ITERATOR PASSED BY REFERENCE !!!!
        vector<datablock_i_entry> ReadASingleDatablockITable( vector<uint8_t>::const_iterator& itfrom )
        {
            vector<datablock_i_entry> table(64u); //We don't know the size in advance
            datablock_i_entry         dataentry;
            table.resize(0);

            do
            {
                dataentry.Reset(); //reset values to 0
                itfrom = dataentry.ReadFromContainer( itfrom );
                table.push_back( dataentry );
            }while( !dataentry.isNullEntry() );

            return std::move(table);
        }

        //INCREMENTS THE ITERATOR PASSED BY REFERENCE !!!!
        vector<uint32_t> ReadASingleDatablockHTable( vector<uint8_t>::const_iterator& itfrom, uint32_t expectednbentries, const vector<uint32_t> & offset2indexconvtable )
        {
            vector<uint32_t> DBHTable     (expectednbentries);
            auto             itLastOff    = offset2indexconvtable.begin();
            uint32_t         indexLastOff = 0;

            for( auto & anoffset : DBHTable )
            {
                uint32_t offset  = utils::ReadIntFromByteVector<uint32_t>(itfrom); //Increments iterator
                auto     itfound = std::find( itLastOff, offset2indexconvtable.end(), offset ); //faster on shorter distances

                if( itfound != offset2indexconvtable.end() )
                {
                    indexLastOff += std::distance( itLastOff, itfound ); //faster on shorter distances
                    itLastOff    = itfound;
                    offset       = indexLastOff; //Set the offset's value to the correct index
                }
                else
                {
                    assert(false); //This means I've failed..
                    throw exception( "pmd2_sprites.cpp->ReadASingleDatablockHTable(): Couldn't convert an offset from Datablock H !" );
                }

                anoffset = offset; //Assign the result
            }

            return std::move( DBHTable );
        }

        void sprite_parser::ReadDatablockG()
        {
            assert(m_offsetFirstFrameBeg); //We need the offset of the first frame !

            auto itBegPtrTableG  = m_itbegdata     + m_sprinf.ptr_offset_g; //Iter to beginning of Datablock G
            auto itLastPtrTableG = itBegPtrTableG  + ( (m_sprinf.nb_blocks_in_offset_g - 1) * datablock_g_entry::MY_SIZE);
            auto itEndPtrTableG  = itLastPtrTableG + datablock_g_entry::MY_SIZE; //Iter to end of Datablock G

            vector<datablock_g_entry> ptrsDBG   ( m_sprinf.nb_blocks_in_offset_g );
            vector<vector<uint32_t> > DataBlockH( m_sprinf.nb_blocks_in_offset_g );
            uint32_t                  lengthDBHInBytes   = 0, //Length in byte of DBH
                                      nbTableEntriesDBH  = 0; //Nb of non-null table entries in DBH

            //<!> - We know that, Datablock H has a size that is dependent on G. So if we take into account the values in G, we
            //      can figure out where DBH begins from G, by estimating its length, and subtracting that from the offset of G!

            //***********************************************************************************************************
            //#1 - Read Datablock G, and determine what we need to know to get to H and I !

            //Read the entire DBG
            auto itReadGPtr = itBegPtrTableG;
            //for( auto & aptr : ptrsDBG )
            for( unsigned int i = 0; i < ptrsDBG.size(); ++i )
            {
                itReadGPtr = ptrsDBG[i].ReadFromContainer( itReadGPtr );

                if( ptrsDBG[i].isNullEntry() )
                {
                    lengthDBHInBytes += 4u;                   //A null entry in DBG is always a single null entry in DBH 
                }
                else
                {
                    nbTableEntriesDBH += ptrsDBG[i].szofarray;      //Add only non-null entries
                    lengthDBHInBytes  += ptrsDBG[i].szofarray * 4u; //add the length of the table in bytes
                    DataBlockH[i].resize(ptrsDBG[i].szofarray);     //Resize the DBH table properly
                }
            }

            //Set offset H
            m_offsetH = (m_sprinf.ptr_offset_g - lengthDBHInBytes);

            //***********************************************************************************************************
            //#2 - Get the beginning and end of DBH, read it. Taking note of all the UNIQUE offsets in DBI it refers to
            auto itBegDBH = m_itbegdata + m_offsetH,
                 itEndDBH = itBegPtrTableG;

            //Read DBH
            auto itReadDBH = itBegDBH;
            for( unsigned int i = 0; i < DataBlockH.size(); ++i )
            {
                for( unsigned int j = 0; j < DataBlockH[i].size(); ++j )
                    DataBlockH[i][j] = utils::ReadIntFromByteVector<uint32_t>( itReadDBH );

                if( DataBlockH[i].empty() )
                    advance( itReadDBH, 4 ); //skip a null entry in this case, since we didn't increment the iterator!
            }

            //***********************************************************************************************************
            //#3 - Read the entire DBI. And convert the file offsets in DBH to indices in the DataBlockI vector as we go!
            uint32_t endOffsetDBI = 0;
            //
            vector<vector<datablock_i_entry> > DataBlockI( nbTableEntriesDBH );

            //This has the same size as the DataBlockI table container. And indicate at what file offset 
            //  each of the tables in DataBlockI were taken from.
            vector<uint32_t> uniquefileoffsetDBITables( nbTableEntriesDBH );

            DataBlockI.resize(0);
            uniquefileoffsetDBITables.resize(0);

            //Iterate through all entries in DBH, to fill up everything in DBI. While at the same time changing the
            // file offsets in DBH to vector indices relative to the "uniquefileoffsetDBITables" and thus to the
            // DataBlockI vector!
            for( unsigned int i = 0; i < DataBlockH.size(); ++i )
            {
                for( unsigned int j = 0; j < DataBlockH[i].size(); ++j )
                {
                    uint32_t currentptr = DataBlockH[i][j];

                    //If we hit this, conflicting information was found in the Datablock H!
                    assert( DataBlockH[i].size() <= static_cast<size_t>(distance( m_itbegdata + currentptr, m_itbegdata + m_offsetFirstFrameBeg )) );

                    //See if we already did this offset
                    auto itfound = find( uniquefileoffsetDBITables.begin(), uniquefileoffsetDBITables.end(), currentptr );
                    if( itfound == uniquefileoffsetDBITables.end() )
                    {
                        //If not found, add it !
                        uniquefileoffsetDBITables.push_back( currentptr );

                        //And read the data
                        DataBlockI.push_back( ReadASingleDatablockITable( m_itbegdata + currentptr ) );

                        //Transform the offset in DBH to the offset we just inserted to in DataBlockI
                        DataBlockH[i][j] = uniquefileoffsetDBITables.size() - 1;
                    }
                    else
                    {
                        //Transform the offset in DBH to the found position 
                        DataBlockH[i][j] = std::distance( uniquefileoffsetDBITables.begin(), itfound );
                    }
                }
            }

            //***********************************************************************************************************
            //#4 - Assign the DBI data first, then the DBH references;

            //Put the data into the target container
            m_pCurSpriteOut->m_datablocksGHI.moveInData( std::move(DataBlockI) );

            // Assign
            m_pCurSpriteOut->m_datablocksGHI.moveInReferences( std::move(DataBlockH) );
        }

        void sprite_parser::ReadDatablockE()
        {
            assert(m_offsetH); //need this

            //#1 - Get the position of the ptr table
            auto itdbbeg = m_itbegdata + m_sprinf.ptr_ptrstable_e;
            unsigned int nbentries = 0;

            //Get the next block's offset to get our length
            if( m_sprinf.ptr_offset_f != 0 ) //offset F is optional, so watch out for that!
                nbentries = (m_sprinf.ptr_offset_f - m_sprinf.ptr_ptrstable_e) / 4u;
            else
                nbentries = (m_offsetH - m_sprinf.ptr_ptrstable_e) / 4u; //If there are no offset F, go for the next one, offset H!

            //Resize target
            m_pCurSpriteOut->m_datablockE.resize(nbentries);

            //#2 - Get all entries using the pointer table.
            for( auto & bbe_entry : m_pCurSpriteOut->m_datablockE )
            {
                //Read the pointer
                uint32_t entryoffset = utils::ReadIntFromByteVector<uint32_t>(itdbbeg);
                //Read the data pointed
                bbe_entry.ReadFromContainer( m_itbegdata + entryoffset );
            }
        }

        void sprite_parser::ReadDatablockF()
        {
            assert(m_offsetH != 0); //need this

            //Check if its ptr isn't null first !
            if( m_sprinf.ptr_offset_f != 0 )
            {
                auto itreadf = m_itbegdata + m_sprinf.ptr_offset_f;

                m_pCurSpriteOut->m_datablockF.resize( (m_offsetH - m_sprinf.ptr_offset_f) / datablock_f_entry::MY_SIZE );

                for( auto & anentry : m_pCurSpriteOut->m_datablockF )
                    itreadf = anentry.ReadFromContainer( itreadf );
            }
        }

    //=====================================================================================
    // Character Sprite Data 
    //=====================================================================================
                
        vector<indexed8bppimg_t>& CCharSpriteData::getAllFrames()
        {
            return m_frames8bpp;
        }

        //
        const indexed8bppimg_t& CCharSpriteData::getFrame( vector<indexed8bppimg_t>::size_type frmno )
        {
            return m_frames8bpp[frmno];
        }

        void CCharSpriteData::setFrame( vector<indexed8bppimg_t>::size_type frmno, const indexed8bppimg_t & frmdata )
        {
            m_frames8bpp[frmno].resize( frmdata.size() );
            std::copy( frmdata.begin(), frmdata.end(), m_frames8bpp[frmno].begin() );
        }

        //
        const vector<colRGB24>& CCharSpriteData::getPalette()const
        {
            return m_palette;
        }

        void CCharSpriteData::setPalette( const vector<colRGB24> & pal )
        {
            m_palette = pal;
        }


        std::string CCharSpriteData::WriteReport()
        {
            stringstream myreport;

            //#1 - Write a list of the size of all frames
            myreport <<"\n"
                     <<"-------------\nFrames Data:\n-------------\n\nFrm #           Ammount of Pixels\n------          ------------------\n";

            for( unsigned int i = 0; i < m_frames8bpp.size(); ++i )
                myreport <<" " <<setfill('0') <<setw(3) <<i <<": " <<setfill(' ') <<setw(4) <<m_frames8bpp[i].size() <<"\n";

            //#2 - Write details on the palette
            myreport <<"\n"
                     <<"-------------\nPalette Data:\n--------------\n"
                     <<m_palfmt.toString()
                     <<setfill('.') <<setw(12) <<left <<"Nb Colors"   <<": "  <<right <<m_palette.size() <<"\n"
                     <<setfill('.') <<setw(12) <<left <<"Colors(RGB)" <<":\n" <<right;

            for( unsigned int i = 0; i < m_palette.size(); ++i )
            {
                myreport <<setfill(' ') <<setw(4) <<i <<": " <<uppercase <<hex 
                         <<setfill('0') <<setw(2) <<static_cast<short>(m_palette[i].red) 
                         <<setfill('0') <<setw(2) <<static_cast<short>(m_palette[i].green) 
                         <<setfill('0') <<setw(2) <<static_cast<short>(m_palette[i].blue) 
                         <<"\n" <<dec;
            }

            //#3 - Write details on datablock G
            //#4 - Write details on datablock H
            //#5 - Write details on datablock I
            myreport <<"\n" <<m_datablocksGHI.WriteReport();

            //#6 - Write details on datablock F
            myreport <<"\n"
                     <<"-------------\nDatablock F:\n--------------\n";
            if( !m_datablockF.empty() )
            {
                decltype(datablock_f_entry::val0) min_val0 = 0,
                                                  max_val0 = 0,
                                                  min_val1 = 0,
                                                  max_val1 = 0;

                for( decltype(m_datablockF.size()) cptf = 0; cptf < m_datablockF.size(); ++cptf )
                {
                    if( m_datablockF[cptf].val0 < min_val0 )
                        min_val0 = m_datablockF[cptf].val0;
                    if( m_datablockF[cptf].val0 > max_val0 )
                        max_val0 = m_datablockF[cptf].val0;

                    if( m_datablockF[cptf].val1 < min_val1 )
                        min_val1 = m_datablockF[cptf].val1;
                    if( m_datablockF[cptf].val1 > max_val1 )
                        max_val1 = m_datablockF[cptf].val1;

                    myreport<<" " <<setfill(' ') <<setw(4) <<cptf <<" : " <<m_datablockF[cptf].toString();
                }

                myreport <<"\n-------------\nDatablock F Analysis:\n--------------\n"
                         <<setfill('.') <<setw(16) <<left  <<"Nb Entries"      <<": " <<m_datablockF.size() <<"\n"
                         <<setfill('.') <<setw(16) <<left  <<"Min/Max value 0" <<": " <<"( " <<min_val0 <<", " <<max_val0 <<" )\n"
                         <<setfill('.') <<setw(16) <<left  <<"Min/Max value 1" <<": " <<"( " <<min_val1 <<", " <<max_val1 <<" )\n";
            }
            else
            {
                myreport <<"    No Datablock F!\n";
            }

            //#7 - Write details on datablock S
            spr_dbs_varstats mydbsstats;
            myreport <<"\n"
                     <<"-------------\nDatablock E/S:\n--------------\n";
            for( decltype(m_datablockE.size()) cpte = 0; cpte < m_datablockE.size(); ++cpte )
            {
                mydbsstats(m_datablockE[cpte]);
                myreport <<"    S-" <<cpte <<"- Content:\n"
                         <<m_datablockE[cpte].toString(8);
            }
            myreport <<"\n"
                     <<"-------------\nDatablock E/S Analysis:\n--------------\n"
                     <<mydbsstats.WriteStatistics()
                     <<"\n";

            return myreport.str();
        }


    //========================================================================================================
    // Sprite File Detection Rules
    //========================================================================================================

        /*
            sir0_sprite_rule
                Rule for sprites stored within a SIR0 container.
        */
    //    class sir0_sprite_rule : public filetypes::IContentHandlingRule
    //    {
    //    public:
    //        sir0_sprite_rule(){}
    //        ~sir0_sprite_rule(){}

    //        //Returns the value from the content type enum to represent what this container contains!
    //        virtual cnt_t getContentType()const;

    //        //Returns an ID number identifying the rule. Its not the index in the storage array,
    //        // because rules can me added and removed during exec. Thus the need for unique IDs.
    //        //IDs are assigned on registration of the rule by the handler.
    //        virtual cntRID_t getRuleID()const;
    //        virtual void              setRuleID( cntRID_t id );

    //        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //        //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
    //                                      //vector<uint8_t>::const_iterator   itdataend );
    //        virtual ContentBlock Analyse( const analysis_parameter& parameters );

    //        //This method is a quick boolean test to determine quickly if this content handling
    //        // rule matches, without in-depth analysis.
    //        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
    //                               vector<uint8_t>::const_iterator   itdataend,
    //                               const std::string & filext);

    //    private:
    //        cntRID_t m_myID;
    //    };

    //    //Returns the value from the content type enum to represent what this container contains!
    //    e_ContentType sir0_sprite_rule::getContentType()const
    //    {
    //        return e_ContentType::SPRITE_CONTAINER;
    //    }

    //    //Returns an ID number identifying the rule. Its not the index in the storage array,
    //    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //    //IDs are assigned on registration of the rule by the handler.
    //    cntRID_t sir0_sprite_rule::getRuleID()const
    //    {
    //        return m_myID;
    //    }
    //    void sir0_sprite_rule::setRuleID( cntRID_t id )
    //    {
    //        m_myID = id;
    //    }

    //    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //    ContentBlock sir0_sprite_rule::Analyse( const analysis_parameter& parameters  )
    //    {
    //        ContentBlock cb;
    //        //build our content block info
    //        cb._startoffset          = 0;
    //        cb._endoffset            = distance( parameters._itparentbeg, parameters._itdataend );
    //        cb._rule_id_that_matched = getRuleID();
    //        cb._type                 = getContentType();

    //        //No known sub-content

    //        return cb;
    //    }

    //    //This method is a quick boolean test to determine quickly if this content handling
    //    // rule matches, without in-depth analysis.
    //    bool sir0_sprite_rule::isMatch( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend , const std::string & filext )
    //    {
    //        //Literally the best check we can do ^^;
    //        unsigned int lengthsofar = 0;
    //        for( auto itcount = itdatabeg; itcount != itdataend && lengthsofar <= 27u; ++lengthsofar, ++itcount )

    //        //It can't be longer than 26, if the last field ends up on the line below..
    //        // -- -- -- -- -- -- 01 01 01 01 02 02 02 02 03 03
    //        // 04 04 AA AA AA AA AA AA AA AA AA AA AA AA AA AA
    //        if( lengthsofar > 27u ) //set to 27, in the very unlikely case that it wouldn't be aligned on the field's size..
    //        {
    //            return false;
    //        }
    //        else if( lengthsofar == sprite_data_header::DATA_LEN )
    //        {
    //            return true;
    //        }
    //        else if( lengthsofar > sprite_data_header::DATA_LEN )
    //        {
    //            vector<uint8_t>::const_iterator itsearch = itdatabeg;
    //            std::advance( itsearch, sprite_data_header::DATA_LEN );
    //            return std::all_of( itsearch, itdataend, []( uint8_t val ){ return val == pmd2::filetypes::COMMON_PADDING_BYTE; } );
    //        }

    //        return false;
    //    }
    //};

    //=========================================================================================================
    //namespace filetypes
    //{
    //    //========================================================================================================
    //    //  sir0_sprite_rule
    //    //========================================================================================================
    //        /*
    //            sir0_sprite_rule
    //                A small singleton that has for only task to register the at4px_rule!
    //        */
    //        RuleRegistrator<graphics::sir0_sprite_rule> RuleRegistrator<graphics::sir0_sprite_rule>::s_instance;
    //};

};};