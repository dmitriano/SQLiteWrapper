#pragma once

#include "Awl/Exception.h"
#include "Awl/StringFormat.h"

#include <typeinfo>

namespace sqlite
{
    class SQLiteException : public awl::Exception
    {
    public:
        
        explicit SQLiteException(int code, std::string message) : m_code(code), m_Message(message)
        {
        }

        const char* what() const throw() override
        {
            return m_Message.c_str();
        }

        awl::String What() const override
        {
            return (awl::format() << _T("SQlite error code: ") << m_code << _T(", ") << awl::FromAString(m_Message));
        }

        int GetCode() const
        {
            return m_code;
        }

    private:

        const int m_code;

        const std::string m_Message;
    };
}
