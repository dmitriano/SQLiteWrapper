#pragma once

#include "SQLiteWrapper/Helpers.h"

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
            m_out << "CREATE TABLE ";

            if (ifNotExists)
            {
                m_out << "IF NOT EXISTS ";
            }

            m_out << name << std::endl;

            m_out << "(" << std::endl;
        }

        template <typename... Keys>
        void setPrimaryKey(Keys Struct::*...fieldPtr)
        {
            (m_primaryKeyColumns.push_back(helpers::findFieldIndex(fieldPtr)), ...);
        }
        
        template <typename... Keys>
        void setPrimaryKeyTuple(std::tuple<Keys Struct::*...> id_ptrs)
        {
            awl::for_each(id_ptrs, [this](auto& id_ptr)
            {
                m_primaryKeyColumns.push_back(helpers::findFieldIndex(id_ptr));
            });
        }

        template <class T>
        void setColumnConstraint(T Struct::*fieldPtr, const std::string & constraint)
        {
            m_columnConstraints[helpers::findTransparentFieldIndex(fieldPtr)] = constraint;
        }
        
        void addColumns()
        {
            ColumnVisitor visitor(*this);
            
            helpers::forEachColumn<Struct>(visitor);
        }

        void addLine(const std::string & line)
        {
            addLineSeparator();

            m_out << "  " << line;
        }
        
        template <class FieldType>
        void addColumn(const std::string & name, const std::string& constraint)
        {
            addLineSeparator();

            m_out << "  " << name;

            using DataType = helpers::RemoveOptionalT<FieldType>;
            
            constexpr bool is_text = std::is_same_v<std::string, DataType>;

            constexpr bool is_blob = std::is_same_v<DataType, std::vector<uint8_t>>;

            if constexpr (is_text)
            {
                m_out << " TEXT";
            }
            else if constexpr (std::is_integral_v<DataType>)
            {
                m_out << " INTEGER";
            }
            else if constexpr (std::is_floating_point_v<DataType>)
            {
                m_out << " REAL";
            }
            else if constexpr (is_blob)
            {
                m_out << " BLOB";
            }

            //Allow zero-length blob by default.
            if constexpr (!is_blob)
            {
                if (constraint.empty())
                {
                    if constexpr (!helpers::IsOptionalV<FieldType>)
                    {
                        m_out << " NOT NULL";
                    }
                }
                else
                {
                    m_out << " " << constraint;
                }
            }
            else
            {
                m_out << " " << constraint;
            }

            if constexpr (is_text)
            {
                switch (m_defaultCollation)
                {
                case Collation::None:
                    break;
                case Collation::Binary:
                    m_out << " COLLATE BINARY";
                    break;
                case Collation::NoCase:
                    m_out << " COLLATE NOCASE";
                    break;
                case Collation::RTrim:
                    m_out << " COLLATE RTRIM";
                    break;
                }
            }
        }
        
        std::string build()
        {
            //Remove rowId from primary key.
            m_primaryKeyColumns.erase(
                std::remove_if(m_primaryKeyColumns.begin(), m_primaryKeyColumns.end(), [](size_t field_index)
                {
                    const auto& memberNames = Struct::member_names();

                    const std::string& name = memberNames[field_index];

                    return name == rowIdFieldName;
                }),
                m_primaryKeyColumns.end());
            
            if (!m_primaryKeyColumns.empty())
            {
                m_out << "," << std::endl;

                m_out << "  PRIMARY KEY(";

                bool firstPK = true;

                for (const size_t fieldIndex : m_primaryKeyColumns)
                {
                    if (firstPK)
                    {
                        firstPK = false;
                    }
                    else
                    {
                        m_out << " ,";
                    }

                    const auto& memberNames = Struct::member_names();

                    const std::string& name = memberNames[fieldIndex];

                    m_out << name;
                }

                m_out << ")";
            }

            if (!m_foreignKeyClause.empty())
            {
                m_out << "," << std::endl;

                m_out << "  " << m_foreignKeyClause;
            }

            m_out << std::endl << ")";

            //if (m_rowIdIndex == noIndex)
            //{
            //    m_out << " WITHOUT ROWID";
            //}

            m_out << ";" << std::endl;

            return m_out.str();
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

            ColumnVisitor(TableBuilder& builder) : m_builder(builder) {}

            bool containsColumn(size_t) const
            {
                return true;
            }

            template <class FieldType>
            void addColumn(const std::string& full_name, size_t field_index)
            {
                const std::string& constraint = m_builder.m_columnConstraints[field_index];

                m_builder.addColumn<FieldType>(full_name, constraint);
            }

        private:

            TableBuilder& m_builder;
        };

        friend ColumnVisitor;

        void addLineSeparator()
        {
            if (m_firstLine)
            {
                m_firstLine = false;
            }
            else
            {
                m_out << "," << std::endl;
            }
        }

        // size_t m_rowIdIndex = noIndex;

        Collation m_defaultCollation = Collation::NoCase;

        std::string m_foreignKeyClause;

        std::vector<size_t> m_primaryKeyColumns;
        
        std::array<std::string, helpers::fieldCount<Struct>()> m_columnConstraints;

        std::ostringstream m_out;

        bool m_firstLine = true;
    };
}
