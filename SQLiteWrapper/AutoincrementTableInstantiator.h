#pragma once

#include "SQLiteWrapper/Database.h"
#include "SQLiteWrapper/TableBuilder.h"
#include "SQLiteWrapper/Element.h"
#include "SQLiteWrapper/AutoincrementSet.h"

#include "Awl/LegacyFormat.h"
#include "Awl/Observer.h"

#include <memory>
#include <functional>
#include <cassert>
#include <tuple>
#include <type_traits>

namespace sqlite
{
    namespace detail
    {
        template <class Ptr>
        struct MemberPointerTraits;

        template <class Class, class T>
        struct MemberPointerTraits<T Class::*>
        {
            using class_type = Class;
            using value_type = T;
        };

        template <class IdPath>
        struct IdPathTraits : MemberPointerTraits<IdPath>
        {
        };

        template <class... Ptrs>
        struct IdPathTraits<std::tuple<Ptrs...>>
        {
            using first_pointer = std::tuple_element_t<0, std::tuple<Ptrs...>>;
            using last_pointer = std::tuple_element_t<sizeof...(Ptrs) - 1, std::tuple<Ptrs...>>;

            using class_type = typename MemberPointerTraits<first_pointer>::class_type;
            using value_type = typename MemberPointerTraits<last_pointer>::value_type;
        };

        template <class IdPath>
        using IdPathClass = typename IdPathTraits<IdPath>::class_type;

        template <class IdPath>
        using IdPathValue = typename IdPathTraits<IdPath>::value_type;
    }

    template <class Value, class Int, class IdPath = Int Value::*> requires std::is_integral_v<Int>
    class AutoincrementTableInstantiator : public awl::Observer<Element>
    {
    private:

        using Record = Value;

    public:

        AutoincrementTableInstantiator(const std::shared_ptr<Database>& db, std::string table_name, IdPath id_path,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
        :
            m_db(db),
            tableName(std::move(table_name)),
            idPath(std::move(id_path)),
            addConstraints(std::move(add_constraints))
        {
            m_db->subscribe(this);
        }

        AutoincrementTableInstantiator(std::string table_name, IdPath id_path,
            std::function<void(TableBuilder<Value>&)> add_constraints = {})
        :
            tableName(std::move(table_name)),
            idPath(std::move(id_path)),
            addConstraints(std::move(add_constraints))
        {
        }

        void create(DatabaseRef db_ref) override
        {
            Database& db = db_ref.get();

            if (!db.tableExists(tableName))
            {
                TableBuilder<Record> builder(tableName);

                // The ROWID chosen for the new row is at least one larger than the largest ROWID that has ever before existed in that same table.
                // For this to apply, we need to explicilty use AUTOINCREMENT keyword:
                builder.setColumnConstraint(idPath, "NOT NULL PRIMARY KEY AUTOINCREMENT");

                if (addConstraints)
                {
                    // Adds constraints like REFERENCES, NULL, UNIQUE, etc...
                    addConstraints(builder);
                }

                const std::string query = builder.create();

                db.logger().debug(awl::format() << "Creating table '" << tableName << "': \n" << query);

                db.exec(query);

                db.invalidateScheme();
            }
            else
            {
                db.logger().debug(awl::format() << "Table '" << tableName << "' already exists.");
            }
        }

        void deleteElement(DatabaseRef db_ref) override
        {
            Database& db = db_ref.get();

            db.dropTable(tableName);
        }

        using SetType = AutoincrementSet<Value, Int>;

        SetType makeSet() const requires std::is_member_object_pointer_v<IdPath>
        {
            assert(m_db != nullptr);

            return makeSet(m_db);
        }

        SetType makeSet(const std::shared_ptr<Database>& db) const requires std::is_member_object_pointer_v<IdPath>
        {
            return SetType(db, tableName, idPath);
        }

    private:

        std::shared_ptr<Database> m_db;

        const std::string tableName;

        const IdPath idPath;

        std::function<void(TableBuilder<Record>&)> addConstraints;
    };

    template <class IdPath>
    AutoincrementTableInstantiator(std::string, IdPath) ->
        AutoincrementTableInstantiator<detail::IdPathClass<IdPath>, detail::IdPathValue<IdPath>, IdPath>;

    template <class IdPath>
    AutoincrementTableInstantiator(std::string, IdPath,
        std::function<void(TableBuilder<detail::IdPathClass<IdPath>>&)>) ->
        AutoincrementTableInstantiator<detail::IdPathClass<IdPath>, detail::IdPathValue<IdPath>, IdPath>;

    template <class IdPath>
    AutoincrementTableInstantiator(const std::shared_ptr<Database>&, std::string, IdPath) ->
        AutoincrementTableInstantiator<detail::IdPathClass<IdPath>, detail::IdPathValue<IdPath>, IdPath>;

    template <class IdPath>
    AutoincrementTableInstantiator(const std::shared_ptr<Database>&, std::string, IdPath,
        std::function<void(TableBuilder<detail::IdPathClass<IdPath>>&)>) ->
        AutoincrementTableInstantiator<detail::IdPathClass<IdPath>, detail::IdPathValue<IdPath>, IdPath>;
}

