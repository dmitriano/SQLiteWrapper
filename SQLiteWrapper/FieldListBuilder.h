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

        void operator() (std::vector<std::string_view>& prefixes, size_t memberIndex, size_t fieldIndex, auto structTd, auto fieldId)
        {
            static_cast<void>(fieldId);

            if (!p_filter || !p_filter->has_value() || p_filter->value().contains(fieldIndex))
            {
                using StructType = typename decltype(structTd)::Type;

                const auto& member_names = StructType::get_member_names();

                const std::string& member_name = member_names[memberIndex];

                std::string full_name = helpers::MakeFullFieldName(prefixes, member_name);

                out() << m_sep;

                if (!table_name.empty())
                {
                    out() << table_name << ".";
                }

                out() << full_name;

                if (options[FieldOption::Parametized])
                {
                    out() << "=?" << static_cast<size_t>(fieldIndex + 1);
                }
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
