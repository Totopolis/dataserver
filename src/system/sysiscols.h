// sysiscols.h
//
#ifndef __SDL_SYSTEM_SYSISCOLS_H__
#define __SDL_SYSTEM_SYSISCOLS_H__

#pragma once

#include "page_type.h"

namespace sdl { namespace db {

/*System Table: sysiscols (ObjectID = 55)
The sysiscols is a table with only 22 bytes of fixed length data per row, 
and it defines all indexes and statistics in system and user tables, 
both clustered and non-clustered indexes. 
Heaps (index id = 0) are not included in this table. 
The data in this table is visible via the DMV sys.index_columns (indexes only, statistics columns are not included). 
This table is clustered by (idmajor, idminor, subid):
*/
struct sysiscols
{
    //FIXME: to be tested
    struct data_type {

        //idmajor(object_id) - 4 bytes - the object_id of the object that the index is defined on
        //idminor(index_id) - 4 bytes - the index id or statistics id for each object_id
        //subid(index_column_id) - 4 bytes - when an index contains multiple columns, this is the order of the columns within the index.For statistics, this appears to always be 1
        //status - 4 bytes - bit mask : 0x1 appears to always be set, 0x2 for index, 0x4 for a descending index column(is_descending_key).
        //intprop(column_id) - 4 bytes - appears to the column(syscolpars.colid)
        //tinyprop1 - 1 byte - appears to be equal to the subid for an index, 0 for a statistic.
        //tinyprop2 - 1 byte - appears to always be 0

    };
    data_type data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSISCOLS_H__