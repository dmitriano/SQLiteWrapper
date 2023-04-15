#pragma once

#include "SQLiteWrapper/Exception.h"

#include "Awl/Stringizable.h"
#include "Awl/TupleHelpers.h"
#include "Awl/StringFormat.h"
#include "Awl/BitMap.h"
#include "Awl/Separator.h"

#include <array>
#include <vector>
#include <optional>
#include <string>
#include <unordered_set>

namespace sqlite
{
    //We need to construct it from a tuple of field pointers.
    using IndexFilter = std::unordered_set<size_t>;
    using OptionalIndexFilter = std::optional<IndexFilter>;

    constexpr size_t noIndex = static_cast<size_t>(-1);

    constexpr const char rowIdFieldName[] = "rowId";
}

namespace sqlite::helpers
{
    //Functions MakeSigned and MakeUnsigned should satisfy the following criteria:
    //For a given pair of parameters a, b and return values a1, b1, if a <= b then a1 <= b1.
    //and for a given unsigned a
    //MakeUnsigned(MakeSigned(a)) == a

    template <typename T>
    constexpr T FlipSignBit(T val)
    {
        return val ^ (static_cast<T>(1) << (sizeof(T) * 8 - 1));
    }

    template <typename T>
    constexpr typename std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, std::make_signed<T>>::type MakeSigned(T val)
    {
        return FlipSignBit(val);
    }

    template <typename T>
    constexpr typename std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, std::make_unsigned<T>>::type MakeUnsigned(T val)
    {
        return FlipSignBit(val);
    }

    static_assert(MakeSigned(std::numeric_limits<unsigned int>::min()) == std::numeric_limits<int>::min());
    static_assert(MakeSigned(std::numeric_limits<unsigned int>::max()) == std::numeric_limits<int>::max());
    static_assert(MakeSigned(std::numeric_limits<uint64_t>::min()) == std::numeric_limits<int64_t>::min());
    static_assert(MakeSigned(std::numeric_limits<uint64_t>::max()) == std::numeric_limits<int64_t>::max());

    static_assert(MakeUnsigned(std::numeric_limits<int>::min()) == std::numeric_limits<unsigned int>::min());
    static_assert(MakeUnsigned(std::numeric_limits<int>::max()) == std::numeric_limits<unsigned int>::max());
    static_assert(MakeUnsigned(std::numeric_limits<int64_t>::min()) == std::numeric_limits<uint64_t>::min());
    static_assert(MakeUnsigned(std::numeric_limits<int64_t>::max()) == std::numeric_limits<uint64_t>::max());

    static_assert(MakeUnsigned(std::numeric_limits<int>::min() + 25) == 25u);
    static_assert(MakeUnsigned(std::numeric_limits<int>::max() - 25) == std::numeric_limits<unsigned int>::max() - 25u);
    static_assert(MakeUnsigned(std::numeric_limits<int64_t>::min() + 25) == 25u);
    static_assert(MakeUnsigned(std::numeric_limits<int64_t>::max() - 25) == std::numeric_limits<uint64_t>::max() - 25u);

    static_assert(MakeSigned(25u) == std::numeric_limits<int>::min() + 25);
    static_assert(MakeSigned(std::numeric_limits<unsigned int>::max() - 25u) == std::numeric_limits<int>::max() - 25);
    static_assert(MakeSigned(std::numeric_limits<uint64_t>::min() + 25u) == std::numeric_limits<int64_t>::min() + 25);
    static_assert(MakeSigned(std::numeric_limits<uint64_t>::max() - 25u) == std::numeric_limits<int64_t>::max() - 25);

    template <typename T>
    struct IsOptional : std::false_type {};

    template <class T>
    struct IsOptional<std::optional<T>> : std::true_type {};

    template <typename T>
    inline constexpr bool IsOptionalV = IsOptional<T>::value;

    template <typename T>
    struct RemoveOptional { typedef T type; };

    template <typename T>
    struct RemoveOptional<std::optional<T>> { typedef T type; };

    template <typename T>
    using RemoveOptionalT = typename RemoveOptional<T>::type;

    template <class Tuple, class Func, std::size_t... index>
    inline constexpr void ForEachTF(Func f, std::index_sequence<index...>)
    {
        (f(std::integral_constant<std::size_t, index>()), ...);
    }

    template <class Tuple, class Func>
    inline constexpr void ForEachTF(Func f)
    {
        ForEachTF<Tuple>(f, std::make_index_sequence<std::tuple_size_v<Tuple>>());
    }

    template <class T>
    struct TypeDescriptor
    {
        using Type = T;
    };

