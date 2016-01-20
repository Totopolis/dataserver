// boot_page.h
//
#ifndef __SDL_SYSTEM_BOOT_PAGE_H__
#define __SDL_SYSTEM_BOOT_PAGE_H__

#pragma once

#include "page_head.h"

// http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx

namespace sdl { namespace db {

#pragma pack(push, 1) 

struct recovery_t  // 28 bytes
{
    struct data_type {
        pageLSN l;          // 10 bytes
        char padding[2];    // 2 bytes
        guid_t g;           // 16 bytes
    };
    data_type data;
};

// Each database has a single boot page at 1:9, 
// which has much the same format as the file header page - a data page with a single record.
// However, this record has no NULL bitmap or variable length column data - it is composed entirely of fixed length columns, 
// which makes parsing it very easy.
struct bootpage_row
{
    struct data_type { // IS_LITTLE_ENDIAN
        char        _0x00[4];                       // 0x00
        uint16      dbi_version;                    // 0x04 : dbi_version - 2 bytes - the file version of the database.For example, this is 661 for SQL 2008 R2.In general, database files can be loaded by any version of SQL server higher than the version that they were first introduced.Restoring an earlier version of a database file will first convert it to the most current version that the storage engine uses.Thus, you cannot take a database file from a later version of SQL server and load it into an earlier version of SQL server.However, there are other ways to move data, such as replication and T - SQL statements.
        uint16      dbi_createVersion;              // 0x06 : dbi_createVersion - 2 bytes - the file version of the database when it was first created.
        char        _0x08[28];                      // 0x08 : 28 bytes - value = 0
        uint32      dbi_status;                     // 0x24 : dbi_status - 4 bytes
        uint32      dbi_nextid;                     // 0x28 : dbi_nextid - 4 bytes
        datetime_t  dbi_crdate;                     // 0x2C : dbi_crdate - 8 bytes - the date that the database was created(not the original creation date, but the last time it was restored).
        nchar_t     dbi_dbname[128];                // 0x34 : dbi_dbname - 256 bytes - nchar(128) - the name of the database in Unicode, padded on the right with space characters.
                                                    // Note that wchar_t doesn't need to be a 16 bit type - there are platforms where it's a 32 - bit type.
        char        _0x134[4];                      // 0x134 : ? ? -4 bytes, value = 6
        uint16      dbi_dbid;                       // 0x138 : dbi_dbid - 2 bytes - the database ID.Since this is only 2 bytes, there can be only 2 ^ 15 databases per SQL instance.
        char        _0x13A[2];                      // 0x13A : 2 bytes, value = 0x64
        uint32      dbi_maxDbTimestamp;             // 0x13C : dbi_maxDbTimestamp - 4 bytes
        char        _0x140[16];                     // 0x140 : ? ? -16 bytes - value = zero
        pageLSN     dbi_checkptLSN;                 // 0x150 : dbi_checkptLSN - 10 bytes
        char        _0x15A[2];                      // 0x15A : ? ? -2 bytes - value = 6
        pageLSN     dbi_differentialBaseLSN;        // 0x15C : dbi_differentialBaseLSN - 10 bytes
        uint16      dbi_dbccFlags;                  // 0x166 : dbi_dbccFlags - 2 bytes
        char         _0x168[32];                    // 0x168 : ? ? -24 bytes - value 0
                                                    // 0x180 : ? ? -4 bytes, value 0x2682 = 9858
                                                    // 0x184 : ? ? -4 bytes - value 0
        uint32      dbi_collation;                  // 0x188 : dbi_collation - 4 bytes
        char        _0x18C[12];                     // 0x18C : ? ? -12 bytes, value 0 with one 0x61 byte
        guid_t      dbi_familyGuid;                 // 0x198 : dbi_familyGuid - 16 bytes
        uint32      dbi_maxLogSpaceUsed;            // 0x1A8 : dbi_maxLogSpaceUsed - 4 bytes
        char        _0x1AC[16];                     // 00x1AC : ? ? -16 bytes - value 0
        recovery_t  dbi_recoveryForkNameStack[2];   // 0x1BC : dbi_recoveryForkNameStack - 56 bytes - array of two 28 - byte entry records = entry[0..1].
                                                    // Each entry holds an LSN, two bytes of padding = 0, and a GUID value.
        guid_t      dbi_differentialBaseGuid;       // 0x1F4 : dbi_differentialBaseGuid - 16 bytes
        pageFileID  dbi_firstSysIndexes;            // 0x204 : dbi_firstSysIndexes - 6 bytes - the FileID : PageID page locator for the first sysindex page
                                                    // - this is the first data page of the sysallocunits(objectID = 7) table, 
                                                    // which is the global table directory that lists all the allocation units in the database by their ObjectID,
                                                    // and their first data page and IAM page. In other words, this is the table that is the entry point to 
                                                    // describe all other tables and indexes in the database.
        uint16      dbi_createVersion2;             // 0x20A : dbi_createVersion - 2 bytes - repeated ?
        char        _0x20C[12];                     // 0x20C : ? ? -12 bytes - value 0
        pageLSN     dbi_versionChangeLSN;           // 0x218 : dbi_versionChangeLSN - 10 bytes
        char        _0x222[94];                     // 0x222 : ? ? -94 bytes - value 0
        pageLSN     dbi_LogBackupChainOrigin;       // 0x280 : dbi_LogBackupChainOrigin - 10 bytes
        char        _0x28A[26];                     // 0x28A : ? ? -26 bytes, value 0
        datetime_t  dbi_modDate;                    // 0x2A4 : dbi_modDate - 8 bytes
        uint32      dbi_verPriv;                    // 0x2AC : dbi_verPriv - 4 bytes
        char        _0x2B0[4];                      // 0x2B0 : ? ? -4 bytes, value 0
        guid_t      dbi_svcBrokerGUID;              // 0x2B4 : dbi_svcBrokerGUID - 16 bytes
        char        _0x2C4[28];                     // 0x2C4 : ? ? -28 bytes, value 0
        uint64      dbi_AuIdNext;                   // 0x2E0 : dbi_AuIdNext - 8 bytes
        char        _0x2E8[700];                    // 0x2E8 : (padding)-700 bytes
    };
    union {
        data_type data;
        char raw[page_head::body_size]; // [1024*8 - 96] = [8096]
    };
};

#pragma pack(pop)

struct recovery_meta: is_static {

