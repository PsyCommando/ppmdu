#ifndef INDEX_ITERATOR_HPP
#define INDEX_ITERATOR_HPP
/*
index_iterator.hpp
2014/11/29
psycommando@gmail.com
Description:
        A simple Standard Library compliant iterator over containers implementing the bracket operator, and size() method.
        The size() method of the owner container should provide constant complexity for maximum performance.
*/
#include <stdexcept>
#include <iterator>

namespace utils
{

    /*
        index_iterator
    */
    template<class _ContainerType, class _parentitert = std::iterator<std::random_access_iterator_tag, typename _ContainerType::value_type> >
        class index_iterator : public _parentitert
    {
    public:
        typedef _parentitert            myiter_t;
        typedef _ContainerType          container_t;
        typedef typename  container_t * container_ptr_t;

        explicit index_iterator( container_ptr_t pcontainer, std::size_t index = 0 )throw()
            :m_index(index), m_pContainer(pcontainer)
        {}

        index_iterator( const index_iterator<container_t,myiter_t> & other )throw()
            :m_index(other.m_index), m_pContainer(other.m_pContainer)
        {}

        virtual ~index_iterator()throw()
        {}

        index_iterator<container_t,myiter_t> & operator=( const index_iterator<container_t,myiter_t> & other )
        {
            m_index      = other.m_index;
            m_pContainer = other.m_pContainer;
            return *this;
        }

        bool operator==(const index_iterator & iter)const throw() { return m_index == iter.m_index; }
        bool operator!=(const index_iterator & iter)const throw() { return !(*this == iter); }

        index_iterator & operator++()  
        { 
            //This is to avoid moving past the end
            if( m_pContainer->size() > m_index )
                ++m_index; 
            else
                throw std::out_of_range("Iterator out of bound!");
            return *this;
        }

        index_iterator operator++(int)
        {
            iterator temp(*this);
            operator++();
            return temp;
        }

        index_iterator & operator--()  
        { 
            //This is to avoid moving past the end
            if( m_index > 0 )
                --m_index; 
            else
                throw std::out_of_range("Iterator out of bound!");
            return *this;
        }
        index_iterator operator--(int)
        {
            iterator temp(*this);
            operator--();
            return temp;
        }


        index_iterator & operator+=( typename myiter_t::difference_type n )  
        { 
            if ( n >= 0 )
            {
                if( m_pContainer->size() >= (m_index + n) )
                    m_index += n;
                else
                    throw std::out_of_range("Iterator out of bound!");
            }
            else
            {
                if( (m_index - n) >= 0 )
                    m_index -= n;
                else
                    throw std::out_of_range("Iterator out of bound!");
            }
            return *this;
        }

        index_iterator operator-=( typename myiter_t::difference_type n )
        {
            return (*this += -n);
        }

        index_iterator operator+( typename myiter_t::difference_type n )const
        {
            index_iterator temp = *this;
            return temp += n;
        }

        index_iterator operator-( typename myiter_t::difference_type n )const
        {
            index_iterator temp = *this;
            return temp -= n;
        }

        typename myiter_t::difference_type operator-( const index_iterator & otherit )const
        {
            return this->m_index - otherit.m_index;
        }

        typename myiter_t::reference operator[]( std::size_t n )const
        {
            return *((*this)+n);
        }

        bool operator<( const index_iterator & otherit )const
        {
            return m_index < otherit.m_index;
        }

        bool operator>( const index_iterator & otherit )const
        {
            return otherit < *this;
        }

        bool operator>=( const index_iterator & otherit )const
        {
            return !(*this < otherit);
        }

        bool operator<=( const index_iterator & otherit )const
        {
            return !(*this > otherit);
        }

        typename myiter_t::reference       operator*()        { return (*m_pContainer)[m_index]; }
        const typename myiter_t::reference operator*() const  { return (*m_pContainer)[m_index]; }
        typename myiter_t::pointer         operator->()       { return &((*m_pContainer)[m_index]); }
        const typename myiter_t::pointer   operator->() const { return &((*m_pContainer)[m_index]); }

    protected:
        std::size_t     m_index;
        container_ptr_t m_pContainer;
    };

    /*
        const_index_iterator
            The same thing as above, but constant
    */
    template<class _CONTAINER_T>
    class const_index_iterator : public index_iterator<_CONTAINER_T, 
                                                       std::iterator<
                                                                     std::random_access_iterator_tag, 
                                                                     typename _CONTAINER_T::value_type, 
                                                                     std::ptrdiff_t, 
                                                                     const typename _CONTAINER_T::value_type *, 
                                                                     const typename _CONTAINER_T::value_type > 
                                                                    >
    {
    public:
        const_index_iterator( container_ptr_t pcontainer, std::size_t index = 0 )throw()
            :index_iterator(pcontainer,index)/*,m_index(index), m_pContainer(pcontainer)*/
        {}

        const_index_iterator( const const_index_iterator<container_t> & other )throw()
            :index_iterator(other.m_pContainer,other.m_index)/*m_index(other.m_index), m_pContainer(other.m_pContainer)*/
        {}

        virtual ~const_index_iterator()throw()
        {}

        const_index_iterator<container_t> & operator=( const const_index_iterator<container_t> & other )
        {
            m_index      = other.m_index;
            m_pContainer = other.m_pContainer;
            return *this;
        }
    };
};

#endif