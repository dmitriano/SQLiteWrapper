#pragma once

#include "SQLiteWrapper/Helpers.h"

#include "Awl/Separator.h"
#include "Awl/StringFormat.h"

namespace sqlite
{
    //TODO: It can build a vector of transparent filed names in the constructor.
    template <class Struct>
    class QueryBuilder
    {
    public:

        void StartSelect(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            m_out << "SELECT ";

            AddFieldNames(filter);

            m_out << " FROM " << table_name;
        }

        void StartInsert(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            m_out << "INSERT INTO " << table_name << " (";

            AddFieldNames(filter);

            m_out << ") VALUES";
        }

        void StartUpdate(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            m_out << "UPDATE " << table_name << " SET ";

            AddFieldNames(filter, true);
        }

        void StartDelete(const std::string& table_name)
        {
            m_out << "DELETE FROM " << table_name;
        }

        void CreateView(const std::string& view_name)
        {
            m_out << "CREATE VIEW " << view_name << " AS ";
        }

        //We need to convert some tuple of field pointers to std::set<size_t>.
        //So it currently works fine for trivial insert and select,
        //but for trivial updates we need the filter with transparent indices.
        void AddFieldNames(const OptionalIndexFilter& filter, bool parametized = false, awl::aseparator sep = MakeCommaSeparator())
        {
            helpers::ForEachFieldType<Struct>([this, &filter, parametized, &sep](std::vector<std::string_view>& prefixes, size_t memberIndex, size_t fieldIndex, auto structTd, auto fieldId)
            {
                static_cast<void>(fieldId);

                if (!filter || filter->find(fieldIndex) != filter->end())
                {
                    using StructType = typename decltype(structTd)::Type;

                    const auto& member_names = StructType::get_member_names();

                    const std::string& member_name = member_names[memberIndex];

                    std::string full_name = helpers::MakeFullFieldName(prefixes, member_name);

                    m_out << sep << full_name;

                    if (parametized)
                    {
                        m_out << "=?" << static_cast<size_t>(fieldIndex + 1);
                    }
                }
            });
        }

        void AddText(const std::string_view& text)
        {
            m_out << text;
        }

        void AddFilteredParameters(const IndexFilter& filter, awl::aseparator sep = MakeCommaSeparator())
        {
        }

        void AddParameters(const OptionalIndexFilter& filter = {})
        {
            m_out << " (";

            auto sep = MakeCommaSeparator();

            auto add = [this, &sep](size_t i)
            {
                m_out << sep << "?" << (i + 1);
            };

            if (filter)
            {
                // We can't simply interate over filter because it is std::unordered_set.
                helpers::ForEachFieldType<Struct>([this, &f = *filter, &add](std::vector<std::string_view>& prefixes, size_t memberIndex, size_t fieldIndex, auto structTd, auto fieldId)
                {
                    static_cast<void>(prefixes);
                    static_cast<void>(fieldId);
                    static_cast<void>(structTd);
                    static_cast<void>(memberIndex);

                    if (f.find(fieldIndex) != f.end())
                    {
                        add(fieldIndex);
                    }
                });
            }
            else
            {
                const size_t count = helpers::GetFieldCount<Struct>();

                for (size_t i = 0; i < count; ++i)
                {
                    add(i);
                }
            }

            m_out << ")";
        }

        void AddWhere()
        {
            AddText(" WHERE ");
        }

        void AddLimit(size_t n)
        {
            m_out << " LIMIT " << n;
        }

        void AddOffset(size_t n)
        {
            m_out << " OFFSET " << n;
        }

        void AddTerminator()
        {
            m_out << ";";
        }

        template <class T>
        QueryBuilder& operator << (const T& val)
        {
            m_out << val;

            return *this;
        }

        QueryBuilder& operator << (const awl::aformat & f)
        {
            m_out << f.str();

            return *this;
        }
        
        std::string str() const
        {
            return m_out.str();
        }

        static awl::aseparator MakeCommaSeparator()
        {
            return awl::aseparator(',');
        }

        static awl::aseparator MakeAndSeparator()
        {
            return awl::aseparator(" AND ");
        }

    private:

        std::ostringstream m_out;
    };

    template <class Struct>
    std::string BuildTrivialSelectQuery(const std::string& table_name)
    {
        QueryBuilder<Struct> builder;

        builder.StartSelect(table_name);

        builder.AddTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string BuildParameterizedSelectQuery(const std::string& table_name, const OptionalIndexFilter& select_fields, const OptionalIndexFilter& where_fields = {})
    {
        QueryBuilder<Struct> builder;

        builder.StartSelect(table_name, select_fields);

        if (where_fields)
        {
            builder.AddWhere();

            builder.AddFieldNames(where_fields, true, builder.MakeAndSeparator());
        }

        builder.AddTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string BuildParameterizedInsertQuery(const std::string & table_name, const OptionalIndexFilter& filter = {})
    {
        QueryBuilder<Struct> builder;

        builder.StartInsert(table_name, filter);
        
        builder.AddParameters(filter);

        builder.AddTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string BuildParameterizedUpdateQuery(const std::string& table_name, const IndexFilter& set_fields, const OptionalIndexFilter& where_fields)
    {
        QueryBuilder<Struct> builder;

        builder.StartUpdate(table_name, set_fields);

        if (where_fields)
        {
            builder.AddWhere();

            builder.AddFieldNames(where_fields, true, builder.MakeAndSeparator());
        }

        builder.AddTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string BuildParameterizedDeleteQuery(const std::string& table_name, const IndexFilter& where_fields)
    {
        QueryBuilder<Struct> builder;

        builder.StartDelete(table_name);

        builder.AddWhere();

        builder.AddFieldNames(where_fields, true, builder.MakeAndSeparator());

        builder.AddTerminator();

        return builder.str();
    }
}
