/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 * 
 * \file   attribute_values_view.cpp
 * \author Andrey Semashev
 * \date   19.04.2007
 * 
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <new>
#include <cassert>
#include <boost/log/attributes/attribute_values_view.hpp>
#include "unordered_mmap_facade.hpp"

namespace boost {

namespace log {

namespace aux {

    //! Default constructor
    template< typename T >
    inline reduced_vector< T >::reduced_vector() : m_pBegin(0), m_pEnd(0), m_pEOS(0) {}

    //! Copy constructor
    template< typename T >
    inline reduced_vector< T >::reduced_vector(reduced_vector const& that)
        : m_pBegin(allocator_type::allocate(that.size())), m_pEnd(m_pBegin), m_pEOS(m_pBegin + that.size())
    {
        for (const_iterator it = that.begin(); it != that.end(); ++it)
            this->push_back(*it); // won't throw
    }

    //! Destructor
    template< typename T >
    inline reduced_vector< T >::~reduced_vector()
    {
        for (pointer p = m_pBegin; p != m_pEnd; ++p)
            p->~value_type();
        allocator_type::deallocate(m_pBegin, m_pEOS - m_pBegin);
    }

    //! Assignment
    template< typename T >
    inline reduced_vector< T >& reduced_vector< T >::operator= (reduced_vector const& that)
    {
        reduced_vector tmp(that);
        this->swap(tmp);
        return *this;
    }

    //  Iterator acquirement
    template< typename T >
    inline typename reduced_vector< T >::iterator reduced_vector< T >::begin() { return m_pBegin; }
    template< typename T >
    inline typename reduced_vector< T >::iterator reduced_vector< T >::end() { return m_pEnd; }
    template< typename T >
    inline typename reduced_vector< T >::const_iterator reduced_vector< T >::begin() const { return m_pBegin; }
    template< typename T >
    inline typename reduced_vector< T >::const_iterator reduced_vector< T >::end() const { return m_pEnd; }

    //  Accessors
    template< typename T >
    inline typename reduced_vector< T >::size_type reduced_vector< T >::size() const { return (m_pEnd - m_pBegin); }
    template< typename T >
    inline bool reduced_vector< T >::empty() const { return (size() == 0); }

    //! Swaps two containers
    template< typename T >
    inline void reduced_vector< T >::swap(reduced_vector& that)
    {
        using std::swap;
        swap(m_pBegin, that.m_pBegin);
        swap(m_pEnd, that.m_pEnd);
        swap(m_pEOS, that.m_pEOS);
    }

    //! Storage reservation
    template< typename T >
    inline void reduced_vector< T >::reserve(size_type n)
    {
        // Should be called once, before any insertions
        assert(m_pBegin == 0);
        m_pBegin = m_pEnd = allocator_type::allocate(n);
        m_pEOS = m_pBegin + n;
    }

    //! Appends a new value to the end of the container
    template< typename T >
    inline void reduced_vector< T >::push_back(const_reference x)
    {
        // Should be called after reservation
        assert(m_pBegin != 0);
        assert(m_pEnd < m_pEOS);
        new (m_pEnd) value_type(x);
        ++m_pEnd;
    }

    //! The method extracts attribute values and arranges them in the container
    template< typename T >
    template< typename IteratorT >
    inline void reduced_vector< T >::adopt_nodes(IteratorT& it, IteratorT end, unsigned char HTIndex)
    {
        for (; it != end && it->m_HTIndex == HTIndex; ++it, ++m_pEnd)
        {
            new (m_pEnd) value_type(it->first, it->second->get_value(), HTIndex);
        }
    }

} // namespace aux

//! Assignment
template< typename CharT >
basic_attribute_values_view< CharT >& basic_attribute_values_view< CharT >::operator= (basic_attribute_values_view const& that)
{
    if (this != &that)
    {
		basic_attribute_values_view tmp(that);
		this->swap(tmp);
    }
	return *this;
}

//! The constructor adopts three attribute sets to the view
template< typename CharT >
basic_attribute_values_view< CharT >::basic_attribute_values_view(
    attribute_set const& source_attrs,
    attribute_set const& thread_attrs,
    attribute_set const& global_attrs)
{
    typedef typename attribute_set::node_container attribute_set_nodes;
    typedef typename attribute_set_nodes::const_iterator attribute_set_nodes_iterator;

    node_container& Nodes = this->nodes();

    // The view should be empty when the method is being called
    assert(Nodes.empty());
    Nodes.reserve(source_attrs.size() + thread_attrs.size() + global_attrs.size());

    attribute_set_nodes const& SourceAttrs = source_attrs.nodes();
    attribute_set_nodes_iterator itSource = SourceAttrs.begin();
    attribute_set_nodes_iterator itSourceEnd = SourceAttrs.end();
    attribute_set_nodes const& ThreadAttrs = thread_attrs.nodes();
    attribute_set_nodes_iterator itThread = ThreadAttrs.begin();
    attribute_set_nodes_iterator itThreadEnd = ThreadAttrs.end();
    attribute_set_nodes const& GlobalAttrs = global_attrs.nodes();
    attribute_set_nodes_iterator itGlobal = GlobalAttrs.begin();
    attribute_set_nodes_iterator itGlobalEnd = GlobalAttrs.end();

    while (itSource != itSourceEnd || itThread != itThreadEnd || itGlobal != itGlobalEnd)
    {
        // Determine the least hash value of the current elements in each container
        register unsigned char HTIndex = (std::numeric_limits< unsigned char >::max)();
        if (itSource != itSourceEnd && itSource->m_HTIndex < HTIndex)
            HTIndex = itSource->m_HTIndex;
        if (itThread != itThreadEnd && itThread->m_HTIndex < HTIndex)
            HTIndex = itThread->m_HTIndex;
        if (itGlobal != itGlobalEnd && itGlobal->m_HTIndex < HTIndex)
            HTIndex = itGlobal->m_HTIndex;

        // Insert nodes to the view that correspond to the selected bucket
        // It is safe since no reallocations will occur because of memory reservation above
        Nodes.adopt_nodes(itSource, itSourceEnd, HTIndex);
        Nodes.adopt_nodes(itThread, itThreadEnd, HTIndex);
        Nodes.adopt_nodes(itGlobal, itGlobalEnd, HTIndex);
    }

    // Rebuild hash table
    this->rehash();
}

//! Explicitly instantiate container implementation
namespace aux {
    template class reduced_vector< unordered_multimap_facade< attribute_values_view_descr< char > >::node >;
    template class reduced_vector< unordered_multimap_facade< attribute_values_view_descr< wchar_t > >::node >;
    template class unordered_multimap_facade< attribute_values_view_descr< char > >;
    template class unordered_multimap_facade< attribute_values_view_descr< wchar_t > >;
} // namespace aux
template class basic_attribute_values_view< char >;
template class basic_attribute_values_view< wchar_t >;

} // namespace log

} // namespace boost
