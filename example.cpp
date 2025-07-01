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

#include "ecs.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>

// for convenience use the entity-component-system namespace
using namespace ecs;

// About this example
//
// In this example we have three components: Transform, RigidBody, Character.
// An entity consists of one or more components. There is no explicit entity object, it
// is implicitly defined by the presence of its components. All components are uniquely
// identified by an integer in this example, although other types can be used if required.
//
// We use ComponentBase as a base class to ease the implementation of the identifier and
// comparison operators for the components. Note that it is _not_ required to have a
// specific base class to use this ECS!

struct ComponentBase {
    ComponentBase() = default;

    ComponentBase(int id) noexcept
        : Id{ id }
    {
    }

    int Id{ 0 };

    int id() const
    {
        // this function is by default used
        // by the entity-component system to identify
        // elements, but this behaviour can be overridden
        // by specializing the ecs::element_id functor (see below)
        return Id;
    }

    // provide comparison operators because we use _ordered_ sets
    bool operator<(const ComponentBase& rhs) const
    {
        return Id < rhs.Id;
    }

    bool operator==(const ComponentBase& rhs) const
    {
        return Id == rhs.Id;
    }
};

struct Transform : public ComponentBase {
    using ComponentBase::ComponentBase;

    Transform(int id, float x, float y, float z)
        : ComponentBase(id)
        , X{ x }
        , Y{ y }
        , Z{ z }
    {
    }

    float X{ 0 };
    float Y{ 0 };
    float Z{ 0 };
};

struct RigidBody : public ComponentBase {
    using ComponentBase::ComponentBase;

    RigidBody(int id, float mass)
        : ComponentBase(id)
        , Mass{ mass }
    {
    }

    float Mass;
};

struct Character : public ComponentBase {
    using ComponentBase::ComponentBase;

    Character(int id, std::string archetype)
        : ComponentBase(id)
        , Archetype{ archetype }
    {
    }

    std::string Archetype;
};

namespace ecs {
// as an example we override the identifier functor
// for the Character component to show how you can
// override the default accessor. note that different
// components can use different accessor functors.
template <>
struct element_id< ::Character> {
    using type = int;
    auto operator()(const Character& x) const
    {
        return x.Id;
    }
};
}

// the functions below show how iterators convert
// implicitly to components and subsets of entities

void HandleTransform(entity<Transform, RigidBody> e)
{
}

void HandleRigidbody(entity<RigidBody> e)
{
}

void HandleRigidbodyByRef(const RigidBody& body)
{
}

void HandleTransformRigidbodyByRef(RigidBody& body, Transform& transform)
{
}

void HandleCharacter(entity<Character> e)
{
}

void HandleTransformRigidBody(entity<Transform, RigidBody> e)
{

    // entity<> are tuples with pointers so cheap to copy as arguments ..
    HandleTransform(e);

    // .. this also allows for for converting to smaller entity subsets
    HandleRigidbody(e);

    // implicit dereferencing to references are supported
    HandleRigidbodyByRef(e);
    HandleTransformRigidbodyByRef(e, e);

    // the below will not compile as Character is not a component in the
    // entity supplied here as argument ..
    // HandleCharacter(e);
}

