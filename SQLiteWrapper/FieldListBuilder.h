#pragma once

#include "SQLiteWrapper/Helpers.h"

#include "Awl/Separator.h"
#include "Awl/LegacyFormat.h"
#include "Awl/BitMap.h"

#include <functional>

namespace sqlite
{
    AWL_SEQUENTIAL_ENUM(FieldOption, Parametized, SequentialBindingIndices)
}

AWL_ENUM_TRAITS(sqlite, FieldOption)

namespace sqlite
{
    inline awl::aseparator makeCommaSeparator()
    {
        return awl::aseparator(',');
    }

    inline awl::aseparator makeAndSeparator()
    {
        return awl::aseparator(" AND ");
    }

    //TODO: It can build a vector of transparent filed names in the constructor.
    template <class Struct>
    class FieldListBuilder
    {
    public:

        FieldListBuilder(std::ostringstream& out, awl::aseparator sep) :
            _out(std::ref(out)),
            _sep(std::move(sep))
        {}

        bool containsColumn(size_t field_index) const
        {
            return !filter || filter->contains(field_index);
        }

        template <class FieldType>
        void addColumn(const std::string& full_name, size_t field_index)
        {
            out() << _sep;

            if (!table_name.empty())
            {
                out() << table_name << ".";
            }

            out() << full_name;

            if (options[FieldOption::Parametized])
            {
                // Bind entire value or a tuple of ids.
                const size_t parameter_index = options[FieldOption::SequentialBindingIndices] && filter ? 
                    filter->index_of(field_index) : field_index;
                    
                out() << "=?" << static_cast<size_t>(parameter_index + 1);
            }
        }

        void setFilter(OptionalIndexFilter optional_filter)
        {
            filter = std::move(optional_filter);
        }

        std::string table_name;
        awl::bitmap<FieldOption> options;

    private:

        OptionalIndexFilter filter;

        std::ostringstream& out()
        {
            return _out;
        }

        std::reference_wrapper<std::ostringstream> _out;

        awl::aseparator _sep;
    };
}

