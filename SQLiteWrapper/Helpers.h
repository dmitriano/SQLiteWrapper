#pragma once

#include "SQLiteWrapper/Exception.h"
#include "SQLiteWrapper/IndexFilter.h"

#include "Awl/Reflection.h"
#include "Awl/TupleHelpers.h"
#include "Awl/LegacyFormat.h"
#include "Awl/BitMap.h"
#include "Awl/Separator.h"

#include <type_traits>
#include <array>
#include <vector>
#include <optional>
#include <string>
#include <set>

namespace sqlite
{
    using OptionalIndexFilter = std::optional<IndexFilter>;

    constexpr size_t noIndex = static_cast<size_t>(-1);

    constexpr const char rowIdFieldName[] = "rowId";
}

namespace sqlite::helpers
{
    template <class... Ptrs>
    struct FieldPath
    {
        std::tuple<Ptrs...> ptrs;
    };

    template <class... Ptrs>
    FieldPath<Ptrs...> fieldPath(Ptrs... ptrs)
    {
        return FieldPath<Ptrs...>{ std::make_tuple(ptrs...) };
    }

    template <class... Paths>
    auto fieldPaths(Paths... paths)
    {
        return std::make_tuple(paths...);
    }

    template <class T>
    struct IsFieldPath : std::false_type {};

    template <class... Ptrs>
    struct IsFieldPath<FieldPath<Ptrs...>> : std::true_type {};

    template <class T>
    inline constexpr bool IsFieldPathV = IsFieldPath<T>::value;

    template <class Ptr>
    struct MemberPointerTraits;

    template <class Struct, class T>
    struct MemberPointerTraits<T Struct::*>
    {
        using value_type = T;
    };

    template <class Path>
    struct FieldPathValue;

    template <class... Ptrs>
    struct FieldPathValue<FieldPath<Ptrs...>>
    {
        using last_pointer = std::tuple_element_t<sizeof...(Ptrs) - 1, std::tuple<Ptrs...>>;
        using type = typename MemberPointerTraits<last_pointer>::value_type;
    };

    // Functions makeSigned and makeUnsigned should satisfy the following criteria:
    // For a given pair of parameters a, b and return values a1, b1, if a <= b then a1 <= b1.
    // and for a given unsigned a
    // makeUnsigned(makeSigned(a)) == a
    // For example, makeSigned(static_cast<unsigned int>(0)) != 0

    template <typename T>
    constexpr T flipSignBit(T val)
    {
        return val ^ (static_cast<T>(1) << (sizeof(T) * 8 - 1));
    }

    // The assertion below fails in Apple Clang 17.
    // using T = size_t;
    // static_assert(std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) == 8);

    template <typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> && (sizeof(T) == 4 || sizeof(T) == 8)
    constexpr std::make_signed_t<T> makeSigned(T val)
    {
        return flipSignBit(val);
    }

    template <typename T> requires std::is_integral_v<T> && std::is_signed_v<T> && (sizeof(T) == 4 || sizeof(T) == 8)
    constexpr std::make_unsigned_t<T> makeUnsigned(T val)
    {
        return flipSignBit(val);
    }

    template <typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> && (sizeof(T) == 1 || sizeof(T) == 2)
    constexpr int32_t makeSigned(T val)
    {
        return static_cast<int32_t>(val);
    }

    template <typename T> requires std::is_integral_v<T> && std::is_signed_v<T> && (sizeof(T) == 1 || sizeof(T) == 2)
    constexpr std::make_unsigned_t<T> makeUnsigned(T val)
    {
        return static_cast<std::make_unsigned_t<T>>(val);
    }

    static_assert(makeSigned(std::numeric_limits<unsigned int>::min()) == std::numeric_limits<int>::min());
    static_assert(makeSigned(std::numeric_limits<unsigned int>::max()) == std::numeric_limits<int>::max());
    static_assert(makeSigned(std::numeric_limits<uint64_t>::min()) == std::numeric_limits<int64_t>::min());
    static_assert(makeSigned(std::numeric_limits<uint64_t>::max()) == std::numeric_limits<int64_t>::max());

    static_assert(makeUnsigned(std::numeric_limits<int>::min()) == std::numeric_limits<unsigned int>::min());
    static_assert(makeUnsigned(std::numeric_limits<int>::max()) == std::numeric_limits<unsigned int>::max());
    static_assert(makeUnsigned(std::numeric_limits<int64_t>::min()) == std::numeric_limits<uint64_t>::min());
    static_assert(makeUnsigned(std::numeric_limits<int64_t>::max()) == std::numeric_limits<uint64_t>::max());

