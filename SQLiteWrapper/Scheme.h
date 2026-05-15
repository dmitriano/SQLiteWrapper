#pragma once

#include "SQLiteWrapper/Element.h"

#include "Awl/Observable.h"
#include "Awl/Observer.h"

#include <functional>

namespace sqlite
{
    class Scheme :
        public awl::Observable<Element, Scheme>,
        public awl::Observer<Element>
    {
    public:

        Scheme() = default;

        Scheme(const Scheme&) = delete;
        Scheme& operator = (const Scheme&) = delete;

        Scheme(Scheme&&) = default;
        Scheme& operator = (Scheme&&) = default;

        void create(Database& db) override
        {
            notify(&Element::create, std::ref(db));
        }

        void drop(Database& db) override
        {
            notify(&Element::drop, std::ref(db));
        }

        using awl::Observable<Element, Scheme>::subscribe;
        using awl::Observable<Element, Scheme>::unsubscribe;
    };
}
