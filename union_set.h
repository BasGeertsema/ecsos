// Copyright (c) 2016 Bas Geertsema <mail@basgeertsema.nl>

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#ifndef UNION_SET_H_INCLUDED
#define UNION_SET_H_INCLUDED

#include <tuple>
#include <type_traits>

namespace ecs {

// indicates whether a container can be used as the base for a union set
//
// NOTE TO APPLICATION DEVELOPERS:
//
// you must specialize this struct/trait for custom containers to enable union set
// functionality
template <class T>
struct is_union_base_set {
    static constexpr bool value = false;
};


// functor to extract the common identifier from a set element
// by default is calls the id() function on an object expecting an integer
//
// NOTE TO APPLICATION DEVELOPERS:
//
// you can either implement the id() function on your element types or
// specialize this struct for your element type
template <class T>
struct element_id {
    using type = int;

    type operator()(const T& x) const
    {
        return x.id();
    }
};

// returns the common identifier from a set element using the element_id functor
template <class T>
auto get_element_id(const T& x)
{
    return element_id<T>{}(x);
}


namespace meta {
    // tuple_has_type is a meta-helper that detect whether a tuple contains a certain type
    template <class T, class Tuple>
    struct tuple_has_type : public std::false_type {
    };

    template <class T, class... Types>
    struct tuple_has_type<T, std::tuple<T, Types...> > : public std::true_type {
    };

    template <class T, class U, class... Types>
    struct tuple_has_type<T, std::tuple<U, Types...> > : public tuple_has_type<T, std::tuple<Types...> > {
    };

    // is_subset is a meta-helper that determine whether a tuple is the subset of another tuple
    template <class TTuple, class... CTypes>
    struct is_subset : std::true_type {
    };

    template <class TTuple, class Head, class... Tail>
    struct is_subset<TTuple, Head, Tail...>
        : std::conditional<tuple_has_type<Head, TTuple>::value, is_subset<TTuple, Tail...>, std::false_type>::type {
    };

    // is_allowed_container is a meta-helper that enables only union sets on allowed containers
    template <class... CTypes>
    struct is_allowed_container : std::true_type {
    };

    template <class Head, class... Tail>
    struct is_allowed_container<Head, Tail...>
        : std::conditional<is_union_base_set<Head>::value, is_allowed_container<Tail...>, std::false_type>::type {
    };
}

// holds two iterators that point to
//  1) the current element in the set and
//  2) the end of the element set
template <class T>
struct union_set_iterator_pair {
    union_set_iterator_pair(T currentIt, T endIt) noexcept
        : current{ currentIt },
          end{ endIt }
    {
    }

    bool operator==(const union_set_iterator_pair& rhs) const
    {
        return current == rhs.current;
    }

    bool operator!=(const union_set_iterator_pair& rhs) const
    {
        return !(*this == rhs);
    }

    T current;
    T end;
};

template <class T>
union_set_iterator_pair<T> make_union_set_iterator_pair(T current, T end)
{
    return { current, end };
}

// represents a single element within the union of one ore more sets
template <class... Types>
struct union_set_el : public std::tuple<Types...> {
    using tuple_type = std::tuple<Types...>;
    using Base = std::tuple<Types...>;

    using Base::Base;

    // implicit dereferencing to reference of the value
    template <class T,
        typename = typename std::enable_if<meta::tuple_has_type<std::add_pointer_t<T>, tuple_type>::value>::type>
    operator T&()
    {
        return *(::std::get<T*>(*this));
    }

    // implicit dereferencing to const reference of the value
    template <class T,
        typename = typename std::enable_if<meta::tuple_has_type<std::add_pointer_t<T>, tuple_type>::value>::type>
    operator const T&() const
    {
        return *(::std::get<T*>(*this));
    }

