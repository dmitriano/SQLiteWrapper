#pragma once

#include "SQLiteWrapper/Helpers.h"

#include "Awl/Separator.h"
#include "Awl/StringFormat.h"
#include "Awl/BitMap.h"

#include <functional>

namespace sqlite
{
    AWL_SEQUENTIAL_ENUM(FieldOption, Parametized)
}

AWL_ENUM_TRAITS(sqlite, FieldOption)

namespace sqlite
{
    //TODO: It can build a vector of transparent filed names in the constructor.
    template <class Struct>
    class FieldListBuilder
    {
    public:

        FieldListBuilder(std::ostringstream& out, awl::aseparator sep) :
            m_out(std::ref(out)),
            m_sep(std::move(sep))
        {}

        bool ContainsColumn(size_t field_index) const
        {
            return !p_filter || !p_filter->has_value() || p_filter->value().contains(field_index);
        }

        template <class FieldType>
        void AddColumn(const std::string& full_name, size_t field_index)
        {
            out() << m_sep;

            if (!table_name.empty())
            {
                out() << table_name << ".";
            }

            out() << full_name;

            if (options[FieldOption::Parametized])
            {
                out() << "=?" << static_cast<size_t>(field_index + 1);
            }
        }

        std::string table_name;
        const OptionalIndexFilter* p_filter;
        awl::bitmap<FieldOption> options;

    private:

        std::ostringstream& out()
        {
            return m_out;
        }

        std::reference_wrapper<std::ostringstream> m_out;

        awl::aseparator m_sep;
    };
}
