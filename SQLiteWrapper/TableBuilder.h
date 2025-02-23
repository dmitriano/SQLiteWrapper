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
        void SetPrimaryKey(Keys Struct::*...fieldPtr)
        {
            (m_primaryKeyColumns.push_back(helpers::FindFieldIndex(fieldPtr)), ...);
        }
        
        template <typename... Keys>
        void SetPrimaryKeyTuple(std::tuple<Keys Struct::*...> id_ptrs)
        {
            awl::for_each(id_ptrs, [this](auto& id_ptr)
            {
                m_primaryKeyColumns.push_back(helpers::FindFieldIndex(id_ptr));
            });
        }

        template <class T>
        void SetColumnConstraint(T Struct::*fieldPtr, const std::string & constraint)
        {
            m_columnConstraints[helpers::FindTransparentFieldIndex(fieldPtr)] = constraint;
        }
        
        void AddColumns()
        {
            //memberNames capture parameter makes lambdas different.
            helpers::ForEachFieldType<Struct>([this](std::vector<std::string_view>& prefixes, size_t memberIndex, size_t fieldIndex, auto structTd, auto fieldTd)
            {
                using StructType = typename decltype(structTd)::Type;
                
                using FieldType = typename decltype(fieldTd)::Type;

                const auto & member_names = StructType::get_member_names();

                const std::string& member_name = member_names[memberIndex];

                // Do not create a column with name 'ROWID'.
                if (!(prefixes.empty() && awl::CStringInsensitiveEqual<char>()(member_name.c_str(), rowIdFieldName)))
                {
                    std::string full_name = helpers::MakeFullFieldName(prefixes, member_name);

                    const std::string& constraint = m_columnConstraints[fieldIndex];

                    AddColumn<FieldType>(full_name, constraint);
                }
                //else
                //{
                //    m_rowIdIndex = fieldIndex;
                //}
            });
        }

        void AddLine(const std::string & line)
        {
            AddLineSearator();

            m_out << "  " << line;
        }
        
        template <class FieldType>
        void AddColumn(const std::string & name, const std::string & constraint)
        {
            AddLineSearator();

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
        
        std::string Build()
        {
            //Remove rowId from primary key.
            m_primaryKeyColumns.erase(
                std::remove_if(m_primaryKeyColumns.begin(), m_primaryKeyColumns.end(), [](size_t field_index)
                {
                    const auto& memberNames = Struct::get_member_names();

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

                    const auto& memberNames = Struct::get_member_names();

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

        std::string Create()
        {
            AddColumns();
            
            return Build();
        }

    private:

        void AddLineSearator()
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
        
        std::array<std::string, helpers::GetFieldCount<Struct>()> m_columnConstraints;

        std::ostringstream m_out;

        bool m_firstLine = true;
    };
}
