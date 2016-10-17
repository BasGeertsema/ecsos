# ECSOS - Entity Component System based on Ordered Sets
ECSOS is a small header-only C++14 library that provides a _data-oriented_ _entity-component-system_ (ECS) based on _ordered sets_. By default it uses the boost ``flat_set`` container, which stores set elements (components) in a _cache-friendly_ _contiguous_ array, although other (custom) containers might be used.

## Why should I use it?
Entity-Component-Systems are commonly used in games where entities consist of one ore more components. The use of an ECS provides some advantages: components can be removed and added during runtime and components can efficiently be processed by systems operating on this data because of the compact data-layout. This is in contrast with Object-Oriented design in which entity types are determined during compile time and where entities are large heterogeneous objects with different datasizes, which leads to pointer-chasing in the heap which is often slower than straight-forward traversal of arrays.

Some characteristics of using ordered sets in an ECS are:

* fast traversal of a single component set and _unions_ of component sets
* reasonably fast lookup of individual components
* convenient value semantics to store and copy state (no pointers involved)

## How does it work?

### Entities and identities
Each component contains an identifier (typically an integer, but can be any type) that defines the entity that it belongs to. Entities are _implicitly_ defined by their components. Therefore only _components_ are ever stored: there are no distinct entity objects stored in the system. For example, if you would have two component of types `Transform` and `RigidBody`, and both components would have an identifier of '2' then these components would implicitly be part of the same entity. Note that components are typically very simple data structs.

### Finding and traversing entities
A common problem in ECS is how to work on entities that must contain at least two or more predefined components. For example, let's say you have a component type `Transform` that defines the world transformation of an entity and is used by the renderer. Furthermore, you also have another component type `RigidBody` that contains the transform of its _collision object_ and is used by the physics engine. In each frame the physics engine updates the positions in the `RigidBody` component. But, just before rendering, you would like to propagate the new positions of the `RigidBody` component to the `Transform` component such that the entity is rendered properly. Because the components are _disjoint_ in memory you somehow have to figure out how to lookup the related `Transform` component for each `RigidBody` component that is associated with the same entity.

In ECSOS, this is solved by making use of the fact that the sets are known to be _ordered_. For a single entity, you can easily lookup another component of the same entity based on the entity identifier. This is a _binary search_ and thus has a lookup cost of, at worst, O(log N). 

However, in many scenarios you would want to iterate over _all_ entities that contain components in a _specific_ list of component types. For example if you would like to iterate over all entities that have at least two components of both type `Transform` and `RigidBody`. The iteration over these entities is done by having multiple iterators, one for each component set, that advances through all components and skips over any components of which the entity id is _not_ found in any of the other set(s). This might sound complicated but fortunately this is abstracted away by the ECSOS library, as you will see. In your application you will typically work with a intuitive single iterator that works on the _union_ of component sets.

The following example will make it clear. In the following example we iterate over all entities that contain both a component of type `Transform` and a component of type `RigidBody`. These components are respectively stored in the sets `transforms` and `bodies`:

``` c++
for (auto it = entities_begin(transforms, bodies); it != entities_end(transforms, bodies); ++it) {
    Transform& transform = get<Transform>(*it);
    RigidBody& body = get<RigidBody>(*it); 
}
```

Equivalently, we can use the auto range loop:

``` c++
for (auto entity : entities(transforms, bodies)) {
    Transform& transform = get<Transform>(entity);
    RigidBody& body = get<RigidBody>(entity);            
}
```

The examples above show that the usage of ECSOS is quite straight-foward and intuitive.

### Accessing components
Individual components can be accessed by using the `get<>` function, as can be seen in the examples above. The `entity` variable in the example above holds a light-weight temporary object of the template type `entity<>`. A `entity<>` object contains pointers to its components and can therefore efficiently be copied to other functions as an argument. For example:

``` c++
void HandleTransformRigidBody(entity<Transform, RigidBody> e)
{
}

entity<Transform, RigidBody> e;
HandleRigidbodyByRef(e);	// this is ok, because the entity object is cheap to copy
```