int main(int argc, char** argv)
{
    // use ordered sets as our 'base' sets holding the components
    component_set<Transform> transforms;
    component_set<RigidBody> bodies;
    component_set<Character> characters;

    // construct three entities, each having different components
    // note that an entity is uniquely identified by its (integer) id
    // and implicitly defined by it's components, i.e. there is no
    // explicit 'entity' object that we construct

    transforms.emplace(1, 2.f, 3.f, 4.f);
    bodies.emplace(1, 120.f);

    transforms.emplace(2, 5.f, 7.f, 8.f);
    bodies.emplace(2, 120.f);
    characters.emplace(2, "Hero");

    transforms.emplace(3, 15.f, -7.f, 8.f);
    characters.emplace(3, "Warlord");

    // first we will search for an entity in a single component set
    // note that this will use the find method() on the ordered set and
    // will therefore run in O(log N) as a binary search is used
    assert(entities_find(1, transforms) != entities_end(transforms));

    // a non-existing entity will refer to the end
    assert(entities_find(100, transforms) == entities_end(transforms));

    {
        // we can get the component of an entity by using the get<T> function

        // first dereference the iterator, this will gave an entity object
        entity<Transform> entity = *entities_find(2, transforms);

        // then call get<T> to get a reference to the component
        Transform& transform = get<Transform>(entity);

        // check whether the right object is found
        assert(transform.X == 5.f);
    }

    // now we will search for an entity that has both a Transform and RigidBody component
    // there are two: entity #1 and entity #2
    assert(entities_find(1, transforms, bodies) != entities_end(transforms, bodies));
    assert(entities_find(2, transforms, bodies) != entities_end(transforms, bodies));
    assert(entities_find(3, transforms, bodies) == entities_end(transforms, bodies));

    // another way is to first declare the _union_ of all entities having both a Transform and RigidBody component
    auto entitiesWithTransformsAndBodies = entities(transforms, bodies);

    // now we can use methods directly on this union
    assert(entitiesWithTransformsAndBodies.find(1) != entities_end(transforms, bodies));
    assert(std::distance(entities_begin(transforms, bodies), entities_end(transforms, bodies)) == 2);

    // or to find the size. note that we use std::distance as there is no size() method.
    // this is because entity unions will always be iterated and therefore
    // is, roughly, a O(N) operation which is counter-intuitive compared with other size() methods.
    assert(std::distance(entitiesWithTransformsAndBodies.begin(), entitiesWithTransformsAndBodies.end()) == 2);

    // in the same way we can use a union of three (or many more) component sets and
    // find entities that contains all three components: Transform, RigidBody and Character
    assert(entities_find(2, transforms, bodies, characters) != entities_end(transforms, bodies, characters));
    assert(entities_find(1, transforms, bodies, characters) == entities_end(transforms, bodies, characters));

    auto entitiesWithAllComponents = entities(transforms, bodies, characters);
    assert(std::distance(entitiesWithAllComponents.begin(), entitiesWithAllComponents.end()) == 1);

    // .. we can iterate using a regular for loop
    {
        int cnt = 0;
        for (auto it = entities_begin(transforms, bodies); it != entities_end(transforms, bodies); ++it) {
            Transform& transform = get<Transform>(*it);
            RigidBody& body = get<RigidBody>(*it);
            ++cnt;
        }
        assert(cnt == 2);
    }

    // .. or use the range-based for loop
    {
        int cnt = 0;
        for (auto entity : entities(transforms, bodies)) {
            Transform& transform = get<Transform>(entity);
            RigidBody& body = get<RigidBody>(entity);
            assert(transform.X > 0.f);
            ++cnt;
        }
        assert(cnt == 2);
    }

    // the constness of the underlying sets is preserved
    // const and non-const sets can also be mixed
    const auto& transforms_const = transforms;
    for (auto entity : entities(transforms_const, bodies)) {
        auto& transform = get<const Transform>(entity);
        static_assert(std::is_const<typename std::remove_reference<decltype(transform)>::type>::value, "Transform should be const");

        // requesting a non-const reference such as below would give a compile-error
        // auto& transform = get<Transform>(entity);

        // note that the RigidBody component remains non-const
        RigidBody& body = get<RigidBody>(entity);
        static_assert(!std::is_const<typename std::remove_reference<decltype(body)>::type>::value, "RigidBody should not be const");
    }

    // dereferencing a union set iterator returns a entity object
    // the entity object contains
    for (auto entity : entities(transforms, bodies, characters)) {
        // note that this function expects an entity with only two components: Transform and RigidBodyu
        // but our entity which contains is the extra Character component is accepted (a subset is copied to the function)
        HandleTransformRigidBody(entity);
    }

    // make use of find_if algorithm
    auto warlord = std::find_if(entities_begin(characters, transforms), entities_end(characters, transforms), [](const auto& x) {
        return get<Character>(x).Archetype == "Warlord";
    });
    assert(warlord != entities_end(characters, transforms));
    assert(get<Character>(*warlord).Archetype == "Warlord");

    // make use of count_if algorithm
    assert(std::count_if(entities_begin(transforms), entities_end(transforms), [](auto x) {
        return get<Transform>(x).Y < 0.f;
    }) == 1);

    std::cout << "ECSOS example finished" << std::endl;

    return 0;
}
