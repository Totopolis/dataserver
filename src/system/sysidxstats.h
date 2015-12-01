// sysidxstats.h
//
#ifndef __SDL_SYSTEM_SYSIDXSTATS_H__
#define __SDL_SYSTEM_SYSIDXSTATS_H__

#pragma once

#include "page_type.h"

namespace sdl { namespace db {

/*(ObjectID = 54)
The sysidxstats system table has one row for every index or heap 
in the database, including indexes on tables returned with table valued functions 
and user defined table types.  It is visible with through the DMV sys.indexes 
and sys.sysindexes.  This table is clustered by (id, indid).  
This table also includes automatically created statistics on columns 
that don't have indexes - these statistics have automatically generated names 
of the form _WA_Sys_[ObjectID-as 8 digit hex number]_[random 8 digit hex number] - 
the query optimizer uses them to select the best excution plan when 
a join includes unindexed columns that only have statistics.*/
struct sysidxstats
{
    //FIXME: to be tested
    struct data_type {

	    /*id (object_id) - 4 bytes - the object_id of the table or view that this index belongs to*/
	    uint32 id;

	    /*indid (index_id) - 4 bytes - the index_id (1 for the clustered index, 
	    larger numbers for non-clustered indexes)*/
	    uint32 indid;

	    /*status - 4 bytes - Note - this is NOT the same as the column sys.sysindexes.status.
	    0x10 = pad index turned on (is_padded).*/
	    uint32 status;

	    /*intprop - 4 bytes*/
	    uint32 intprop;

	    /*fillfact (fill_factor) - 1 byte - the fill factor for the index in percent (0-100), 
	    defaults to 0.*/
	    uint8 fillfact;

	    /*type (type) - 1 byte - 0 for heap, 1 for clustered index, 2 for non-clustered index*/
	    uint8 type;

	    // tinyprop - 1 byte
	    uint8 tinyprop;
	
	    /* dataspace - 4 bytes - appears to be 1 (PRIMARY) for permanent tables, 
	    and 0 for return tables of functions and user defined table types (NO data space).*/
	    uint32 dataspace;
	
	    /*lobds - 4 bytes*/
	    uint32 lobds;

	    /*rowset - 8 bytes - appears to be the sysallocunits.container_id 
	    that this index belongs, visible through the DMV sys.partitions.hobt_id.*/
	    uint64 rowset;

	    /*name (name) - nvarchar - the name of the index.  
	    This will be programmatically generated for function generated table and 
	    user defined table types.  It will be NULL for heaps.*/
    };
    data_type data;
};

} // db
} // sdl

#endif // __SDL_SYSTEM_SYSSCALARTYPES_H__