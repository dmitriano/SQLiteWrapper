#pragma once

#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/FieldListBuilder.h"

#include "Awl/Separator.h"
#include "Awl/BitMap.h"

#include <sstream>

namespace sqlite
{
    //TODO: It can build a vector of transparent filed names in the constructor.
    template <class Struct>
    class QueryBuilder
    {
    public:

        void Startselect(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            FieldListBuilder<Struct> builder = makeFieldBuilder();

            builder.setFilter(filter);

            Startselect(table_name, builder);
        }

        void Startselect(const std::string& table_name, FieldListBuilder<Struct>& builder)
        {
            _out << "SELECT ";

            addFieldNames(builder);

            _out << " FROM " << table_name;
        }

        void Startinsert(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            _out << "INSERT INTO " << table_name << " (";

            addFieldNames(filter);

            _out << ") VALUES";
        }

        void Startupdate(const std::string& table_name, const OptionalIndexFilter& filter = {})
        {
            _out << "UPDATE " << table_name << " SET ";

            addFieldNames(filter, { FieldOption::Parametized });
        }

        void StartdeleteElement(const std::string& table_name)
        {
            _out << "DELETE FROM " << table_name;
        }

        void createView(const std::string& view_name)
        {
            _out << "CREATE VIEW " << view_name << " AS ";
        }

        FieldListBuilder<Struct> makeFieldBuilder(awl::aseparator sep = makeCommaSeparator())
        {
            return FieldListBuilder<Struct>(_out, std::move(sep));
        }

        //We need to convert some tuple of field pointers to std::set<size_t>.
        //So it currently works fine for trivial insert and select,
        //but for trivial updates we need the filter with transparent indices.
        void addFieldNames(const OptionalIndexFilter& filter, awl::bitmap<FieldOption> options = {}, awl::aseparator sep = makeCommaSeparator())
        {
            FieldListBuilder<Struct> builder = makeFieldBuilder(std::move(sep));

            builder.setFilter(filter);
            builder.options = std::move(options);

            helpers::forEachColumn<Struct>(builder);
        }

        void addFieldNames(FieldListBuilder<Struct>& builder)
        {
            helpers::forEachColumn<Struct>(builder);
        }

        void addText(const std::string_view& text)
        {
            _out << text;
        }

        void addParameters(const OptionalIndexFilter& filter = {})
        {
            _out << " (";

            auto sep = makeCommaSeparator();

            auto add = [this, &sep](size_t i)
            {
                _out << sep << "?" << (i + 1);
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
                const size_t count = helpers::fieldCount<Struct>();

                for (size_t i = 0; i < count; ++i)
                {
                    add(i);
                }
            }

            _out << ")";
        }

        void addWhere()
        {
            addText(" WHERE ");
        }

        void addWhereParam(size_t index)
        {
            _out << "=?" << index + 1;
        }

        void addLimit(size_t n)
        {
            _out << " LIMIT " << n;
        }

        void addOffset(size_t n)
        {
            _out << " OFFSET " << n;
        }

        void addTerminator()
        {
            _out << ";";
        }

        void addJoinOn(const std::string& right_table)
        {
            _out << " JOIN " << right_table << " ON ";
        }

        void addLeftJoinOn(const std::string& right_table)
        {
            _out << " LEFT JOIN " << right_table << " ON ";
        }

        void addJoinCondition(const std::string& left_table, const std::string& right_table,
            const std::string& left_id, const std::string& right_id)
        {
            _out << left_table << "." << left_id << "=" << right_table << "." << right_id;
        }

        template <class T>
        QueryBuilder& operator << (const T& val)
        {
            _out << val;

            return *this;
        }

        std::string str() const
        {
            return _out.str();
        }

    private:

        std::ostringstream _out;
    };

    template <class Struct>
    std::string buildTrivialSelectQuery(const std::string& table_name)
    {
        QueryBuilder<Struct> builder;

        builder.Startselect(table_name);

        builder.addTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string buildParameterizedSelectQuery(const std::string& table_name, const OptionalIndexFilter& select_fields,
        const OptionalIndexFilter& where_fields = {}, bool sequential_binding_indices = false)
    {
        QueryBuilder<Struct> builder;

        builder.Startselect(table_name, select_fields);

        if (where_fields)
        {
            builder.addWhere();

            awl::bitmap<FieldOption> options = { FieldOption::Parametized };

            if (sequential_binding_indices)
            {
                options |= FieldOption::SequentialBindingIndices;
            }

            builder.addFieldNames(where_fields, options, makeAndSeparator());
        }

        builder.addTerminator();

        return builder.str();
    }

    template <class LeftStruct, class RightStruct, class T>
    std::string buildListJoinQuery(const std::string& left_table_name, const std::string& right_table_name,
        T LeftStruct::* left_id_ptr, T RightStruct::* right_id_ptr,
        const OptionalIndexFilter& right_filter = {}, T LeftStruct::* where_id_ptr = nullptr)
    {
        QueryBuilder<RightStruct> builder;

        // disambiguate column names
        {
            FieldListBuilder<RightStruct> field_builder = builder.makeFieldBuilder();

            field_builder.setFilter(right_filter);
            field_builder.table_name = right_table_name;

            builder.Startselect(left_table_name, field_builder);
        }

        builder.addLeftJoinOn(right_table_name);

        // We do not need transparent indices to get names.
        // Key ids can't be in nested structures (this is not supported yet).
        builder.addJoinCondition(left_table_name, right_table_name,
            helpers::findFieldName(left_id_ptr), helpers::findFieldName(right_id_ptr));

        if (where_id_ptr != nullptr)
        {
            builder.addWhere();

            builder.addText(left_table_name);
            
            builder.addText(".");

            builder.addText(helpers::findFieldName(where_id_ptr));

            builder.addWhereParam(0);
        }

        builder.addTerminator();

        return builder.str();
    }

    template <class Struct, class T>
    std::string buildListWhereQuery(const std::string& table_name, T Struct::* where_id_ptr)
    {
        return sqlite::buildParameterizedSelectQuery<Struct>(table_name, {},
            helpers::findTransparentFieldIndices(std::make_tuple(where_id_ptr)), true);
    }
        
    template <class Struct>
    std::string buildParameterizedInsertQuery(const std::string& table_name, const OptionalIndexFilter& filter = {})
    {
        QueryBuilder<Struct> builder;

        builder.Startinsert(table_name, filter);
        
        builder.addParameters(filter);

        builder.addTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string buildParameterizedUpdateQuery(const std::string& table_name, const IndexFilter& set_fields, const OptionalIndexFilter& where_fields)
    {
        QueryBuilder<Struct> builder;

        builder.Startupdate(table_name, set_fields);

        if (where_fields)
        {
            builder.addWhere();

            builder.addFieldNames(where_fields, { FieldOption::Parametized }, makeAndSeparator());
        }

        builder.addTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string buildParameterizedDeleteQuery(const std::string& table_name, const IndexFilter& where_fields)
    {
        QueryBuilder<Struct> builder;

        builder.StartdeleteElement(table_name);

        builder.addWhere();

        builder.addFieldNames(where_fields, { FieldOption::Parametized }, makeAndSeparator());

        builder.addTerminator();

        return builder.str();
    }

    template <class Struct>
    std::string buildTrivialDeleteQuery(const std::string& table_name)
    {
        QueryBuilder<Struct> builder;

        builder.StartdeleteElement(table_name);

        builder.addTerminator();

        return builder.str();
    }
}

