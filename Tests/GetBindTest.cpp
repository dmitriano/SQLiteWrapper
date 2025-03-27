#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

using namespace swtest;

namespace
{
    const char create_query[] = "CREATE TABLE bots (name TEXT COLLATE NOCASE, state BLOB);";
    const char insert_query[] = "INSERT INTO bots (name, state) VALUES (?1, ?2);";
    const char select_query[] = "SELECT name, state FROM bots";
}

// A test demonstrating that an empty string IS NOT NULL, but an empty BLOB IS NULL and NULL is not TEXT or BLOB.
AWL_TEST(GetBindNull)
{
    DbContainer c(context);

    c.m_db->Exec(create_query);

    {
        sqlite::Statement insert_statement(*c.m_db, insert_query);

        // Empty string is not Null.
        insert_statement.BindText(0, "");
        insert_statement.BindBlob(1, {});

        insert_statement.Exec();

        insert_statement.BindNull(0);
        insert_statement.BindNull(1);

        insert_statement.Exec();
    }

    {
        sqlite::Statement select_statement(*c.m_db, select_query);

        AWL_ASSERT(select_statement.Next());

        AWL_ASSERT(!select_statement.IsNull(0));
        AWL_ASSERT(select_statement.IsText(0));
        AWL_ASSERT(select_statement.IsNull(1));
        AWL_ASSERT(!select_statement.IsBlob(1));

        {
            std::string text;
            sqlite::Get(select_statement, 0, text);
            AWL_ASSERT(text.empty());
        }

        {
            std::vector<uint8_t> blob;
            sqlite::Get(select_statement, 1, blob);
            AWL_ASSERT(blob.empty());
        }

        AWL_ASSERT(select_statement.Next());

        AWL_ASSERT(select_statement.IsNull(0));
        AWL_ASSERT(!select_statement.IsText(0));
        AWL_ASSERT(select_statement.IsNull(1));
        AWL_ASSERT(!select_statement.IsBlob(1));

        {
            std::vector<uint8_t> blob;
            sqlite::Get(select_statement, 1, blob);
            AWL_ASSERT(blob.empty());
        }
    }
}

AWL_TEST(GetBindOptional)
{
    DbContainer c(context);

    c.m_db->Exec(create_query);

    const std::optional<std::string> sample_text = "a";
    const std::optional<std::string> null_text;

    {
        sqlite::Statement insert_statement(*c.m_db, insert_query);

        sqlite::Bind(insert_statement, 0, sample_text);

        insert_statement.Exec();

        sqlite::Bind(insert_statement, 0, null_text);

        insert_statement.Exec();
    }

    {
        sqlite::Statement select_statement(*c.m_db, select_query);

        AWL_ASSERT(select_statement.Next());

        {
            std::optional<std::string> text;
            sqlite::Get(select_statement, 0, text);
            AWL_ASSERT(text == sample_text);
        }

        AWL_ASSERT(select_statement.Next());

        {
            std::optional<std::string> text;
            sqlite::Get(select_statement, 0, text);
            AWL_ASSERT(text == null_text);
        }
    }
}
