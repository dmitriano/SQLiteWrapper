#pragma once

#include "SQLiteWrapper/Helpers.h"
#include "SQLiteWrapper/Types.h"

#include <sstream>

namespace sqlite
{
    template <class Struct>
    class TableBuilder
    {
    public:

        enum class Collation
        {
            None,
            Binary,
            RTrim,
            NoCase
        };

        TableBuilder(const std::string & name, bool ifNotExists = false)
        {
            _out << "CREATE TABLE ";

            if (ifNotExists)
            {
                _out << "IF NOT EXISTS ";
            }

            _out << name << std::endl;

            _out << "(" << std::endl;
        }

        template <typename... Keys>
        void setPrimaryKey(Keys Struct::*...fieldPtr)
        {
            (_primaryKeyColumns.push_back(helpers::findFieldIndex(fieldPtr)), ...);
        }
        
        template <typename... Keys>
        void setPrimaryKeyTuple(std::tuple<Keys Struct::*...> id_ptrs)
        {
            awl::for_each(id_ptrs, [this](auto& id_ptr)
            {
                _primaryKeyColumns.push_back(helpers::findFieldIndex(id_ptr));
            });
        }

        template <class T>
        void setColumnConstraint(T Struct::*fieldPtr, const std::string & constraint)
        {
            _columnConstraints[helpers::findTransparentFieldIndex(fieldPtr)] = constraint;
        }
        
        void addColumns()
        {
            ColumnVisitor visitor(*this);
            
            helpers::forEachColumn<Struct>(visitor);
        }

        void addLine(const std::string & line)
        {
            addLineSeparator();

            _out << "  " << line;
        }
        
        template <class FieldType>
        void addColumn(const std::string & name, const std::string& constraint)
        {
            addLineSeparator();

            _out << "  " << name;

            using DataType = helpers::RemoveOptionalT<FieldType>;
            
            constexpr bool is_text = is_text_type_v<DataType>;

            constexpr bool is_blob = std::is_same_v<DataType, std::vector<uint8_t>>;

            if constexpr (is_text)
            {
                _out << " TEXT";
            }
            else if constexpr (std::is_integral_v<DataType>)
            {
                _out << " INTEGER";
            }
            else if constexpr (std::is_floating_point_v<DataType>)
            {
                _out << " REAL";
            }
            else if constexpr (is_blob)
            {
                _out << " BLOB";
            }

            //Allow zero-length blob by default.
            if constexpr (!is_blob)
            {
                if (constraint.empty())
                {
                    if constexpr (!helpers::IsOptionalV<FieldType>)
                    {
                        _out << " NOT NULL";
                    }
                }
                else
                {
                    _out << " " << constraint;
                }
            }
            else
            {
                _out << " " << constraint;
            }

            if constexpr (is_text)
            {
                switch (_defaultCollation)
                {
                case Collation::None:
                    break;
                case Collation::Binary:
                    _out << " COLLATE BINARY";
                    break;
                case Collation::NoCase:
                    _out << " COLLATE NOCASE";
                    break;
                case Collation::RTrim:
                    _out << " COLLATE RTRIM";
                    break;
                }
            }
        }
        
        std::string build()
        {
            //Remove rowId from primary key.
            _primaryKeyColumns.erase(
                std::remove_if(_primaryKeyColumns.begin(), _primaryKeyColumns.end(), [](size_t field_index)
                {
                    const auto& memberNames = Struct::member_names();

                    const std::string& name = memberNames[field_index];

                    return name == rowIdFieldName;
                }),
                _primaryKeyColumns.end());
            
            if (!_primaryKeyColumns.empty())
            {
                _out << "," << std::endl;

                _out << "  PRIMARY KEY(";

                bool firstPK = true;

                for (const size_t fieldIndex : _primaryKeyColumns)
                {
                    if (firstPK)
                    {
                        firstPK = false;
                    }
                    else
                    {
                        _out << " ,";
                    }

                    const auto& memberNames = Struct::member_names();

                    const std::string& name = memberNames[fieldIndex];

                    _out << name;
                }

                _out << ")";
            }

            if (!_foreignKeyClause.empty())
            {
                _out << "," << std::endl;

                _out << "  " << _foreignKeyClause;
            }

            _out << std::endl << ")";

            //if (_rowIdIndex == noIndex)
            //{
            //    _out << " WITHOUT ROWID";
            //}

            _out << ";" << std::endl;

            return _out.str();
        }

        std::string create()
        {
            addColumns();
            
            return build();
        }

    private:

        class ColumnVisitor
        {
        public:

            ColumnVisitor(TableBuilder& builder) : _builder(builder) {}

            bool containsColumn(size_t) const
            {
                return true;
            }

            template <class FieldType>
            void addColumn(const std::string& full_name, size_t field_index)
            {
                const std::string& constraint = _builder._columnConstraints[field_index];

                _builder.addColumn<FieldType>(full_name, constraint);
            }

        private:

            TableBuilder& _builder;
        };

        friend ColumnVisitor;

        void addLineSeparator()
        {
            if (_firstLine)
            {
                _firstLine = false;
            }
            else
            {
                _out << "," << std::endl;
            }
        }

        // size_t _rowIdIndex = noIndex;

        Collation _defaultCollation = Collation::NoCase;

        std::string _foreignKeyClause;

        std::vector<size_t> _primaryKeyColumns;
        
        std::array<std::string, helpers::fieldCount<Struct>()> _columnConstraints;

        std::ostringstream _out;

        bool _firstLine = true;
    };
}