    static_assert(makeUnsigned(std::numeric_limits<int>::min() + 25) == 25u);
    static_assert(makeUnsigned(std::numeric_limits<int>::max() - 25) == std::numeric_limits<unsigned int>::max() - 25u);
    static_assert(makeUnsigned(std::numeric_limits<int64_t>::min() + 25) == 25u);
    static_assert(makeUnsigned(std::numeric_limits<int64_t>::max() - 25) == std::numeric_limits<uint64_t>::max() - 25u);

    static_assert(makeSigned(25u) == std::numeric_limits<int>::min() + 25);
    static_assert(makeSigned(std::numeric_limits<unsigned int>::max() - 25u) == std::numeric_limits<int>::max() - 25);
    static_assert(makeSigned(std::numeric_limits<uint64_t>::min() + 25u) == std::numeric_limits<int64_t>::min() + 25);
    static_assert(makeSigned(std::numeric_limits<uint64_t>::max() - 25u) == std::numeric_limits<int64_t>::max() - 25);

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
    constexpr void forEachTF(Func f, std::index_sequence<index...>)
    {
        (f(std::integral_constant<std::size_t, index>()), ...);
    }

    template <class Tuple, class Func>
    constexpr void forEachTF(Func f)
    {
        forEachTF<Tuple>(f, std::make_index_sequence<std::tuple_size_v<Tuple>>());
    }

    template <class T>
    struct TypeDescriptor
    {
        using Type = T;
    };

