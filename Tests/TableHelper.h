#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableInstantiator.h"
#include "SQLiteWrapper/AutoincrementTableInstantiator.h"
#include "SQLiteWrapper/AutoincrementSet.h"
#include "SQLiteWrapper/Set.h"

namespace swtest
{
    template <class Value, class... Keys>
    Set<Value, Keys...> MakeSet(const std::shared_ptr<Database>& db, const std::string& table_name, std::tuple<Keys Value::*...> id_ptrs)
    {
        using namespace sqlite;

        TableInstantiator<Value, Keys...> instantiator(db, table_name, id_ptrs);

        instantiator.Create();

        return Set<Value, Keys...>(db, table_name, id_ptrs);
    }

    template <class Value, class Int> requires std::is_integral_v<Int>
    AutoincrementSet<Value, Int> MakeAutoincrementSet(const std::shared_ptr<Database>& db, const std::string& table_name, Int Value::* id_ptr)
    {
        using namespace sqlite;

        AutoincrementTableInstantiator<Value, Int> instantiator(db, table_name, id_ptr);

        instantiator.Create();

        return AutoincrementSet<Value, Int>(db, table_name, id_ptr);
    }
}