    typedef_col_type_n(recovery_t, l);
    typedef_col_type_n(recovery_t, g);

    typedef TL::Seq<l, g>::Type type_list;
};

struct bootpage_row_meta: is_static {

    typedef_col_type_n(bootpage_row, dbi_version);
    typedef_col_type_n(bootpage_row, dbi_createVersion);
    typedef_col_type_n(bootpage_row, dbi_status);
    typedef_col_type_n(bootpage_row, dbi_nextid);
    typedef_col_type_n(bootpage_row, dbi_crdate);
    typedef_col_type_n(bootpage_row, dbi_dbname);
    typedef_col_type_n(bootpage_row, dbi_dbid);
    typedef_col_type_n(bootpage_row, dbi_maxDbTimestamp);
    typedef_col_type_n(bootpage_row, dbi_checkptLSN);
    typedef_col_type_n(bootpage_row, dbi_differentialBaseLSN);
    typedef_col_type_n(bootpage_row, dbi_dbccFlags);
    typedef_col_type_n(bootpage_row, dbi_collation);
    typedef_col_type_n(bootpage_row, dbi_familyGuid);
    typedef_col_type_n(bootpage_row, dbi_maxLogSpaceUsed);
    typedef_col_type_n(bootpage_row, dbi_recoveryForkNameStack);
    typedef_col_type_n(bootpage_row, dbi_differentialBaseGuid);
    typedef_col_type_n(bootpage_row, dbi_firstSysIndexes);
    typedef_col_type_n(bootpage_row, dbi_createVersion2);
    typedef_col_type_n(bootpage_row, dbi_versionChangeLSN);
    typedef_col_type_n(bootpage_row, dbi_LogBackupChainOrigin);
    typedef_col_type_n(bootpage_row, dbi_modDate);
    typedef_col_type_n(bootpage_row, dbi_verPriv);
    typedef_col_type_n(bootpage_row, dbi_svcBrokerGUID);
    typedef_col_type_n(bootpage_row, dbi_AuIdNext);

    typedef TL::Seq<
        dbi_version
        ,dbi_createVersion
        ,dbi_status
        ,dbi_nextid
        ,dbi_crdate
        ,dbi_dbname
        ,dbi_dbid
        ,dbi_maxDbTimestamp
        ,dbi_checkptLSN
        ,dbi_differentialBaseLSN
        ,dbi_dbccFlags
        ,dbi_collation
        ,dbi_familyGuid
        ,dbi_maxLogSpaceUsed
        ,dbi_recoveryForkNameStack
        ,dbi_differentialBaseGuid
        ,dbi_firstSysIndexes
        ,dbi_createVersion2
        ,dbi_versionChangeLSN
        ,dbi_LogBackupChainOrigin
        ,dbi_modDate
        ,dbi_verPriv
        ,dbi_svcBrokerGUID
        ,dbi_AuIdNext
    >::Type type_list;
};

struct boot_info: is_static {
    static std::string type_meta(bootpage_row const &);
    static std::string type_raw(bootpage_row const &);
private:
    static std::string type(bootpage_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_BOOT_PAGE_H__
