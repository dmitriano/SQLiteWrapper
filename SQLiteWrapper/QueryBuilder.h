#pragma once

#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/FieldListBuilder.h"

#include "Awl/Separator.h"
#include "Awl/StringFormat.h"
#include "Awl/BitMap.h"

namespace sqlite
{
    //TODO: It can build a vector of transparent filed names in the constructor.
    template <class Struct>
    class QueryBuilder
    {
    public:

        void StartSelect(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            FieldListBuilder<Struct> builder = MakeFieldBuilder();

            builder.p_filter = &filter;

            StartSelect(table_name, builder);
        }

        void StartSelect(const std::string& table_name, FieldListBuilder<Struct>& builder)
        {
            m_out << "SELECT ";

            AddFieldNames(builder);

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

            AddFieldNames(filter, { FieldOption::Parametized });
        }

        void StartDelete(const std::string& table_name)
        {
            m_out << "DELETE FROM " << table_name;
        }

        void CreateView(const std::string& view_name)
        {
            m_out << "CREATE VIEW " << view_name << " AS ";
        }

        FieldListBuilder<Struct> MakeFieldBuilder(awl::aseparator sep = MakeCommaSeparator())
        {
            return FieldListBuilder<Struct>(m_out, std::move(sep));
        }

        //We need to convert some tuple of field pointers to std::set<size_t>.
        //So it currently works fine for trivial insert and select,
        //but for trivial updates we need the filter with transparent indices.
        void AddFieldNames(const OptionalIndexFilter& filter, awl::bitmap<FieldOption> options = {}, awl::aseparator sep = MakeCommaSeparator())
        {
            FieldListBuilder<Struct> builder = MakeFieldBuilder(std::move(sep));

            builder.p_filter = &filter;
            builder.options = std::move(options);

            AddFieldNames(builder);
        }

        void AddFieldNames(FieldListBuilder<Struct>& builder)
        {
            helpers::ForEachFieldType<Struct>(builder);
        }

        void AddText(const std::string_view& text)
        {
            m_out << text;
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
                for (size_t i : *filter)
                {
                    add(i);
                }
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

        void AddWhereParam(size_t index)
        {
            m_out << "=?" << index + 1;
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

        void AddJoinOn(const std::string& right_table)
        {
            m_out << " JOIN " << right_table << " ON ";
        }

        void AddLeftJoinOn(const std::string& right_table)
        {
            m_out << " LEFT JOIN " << right_table << " ON ";
        }

        void AddJoinCondition(const std::string& left_table, const std::string& right_table,
            const std::string& left_id, const std::string& right_id)
        {
            m_out << left_table << "." << left_id << "=" << right_table << "." << right_id;
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

            builder.AddFieldNames(where_fields, { FieldOption::Parametized }, builder.MakeAndSeparator());
        }

        builder.AddTerminator();

        return builder.str();
    }

    template <class LeftStruct, class RightStruct, class T>
    std::string BuildListJoinQuery(const std::string& left_table_name, const std::string& right_table_name,
        T LeftStruct::* left_id_ptr, T RightStruct::* right_id_ptr,
        const OptionalIndexFilter& right_filter = {}, T LeftStruct::* where_id_ptr = nullptr)
    {
        QueryBuilder<RightStruct> builder;

        // disambiguate column names
        {
            FieldListBuilder<RightStruct> field_builder = builder.MakeFieldBuilder();

            field_builder.p_filter = &right_filter;
            field_builder.table_name = right_table_name;

            builder.StartSelect(left_table_name, field_builder);
        }

        builder.AddLeftJoinOn(right_table_name);

        // We do not need transparent indices to get names.
        // Key ids can't be in nested structures (this is not supported yet).
        builder.AddJoinCondition(left_table_name, right_table_name,
            helpers::FindFieldName(left_id_ptr), helpers::FindFieldName(right_id_ptr));

        if (where_id_ptr != nullptr)
        {
            builder.AddWhere();

            builder.AddText(left_table_name);
            
            builder.AddText(".");

            builder.AddText(helpers::FindFieldName(where_id_ptr));

            builder.AddWhereParam(0);
        }

        builder.AddTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string BuildParameterizedInsertQuery(const std::string& table_name, const OptionalIndexFilter& filter = {})
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

            builder.AddFieldNames(where_fields, { FieldOption::Parametized }, builder.MakeAndSeparator());
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

        builder.AddFieldNames(where_fields, { FieldOption::Parametized }, builder.MakeAndSeparator());

        builder.AddTerminator();

        return builder.str();
    }
}
