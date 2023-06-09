#include "DbContainer.h"
#include "SQLiteWrapper/Bind.h"
#include "SQLiteWrapper/Get.h"

using namespace swtest;

// A test demonstrating that an empty string IS NOT NULL, but an empty BLOB IS NULL and NULL is not TEXT or BLOB.
AWT_TEST(GetBindNull)
{
    DbContainer c(context);

    c.m_db->Exec("CREATE TABLE bots (name TEXT COLLATE NOCASE, state BLOB);");

    const char insert_query[] = "INSERT INTO bots (name, state) VALUES (?1, ?2);";
    const char select_query[] = "SELECT name, state FROM bots";
    
    {
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

            AWT_ASSERT(select_statement.Next());

            AWT_ASSERT(!select_statement.IsNull(0));
            AWT_ASSERT(select_statement.IsText(0));
            AWT_ASSERT(select_statement.IsNull(1));
            AWT_ASSERT(!select_statement.IsBlob(1));

            {
                std::string text;
                sqlite::Get(select_statement, 0, text);
                AWT_ASSERT(text.empty());
            }

            {
                std::vector<uint8_t> blob;
                sqlite::Get(select_statement, 1, blob);
                AWT_ASSERT(blob.empty());
            }

            AWT_ASSERT(select_statement.Next());

            AWT_ASSERT(select_statement.IsNull(0));
            AWT_ASSERT(!select_statement.IsText(0));
            AWT_ASSERT(select_statement.IsNull(1));
            AWT_ASSERT(!select_statement.IsBlob(1));

            {
                std::vector<uint8_t> blob;
                sqlite::Get(select_statement, 1, blob);
                AWT_ASSERT(blob.empty());
            }
        }
    }
}
