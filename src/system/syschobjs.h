// syschobjs.h
//
#ifndef __SDL_SYSTEM_SYSCHOBJS_H__
#define __SDL_SYSTEM_SYSCHOBJS_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

/* Schema Objects / Type
Every object type has a char(2) type code:
AF - AGGREGATE_FUNCTION (2005) - A user-defined aggregate function using CLR code.
C - CHECK_CONSTRAINT - A constraint on a single table to validate column values.
D - DEFAULT_CONSTRAINT - A short SQL procedure to run to generate a default value for a column when a new row is added, and the value for the column is not specified.
F - FOREIGN_KEY_CONSTRAINT - A constraint between two tables which enforces referential integrity.
FN - SQL_SCALAR_FUNCTION (2000) - A user-defined-function (UDF) which returns a single value.
FS - CLR_SCALAR_FUNCTION (2005) - CLR based scalar function
FT - CLR_TABLE_VALUED_FUNCTION (2005) - CLR based table valued function
IF - SQL_INLINE_TABLE_VALUED_FUNCTION (2000) - SQL inline table-valued function where the function is a single SQL statement.  The advantage of this over a multi statement UDF is that the query optimizer can 'understand' how the inline function works and know how many rows of data to expect ahead of time so that the query optimizer knows what to do without having to use temporary results in a table variable and/or a RECOMPILE or a FORCE ORDER option.
IT - INTERNAL_TABLE (2005) - An automatically created persistent table similar to an index which holds results such as XML nodes, transactional data, service queue messages, etc...  These tables will usually have a unique number appended to the name, and are transparent for the most part to your code, except that they take up space in the database, and need to be there to enable the functionality they provide.
P - SQL_STORED_PROCEDURE - A stored procedure which is capable of running any T-SQL, and can return an integer, output parameters, and multiple result sets.
PC - CLR_STORED_PROCEDURE (2005) - CLR based stored procedure
PG - PLAN_GUIDE (2008) - A query hint that can exist separately from the query that it is targeted to optimize, useful when you need to modify the query optimization on a vendor database where you are not allowed to change the objects directly (usually because they might be overwritten during the next upgrade of the product).
PK - PRIMARY_KEY_CONSTRAINT - A primary key on a table
R - RULE - an old-style stand-alone rule which has been deprecated in favor of CHECK_CONSTRAINT starting with SQL 2000.
RF - REPLICATION_FILTER_PROCEDURE (2005) - A replication-filter procedure
S - SYSTEM_TABLE - A system base table
SN - SYNONYM (2005) - An alias name for a database object
SQ - SERVICE_QUEUE (2005) - An asynchronous message queue
TA - CLR_TRIGGER (2005) - CLR based DML trigger
TF - SQL_TABLE_VALUED_FUNCTION (2000) - A user-defined-function (UDF) which returns a table.
TR - SQL_TRIGGER - DML trigger
TT - TYPE_TABLE (2008) - A user defined table type which can be passed as a parameter to a function or procedure
U - USER_TABLE - A user-defined table
UQ - UNIQUE_CONSTRAINT - Unique constraint on a set of columns
V - VIEW - A user-defined view
X - EXTENDED_STORED_PROCEDURE - An extended stored procedure
*/
struct obj_code // 2 bytes
{
    union {
        char c[2];
        uint16 u;
    };
};

// System Table: syschobjs(ObjectID = 34)
// The sysschobjs table is the underlying table for the sys.objects table.
// It has a NULL bitmap and one variable length column.
// Note that there is also a sys.system_objects table with object IDs less than 0,
// but the objects shown in that view are not in this table.

struct syschobjs_row
{
    struct data_type
	{
        record_head     head;       // 4 bytes
        int32           id;         // id(object_id) - 4 bytes - the unique ID for the object.
                                    // This will be the same as the allocation unit's object ID for system objects, 
                                    // but otherwise it will be a random number between 100 and 2^31.
        uint32          nsid;       // nsid(schema_id) - 4 bytes - the schema ID of this object.
        uint8           nsclass;    // nsclass - 1 byte - this is not shown in the DMV
        uint32          status;     // status - 4 bytes - this is not shown in the DMV
        obj_code        type;       // type(type) - 2 bytes, char(2) - this is the type of the object
        uint32          pid;        // pid(parent_object_id) - 4 bytes - if this object belongs to a table, then this is the ObjectID of the table it belongs to.
        uint8           pclass;     // pclass - 1 byte
        uint32          intprop;    // intprop - 4 bytes
        datetime_t      created;    // created(create_date) - 8 bytes, datetime - the time the object was first created.
        datetime_t      modified;   // modified(modify_date) - 8 bytes, datetime - the time the schema for this object was last modified.

        /*FIXME: name (name) - the name of the object, Unicode, variable length. 
        The (name, schema) must be unique among all objects in a database.*/
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#pragma pack(pop)

template<> struct row_traits<syschobjs_row> {
    enum { null_bitmap = 1 };
};

struct syschobjs_row_meta {

    typedef_col_type_n(syschobjs_row, head);
    typedef_col_type_n(syschobjs_row, id);
    typedef_col_type_n(syschobjs_row, nsid);
    typedef_col_type_n(syschobjs_row, nsclass);
    typedef_col_type_n(syschobjs_row, status);
    typedef_col_type_n(syschobjs_row, type);
    typedef_col_type_n(syschobjs_row, pid);
    typedef_col_type_n(syschobjs_row, pclass);
    typedef_col_type_n(syschobjs_row, intprop);
    typedef_col_type_n(syschobjs_row, created);
    typedef_col_type_n(syschobjs_row, modified);

    typedef TL::Seq<
        head
        ,id
        ,nsid
        ,nsclass
        ,status
        ,type
        ,pid
        ,pclass
        ,intprop
        ,created
        ,modified
    >::Type type_list;

    syschobjs_row_meta() = delete;
};

struct syschobjs_row_info {
    syschobjs_row_info() = delete;
    static std::string type_meta(syschobjs_row const &);
    static std::string type_raw(syschobjs_row const &);
    static const char * code_name(obj_code const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSCHOBJS_H__