    // constructor to create a tuple from a superset tuple
    template <class... CTypes,
        typename = typename std::enable_if<meta::is_subset<typename union_set_el<CTypes...>::tuple_type, Types...>::value>::type>
    union_set_el(const union_set_el<CTypes...>& x)
        : Base{ &*(std::get<Types>(x))... }
    {
    }
};

// get a const reference to the value in the specified set in the union set element
template <class T, class... Types>
inline const T& get(const union_set_el<Types...>& x)
{
    return *(std::get<std::add_pointer_t<T> >(x));
}

// get a reference to the value in the specified set in the union set element
template <class T, class... Types>
inline T& get(union_set_el<Types...>& x)
{
    return *(std::get<std::add_pointer_t<T> >(x));
}

// get a reference to the value in the specified set in the temporary union set element
template <class T, class... Types>
inline T& get(union_set_el<Types...>&& x)
{
    return *(std::get<std::add_pointer_t<T> >(x));
}

// a forward iterator over a union set. the types specified are references.
// the iterator holds a tuple of iterator pairs, which point
// to the current value and and end in the invidiual sets.
// upon construction the iterator will search for the first value
// that is present in all sets, such that dereferencing is directly available.
// upon dereferencing, a union_set_el object is created and returned. The union_set_el
// holds the pointers to the values in the underlying sets.
template <class... Types>
struct union_set_iterator {

    // determine the type of the element identifier based on just the first set.
    // note that this assumes that all elements in all sets use the same type for the element identifier.
    using element_id_type = typename element_id<std::tuple_element_t<0, std::tuple<typename Types::reference...> > >::type;

    // value_type is a union set selement
    // that holds pointers to all values in the underlying sets
    using value_type = union_set_el<typename Types::pointer...>;

    // constructs the iterator with iterator pairs for each set
    // upon construction the iterator is immediately advanced to the first
    // element that is present in all sets, or the end iterators if such an
    // element cannot be found
    union_set_iterator(union_set_iterator_pair<Types>... args) noexcept
        : sets_{ std::make_tuple(args...) }
    {
        start();
    }

    // upon dereferencing a union set selement is created that holds pointers
    // to all values in the underlying sets
    value_type operator*()
    {
        return { &*(std::get<union_set_iterator_pair<Types> >(sets_).current)... };
    }

    // equal when all corresponding iterators in all corresponding sets are equal
    bool operator==(const union_set_iterator& rhs) const
    {
        return sets_ == rhs.sets_;
    }

    // not equal when any corresponding iterator in any corresponding set is not equal
    bool operator!=(const union_set_iterator& rhs) const
    {
        return !(*this == rhs);
    }

    // advances to the next element that is present in all sets
    // if there is no such element, then it advances to the end position
    void operator++()
    {            
        // the iterator for the first set is always incremented to ensure
        // at least a single advance within the sets
        ++(std::get<max_index()>(sets_).current);

        // advance an iterator until both iterators point
        // at the same common id, or if any iterator points to the end
        if (max_index() > 0 && std::get<max_index()>(sets_).current == std::get<max_index()>(sets_).end) {
            // set all iterators to the end
            advance_all_to_end();
            return;
        }
        if (max_index() > 0) {
            // get the current value identifier
            max_ = get_element_id(*(std::get<max_index()>(sets_).current));

            // now increment the others until a match is found for all sequences
            // tag dispatch will ensure that this check does not happen at runtime for single sets
            while (!advance_until<max_index()>(has_single_set)) {
                // continue iterating over the sets
            }
        }
    }

protected:
    using has_single_set_t = std::integral_constant<bool, sizeof...(Types) == 1>;
    static constexpr has_single_set_t has_single_set{};

    static constexpr size_t max_index()
    {
        return sizeof...(Types)-1;
    }

    void start()
    {
        // if any set is empty, then reset all sequences to the end
        // due to tag dispatching, the check is only performed at runtime if there are indeed multiple sets
        if (max_index() != 0 && any_at_end()) {
            advance_all_to_end();
            return;
        }
        max_ = std::get<max_index()>(sets_).current->Id;
        if (!advance_until<max_index() - 1>(has_single_set)) {
            // if the others have not the same elements, increase
            operator++();
        }
    }

    template <size_t N>
    void advance_to_end(std::true_type)
    {
        static_assert(N == 0, "terminating");
        std::get<N>(sets_).current = std::get<N>(sets_).end;
    }

    template <size_t N>
    void advance_to_end(std::false_type)
    {
        static_assert(N > 0, "non terminating");
        std::get<N>(sets_).current = std::get<N>(sets_).end;
        advance_to_end<N - 1>(std::integral_constant<bool, (N - 1) == 0>{});
    }

    void advance_all_to_end()
    {
        advance_to_end<max_index()>(has_single_set);
    }

    template <std::size_t N>
    bool at_end(std::true_type) const
    {
        static_assert(N == 0, "terminating");
        return std::get<N>(sets_).current == std::get<N>(sets_).end;
    }