    template <class T, typename Func>
    inline size_t ForEachFieldTypeImpl(std::vector<std::string_view>& prefixes, Func&& func, size_t startIndex)
    {
        size_t count = 0;

        using Tie = typename awl::tuplizable_traits<T>::Tie;

        ForEachTF<Tie>([&prefixes, &count, &startIndex, &func](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_stringizable_v<FieldType>)
            {
                const auto& member_names = T::get_member_names();

                const std::string& name = member_names[fieldIndex];

                prefixes.push_back(name);

                //It returns std::tuple_size_v<Tie> + subtree size.
                count += ForEachFieldTypeImpl<FieldType>(prefixes, func, startIndex + count);

                prefixes.pop_back();
            }
            else
            {
                func(prefixes, fieldIndex, startIndex + count, TypeDescriptor<T>{}, TypeDescriptor<FieldType>{});

                ++count;
            }
        });

        return count;
    }

    template <class T, typename Func>
    inline size_t ForEachFieldType(Func&& func)
    {
        std::vector<std::string_view> prefixes;

        return ForEachFieldTypeImpl<T>(prefixes, func, 0);
    }

    template <class Struct, class T>
    size_t FindFieldIndex(T Struct::* fieldPtr)
    {
        Struct instance = {};

        size_t foundIndex = std::numeric_limits<size_t>::max();

        awl::for_each_index(instance.as_tuple(), [fieldPtr, &instance, &foundIndex](auto& field, auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (std::is_same_v<FieldType, T>)
            {
                if (&field == &(instance.*fieldPtr))
                {
                    foundIndex = fieldIndex;
                }
            }
            else
            {
                static_cast<void>(fieldIndex);
            }
        });

        assert(foundIndex != std::numeric_limits<size_t>::max());

        return foundIndex;
    }

    template <class T>
    constexpr inline size_t GetFieldCount()
    {
        size_t count = 0;
        
        using Tie = typename awl::tuplizable_traits<T>::Tie;

        ForEachTF<Tie>([&count](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_stringizable_v<FieldType>)
            {
                count += GetFieldCount<FieldType>();
            }
            else
            {
                ++count;
            }
        });

        return count;
    }

    //TO DO: Make it possible to find an index of the tuple of field pointers.
    //Now it finds only a top-level index.
    template <class Struct, class T>
    size_t FindTransparentFieldIndex(T Struct::* fieldPtr)
    {
        Struct instance = {};

        size_t foundIndex = std::numeric_limits<size_t>::max();

        size_t count = 0;

        awl::for_each(instance.as_tuple(), [fieldPtr, &instance, &foundIndex, &count](auto& field)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (awl::is_stringizable_v<FieldType>)
            {
                count += GetFieldCount<FieldType>();
            }
            else
            {
                if constexpr (std::is_same_v<FieldType, T>)
                {
                    if (&field == &(instance.*fieldPtr))
                    {
                        foundIndex = count;
                    }
                }

                ++count;
            }
        });

        assert(foundIndex != std::numeric_limits<size_t>::max());

        return foundIndex;
    }

    template <class Value, class... Field>
    IndexFilter FindTransparentFieldIndices(std::tuple<Field Value::*...> field_ptrs)
    {
        IndexFilter indices;

        awl::for_each(field_ptrs, [&indices](auto& field_ptr)
        {
            const size_t index = helpers::FindTransparentFieldIndex(field_ptr);

            indices.insert(index);
        });

        return indices;
    }

    /*
    template <class T, class Tuple, std::size_t level_index>
    constexpr inline size_t FindFieldIndexImpl(Tuple t)
    {
        size_t count = 0;

        using Tie = typename awl::tuplizable_traits<T>::Tie;

        ForEachTF<Tie>([&count](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_stringizable_v<FieldType>)
            {
                return FindFieldIndex<FieldType, Tuple, level_index>(t);
            }
            else
            {


                ++count;
            }
        });

        return count;
    }
    */

    template <class T, typename Func>
    constexpr size_t ForEachFieldValueImpl(T& val, Func&& func, size_t start_index)
    {
        size_t count = 0;

        awl::for_each(val.as_tuple(), [start_index, &count, &func](auto& field)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (awl::is_stringizable_v<FieldType>)
            {
                count += ForEachFieldValueImpl(field, func, start_index + count);
            }
            else
            {
                func(field, start_index + count);

                ++count;
            }
        });

        return count;
    }

    template <class T, typename Func>
    constexpr void ForEachFieldValue(T& val, Func&& func)
    {
        ForEachFieldValueImpl(val, func, 0);
    }

    inline std::string MakeFullFieldName(std::vector<std::string_view>& prefixes, std::string_view name)
    {
        std::ostringstream out;

        awl::aseparator sep("_");

        for (auto prefix : prefixes)
        {
            out << sep << prefix;
        }

        out << sep << name;

        return out.str();
    }
}