    template <class T, typename Func>
    size_t forEachFieldTypeImpl(std::vector<std::string_view>& prefixes, Func&& func, size_t startIndex)
    {
        size_t count = 0;

        using Tie = typename awl::tuplizable_traits<T>::Tie;

        forEachTF<Tie>([&prefixes, &count, &startIndex, &func](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                const auto& member_names = T::member_names();

                const std::string& name = member_names[fieldIndex];

                prefixes.push_back(name);

                //It returns std::tuple_size_v<Tie> + subtree size.
                count += forEachFieldTypeImpl<FieldType>(prefixes, func, startIndex + count);

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
    size_t forEachFieldType(Func&& func)
    {
        std::vector<std::string_view> prefixes;

        return forEachFieldTypeImpl<T>(prefixes, func, 0);
    }

    template <class Struct, class T>
    size_t findFieldIndex(T Struct::* fieldPtr)
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

    template <class Struct, class T>
    const std::string& findFieldName(T Struct::* field_ptr)
    {
        const std::size_t index = findFieldIndex(field_ptr);

        return Struct::member_names()[index];
    }
        
    template <class T>
    constexpr inline size_t fieldCount()
    {
        size_t count = 0;
        
        using Tie = typename awl::tuplizable_traits<T>::Tie;

        forEachTF<Tie>([&count](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                count += fieldCount<FieldType>();
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
    size_t findTransparentFieldIndex(T Struct::* fieldPtr)
    {
        Struct instance = {};

        size_t foundIndex = std::numeric_limits<size_t>::max();

        size_t count = 0;

        awl::for_each(instance.as_tuple(), [fieldPtr, &instance, &foundIndex, &count](auto& field)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                count += fieldCount<FieldType>();
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

    template <class Struct, class T>
    size_t findTransparentFieldIndexInStruct(T Struct::* fieldPtr)
    {
        Struct instance = {};

        size_t foundIndex = std::numeric_limits<size_t>::max();
        size_t count = 0;

        awl::for_each(instance.as_tuple(), [fieldPtr, &instance, &foundIndex, &count](auto& field)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (std::is_same_v<FieldType, T>)
            {
                if (&field == &(instance.*fieldPtr))
                {
                    foundIndex = count;
                }
            }

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                count += fieldCount<FieldType>();
            }
            else
            {
                ++count;
            }
        });

        assert(foundIndex != std::numeric_limits<size_t>::max());

        return foundIndex;
    }

    template <class Struct, class T>
    size_t transparentFieldPathIndex(T Struct::* fieldPtr)
    {
        return findTransparentFieldIndexInStruct(fieldPtr);
    }

    template <class Struct, class NestedStruct, class T, class... Tail>
    size_t transparentFieldPathIndex(NestedStruct Struct::* nested_field_ptr, T NestedStruct::* field_ptr, Tail... tail)
    {
        const size_t nested_index = findTransparentFieldIndexInStruct(nested_field_ptr);

        if constexpr (sizeof...(Tail) == 0)
        {
            return nested_index + findTransparentFieldIndexInStruct(field_ptr);
        }
        else
        {
            return nested_index + transparentFieldPathIndex(field_ptr, tail...);
        }
    }

    template <class... Ptrs>
    size_t findTransparentFieldIndex(std::tuple<Ptrs...> field_path)
    {
        return std::apply([](auto... ptrs)
        {
            return transparentFieldPathIndex(ptrs...);
        }, field_path);
    }

    template <class... Ptrs>
    size_t findTransparentFieldIndex(FieldPath<Ptrs...> field_path)
    {
        return findTransparentFieldIndex(field_path.ptrs);
    }

    template <class Path>
    void addTransparentFieldIndices(IndexFilter& indices, Path field_path)
    {
        const size_t start_index = findTransparentFieldIndex(field_path);

        using FieldType = typename FieldPathValue<Path>::type;

        if constexpr (awl::is_reflectable_v<FieldType>)
        {
            for (size_t index = start_index; index < start_index + fieldCount<FieldType>(); ++index)
            {
                indices.insert(index);
            }
        }
        else
        {
            indices.insert(start_index);
        }
    }

    template <class... Ptrs>
    IndexFilter findTransparentFieldIndices(FieldPath<Ptrs...> field_path)
    {
        IndexFilter indices;
        addTransparentFieldIndices(indices, field_path);
        return indices;
    }

    template <class... Paths> requires (IsFieldPathV<Paths> && ...)
    IndexFilter findTransparentFieldIndices(std::tuple<Paths...> field_paths)
    {
        IndexFilter indices;

        awl::for_each(field_paths, [&indices](auto field_path)
        {
            addTransparentFieldIndices(indices, field_path);
        });

        return indices;
    }

    template <class Value, class... Field>
    IndexFilter findTransparentFieldIndices(std::tuple<Field Value::*...> field_ptrs)
    {
        IndexFilter indices;

        auto add_indices = [&indices]<class Struct, class T>(T Struct::* fieldPtr)
        {
            Struct instance = {};

            size_t count = 0;
            bool found = false;

            awl::for_each(instance.as_tuple(), [fieldPtr, &instance, &indices, &count, &found](auto& field)
            {
                using FieldType = std::remove_reference_t<decltype(field)>;

                if constexpr (std::is_same_v<FieldType, T>)
                {
                    if (&field == &(instance.*fieldPtr))
                    {
                        found = true;

                        if constexpr (awl::is_reflectable_v<FieldType>)
                        {
                            for (size_t index = count; index < count + fieldCount<FieldType>(); ++index)
                            {
                                indices.insert(index);
                            }
                        }
                        else
                        {
                            indices.insert(count);
                        }
                    }
                }

                if constexpr (awl::is_reflectable_v<FieldType>)
                {
                    count += fieldCount<FieldType>();
                }
                else
                {
                    ++count;
                }
            });

            assert(found);
        };

        awl::for_each(field_ptrs, add_indices);

        return indices;
    }

    /*
    template <class T, class Tuple, std::size_t level_index>
    constexpr inline size_t findFieldIndexImpl(Tuple t)
    {
        size_t count = 0;

        using Tie = typename awl::tuplizable_traits<T>::Tie;

        forEachTF<Tie>([&count](auto fieldIndex)
        {
            using FieldType = std::remove_reference_t<std::tuple_element_t<fieldIndex, Tie>>;

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                return findFieldIndex<FieldType, Tuple, level_index>(t);
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
    constexpr size_t forEachFieldValueImpl(T& val, Func&& func, size_t start_index)
    {
        size_t count = 0;

        awl::for_each(val.as_tuple(), [start_index, &count, &func](auto& field)
        {
            using FieldType = std::remove_reference_t<decltype(field)>;

            if constexpr (awl::is_reflectable_v<FieldType>)
            {
                count += forEachFieldValueImpl(field, func, start_index + count);
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
    constexpr void forEachFieldValue(T& val, Func&& func)
    {
        forEachFieldValueImpl(val, func, 0);
    }

    inline std::string makeFullFieldName(std::vector<std::string_view>& prefixes, std::string_view name)
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

    template <class Struct, class ColumnVisitor>
    void forEachColumn(ColumnVisitor& visitor)
    {
        //memberNames capture parameter makes lambdas different.
        forEachFieldType<Struct>([&visitor](std::vector<std::string_view>& prefixes, size_t memberIndex, size_t fieldIndex, auto structTd, auto fieldTd)
            {
                if (visitor.containsColumn(fieldIndex))
                {
                    using StructType = typename decltype(structTd)::Type;

                    using FieldType = typename decltype(fieldTd)::Type;

                    const auto& member_names = StructType::member_names();

                    const std::string& member_name = member_names[memberIndex];

                    if (prefixes.empty() && awl::CStringInsensitiveEqual<char>()(member_name.c_str(), rowIdFieldName))
                    {
                        throw SQLiteException(0, "A field with name ROWID is not allowed.");
                    }

                    std::string full_name = helpers::makeFullFieldName(prefixes, member_name);

                    visitor.template addColumn<FieldType>(full_name, fieldIndex);
                }
            });
    }
}

