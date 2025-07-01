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

#ifndef ECS_H_INCLUDED
#define ECS_H_INCLUDED

// generic code for handling unions of ordered sets
#include "union_set.h"

// functions specialized towards boost::container::flat_set
#include "ecs_flatset.h"

namespace ecs {
// use convenient types and functions directed towards
// ECS terminology rather than the more abstract union set types

template <class... T>
using entity = union_set_el<typename std::add_pointer<T>::type...>;

template <class T>
using component_set = boost::container::flat_set<T>;

template <class... T>
inline auto entities(T&... sets)
{
    return union_of(sets...);
}

template <class... T>
inline auto entities_begin(T&... sets)
{
    return ecs::union_begin(sets...);
}

template <class... T>
inline auto entities_end(T&... sets)
{
    return ecs::union_end(sets...);
}

template <class R, class... T>
inline auto entities_find(R id, T&... sets)
{
    return ecs::union_find(id, sets...);
}
}

#endif