An entity object can implicitly be converted to subsets if possible. For example, the entity in the example above contains a `Transform`, `RigidBody` and `Character` component but can also be given as an argument to the following function, that expects an entity containing only a `Transform` and `Character` component.

``` C++
void HandleRigidbodyTransform(entity<Transform, Character> e)
{
}

entity<Transform, RigidBody, Character> e;
HandleRigidbodyByRef(e);	// implicitly converted to a subset entity
```

The `entity` object also implicitly converts to _references_ of any of its _components_, for example:

``` C++
void HandleRigidbodyByRef(const RigidBody& body)
{
}

entity<Transform, RigidBody> e;
HandleRigidbodyByRef(e);	// implicitly converted to a reference to its RigidBody component
```

ECSOS has a very straight-forward, convenient and type-safe approach to using entities and components in your system.

## How fast is it?
Fair question, but it is hard to give a simple answer. The performance depends heavily on your usage patterns, data layout, component sizes, etc. So I cannot give you absolute values. However, you can make educated guesses on its performance for _your_ use case based on the characteristics of ECSOS.

What makes ECSOS fast:
* Components are stored in contiguous arrays. Traversing is cache-friendly and works well with pre-fetching. 
* Component sets are disjoint in memory. Systems only load the components in cache that they actually need to do their work.
* Components (state) can easily be copied and processed in parallel. Because there are no pointers to keep track of. For example, you can trivially copy and retain the whole state of the ECS for a couple of frame in a computergame to allow for pipelined execution.
* Searching the component for a particular entity is a relatively fast binary search. Which is faster than a linear search. However, it is obviously not as fast as a direct pointer and might also be slower than a lookup in a hashtable.

What might make ECSOS not so fast:
* When iterating a union of sets, all components in the sets are read to extract the entity identifier. This means that for union sets in which there is a large ratio between the component sets (i.e. there are a large number of elements in set A, and only a few in set B) a lot of data is read and subsequently skipped over. Technically we could do a smart binary search if the ratio is very high and this might be faster, but this is currently not implemented. The impact of this is largely determined by the ratio of your component sets in the union sets that you use.
* Removing or inserting an element in the middle of a ordered set means that other elements must be moved to fill the gap. Although this reduces to a comfortable memmove operation it is still slower than using unordered sets or hashmaps.

That being said, this system has served me well on my (in progress) development of a RTS game which has 'only' a couple of hundred entities that do not change a lot over time. Especially the option to create relatively fast copies of the whole state has been proven very convenient for, amongst others, pipelining, parallel processing, deserialization and testing.

So the bottomline is that you really have to experiment and measure for yourself whether this would give you enough performance for your needs.

## Are other (custom) containers supported?
Yes, you can specialize the `ecs::is_union_base_set` trait for any other (custom) container. It is assumed that the container keeps its elements ordered at all times. See [here](ecs_flatset.h) the specialization for `boost::container::flat_set`. As another example it can also work with `std::set`, however the iterators of `std::set` do not provide mutable references so I found it much less usable in practice.

## How can I use this in my project?
ECSOS is a header-only library so you only have to include three header files in your project:

* [union_set.h](union_set.h) - this is the main library
* [ecs_flatset.h](ecs_flatset.h) - contains the specialization for `boost::container::flat_set`
* [ecs.h](ecs.h) - contains some convenience helpers

You can easily combine all these headers in a single file if you believe that is more convenient.

Look at the code in [example.cpp](example.cpp) for examples on how to use the library, or inspect the header files. Optionally you can checkout the repository and build the example using cmake, but to be honest this does not give you a lot more than just inspection of the code.

This code requires a C++14 compliant compiler and has been been tested with:

* MSVC 2015 Update 3
* GCC 6.2.1
* Clang 3.8.0

All with Boost version 1.59.0.

I hope you'll find ECSOS useful and if you have any cool projects utilizing this library, let me know!

## License

> The MIT License (MIT)
>
> Copyright (c) 2016 Bas Geertsema <mail@basgeertsema.com>
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of
> this software and associated documentation files (the "Software"), to deal in
> the Software without restriction, including without limitation the rights to
> use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
> of the Software, and to permit persons to whom the Software is furnished to do
> so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.