    template <std::size_t N>
    bool at_end(std::false_type) const
    {
        static_assert(N > 0, "non terminating");
        return std::get<N>(sets_).current == std::get<N>(sets_).end
            || at_end<N - 1>(std::integral_constant<bool, (N - 1) == 0>{});
    }

    bool any_at_end() const
    {
        return at_end<max_index()>(has_single_set);
    }

    template <size_t N>
    bool advance_until(std::false_type)
    {
        while (get_element_id(*std::get<N>(sets_).current) < max_) {
            ++std::get<N>(sets_).current;
            if (std::get<N>(sets_).current == std::get<N>(sets_).end) {
                advance_all_to_end();
                return true;
            }
        }

        if (max_ < get_element_id(*std::get<N>(sets_).current)) {
            max_ = get_element_id(*std::get<N>(sets_).current);
            return false;
        }

        return advance_until<N - 1>(std::integral_constant<bool, N == 0>());
    }

    template <size_t N>
    bool advance_until(std::true_type)
    {
        return true;
    }

    element_id_type max_{ 0 };
    std::tuple<union_set_iterator_pair<Types>...> sets_;
};

template <class... Types>
union_set_iterator<Types...> make_union_set_iterator(union_set_iterator_pair<Types>... args)
{
    return { args... };
}

// a union_set represents the union of one ore more base sets
// elements within the sets are compared using the element_id functor to extract the identifier
// the base sets are assumed to be ordered at all time!
template <class... TSet>
struct union_set {
    union_set(TSet&... sets)
    {
        // store pointers to the sets in a tuple
        sets_ = std::make_tuple((&sets)...);
    }

    template <class T>
    using value_type = typename std::remove_reference_t<T>::value_type;

    using element_id_type = typename element_id<std::tuple_element_t<0, std::tuple<value_type<TSet>...> > >::type;

    auto find(value_type<TSet>&&... key)
    {
        return make_union_set_iterator(
            make_union_set_iterator_pair(
                std::get<TSet*>(sets_)->find(std::forward<value_type<TSet> >(key)),
                std::get<TSet*>(sets_)->end())...);
    }

    auto find(element_id_type key)
    {
        return make_union_set_iterator(
            make_union_set_iterator_pair(
                std::get<TSet*>(sets_)->find({ key }),
                std::get<TSet*>(sets_)->end())...);
    }

    auto begin()
    {
        return make_union_set_iterator(
            make_union_set_iterator_pair(
                std::get<TSet*>(sets_)->begin(),
                std::get<TSet*>(sets_)->end())...);
    }

    auto end()
    {
        return make_union_set_iterator(
            make_union_set_iterator_pair(
                std::get<TSet*>(sets_)->end(),
                std::get<TSet*>(sets_)->end())...);
    }

private:
    std::tuple<std::add_pointer_t<TSet>...> sets_;
};

template <class... T>
union_set<T...> make_union_set(T&... sets)
{
    return { sets... };
}

// below are various free standing functions for ease of use

// ensure via SFINAE that only boost flatsets are accepted here
// we do use variadic arguments to allow mixing of const and non-const sets
template <class... Types, typename = std::enable_if_t<meta::is_allowed_container<Types...>::value> >
auto union_of(Types&... sets)
{
    return make_union_set(sets...);
}

template <class... Types, typename = std::enable_if_t<meta::is_allowed_container<Types...>::value> >
auto union_begin(Types&... sets)
{
    return make_union_set(sets...).begin();
}

template <class... Types, typename = std::enable_if_t<meta::is_allowed_container<Types...>::value> >
auto union_end(Types&... sets)
{
    return make_union_set(sets...).end();
}

template <class T, class... Types, typename = std::enable_if_t<meta::is_allowed_container<Types...>::value> >
auto union_find(T id, Types&... sets)
{
    return make_union_set(sets...).find(id);
}
}

namespace std {
template <class... Types>
struct iterator_traits<ecs::union_set_iterator<Types...> > {
    using difference_type = size_t;
    using value_type = typename ecs::union_set_iterator<Types...>::value_type;
    using pointer = std::add_pointer<value_type>;
    using reference = std::add_lvalue_reference<value_type>;
    using iterator_category = std::forward_iterator_tag;
};
}

#endif
