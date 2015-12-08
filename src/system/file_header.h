// file_header.h
//
#ifndef __SDL_SYSTEM_FILE_HEADER_H__
#define __SDL_SYSTEM_FILE_HEADER_H__

#pragma once

#include "page_head.h"

namespace sdl { namespace db {

#pragma pack(push, 1) 

// The first page in each database file is the file header page, and there is only one such page per file.
struct fileheader_row
{
    //FIXME: to be tested
    struct data_type { // IS_LITTLE_ENDIAN

        datarow_head    head;           // 4 bytes

        // The fixed length file header fields, followed by the offsets for the variable length fields are as following:
        char        _0x00[2];               // 0x00 : 2 bytes - value 0x0030
        char        _0x02[2];               // 0x02 : 2 bytes - value 0x0008
        char        _0x04[4];               // 0x04 : 4 bytes - value 0
        char        _0x08[4];               // 0x08 : 4 bytes - value 47 (49 ?)
        char        _0x0C[4];               // 0x0C : 4 bytes - value 0
        uint16      NumberFields;           // 0x10 : NumberFields - 2 bytes - count of number fields
        uint16      FieldEndOffsets;        // 0x12 : FieldEndOffsets - 2 * (NumFields)-offset to the end of each field in bytes relative to the start of the page.
                                            // The last offset is the end of the overall file header structure
        // The variable length fields are(by column index) :
        // ...
    };
    union {
        data_type data;
        char raw[sizeof(data_type)];
    };
};

#if 0
http://ugts.azurewebsites.net/data/UGTS/document/2/4/46.aspx

File Header Page

The first page in each database file is the file header page, and there is only one such page per file.Reading this page can be done either with the standard DBCC PAGE command, or with the DBCC FILEHEADER command which is also undocumented, see example below :

DBCC FILEHEADER('[dbname]', [FileID])
DBCC FILEHEADER('testDB', 1)

The file header page has a format identical to a data page, but the table is a system table whose schema seems to be hard - coded into SQL server, rather than defined in any other table.This data page has only a single row, and most of the columns are in the variable length columns section.
The page / tornBits checksum in the page header acts as a security checksum for this page - if you try to manually edit the header data in a binary editor and then attach the file in SSMS, the UI will tell you that the file is not a primary database file.
The first 16 bytes of the file header are fixed length(16 bytes).The remaining fields are all optional / variable length using the same structure as the variable length columns section of a data row.Therefore, rather than listing the offsets of each of these fields, the field index instead is given(there are 46 such fields in SQL 2008 R2, numbered 0x0 - 0x2d.To lookup the position of field N, read the offset at 0x12 + N * 2 and the offset just before it, to get the start to end position of the field in the file header.
The fixed length file header fields, followed by the offsets for the variable length fields are as following :

0x00 : 2 bytes - value 0x0030
0x02 : 2 bytes - value 0x0008
0x04 : 4 bytes - value 0
0x08 : 4 bytes - value 47
0x0C : 4 bytes - value 0
0x10 : NumberFields - 2 bytes - count of number fields
0x12 : FieldEndOffsets - 2 * (NumFields)-offset to the end of each field in bytes relative to the start of the page.
The last offset is the end of the overall file header structure
The variable length fields are(by column index) :

0 : BindingID - 16 byte GUID - used to make sure this file is really part of this database, according to Paul Randal(but how ? )
1 : ? ? -0 bytes
2 : FileIdProp - 2 bytes - the FileID value for this file
3 : FileGroupID - 2 bytes - not sure what this is, value usually 1
4 : Size - 4 bytes - number of 8KB pages in this database file
5 : MaxSize - 4 bytes - maximum number of pages allowed for this file(-1 for unlimited size).
6 : Growth - 4 bytes - the number of pages to automatically grow the database file by, or a whole number percent value for auto - growth, depending on Status bit 0x100000.
7 : Perf - 4 bytes - value 0
8 : BackupLSN - 10 bytes
9 : FirstUpdateLSN - 10 bytes
10 : OldestRestoredLSN - 10 bytes
11 : FirstNonLoggedLSN - 0 bytes
12 : MinSize - 4 bytes
13 : Status - 4 bytes - bitmask for state of the database : 0x2 = regular database file.  0x100000 = grow the database file by a percent rather than by a fixed number of pages.
14 : UserShrinkSize - 4 bytes
15 : SectorSize - 4 bytes - the disk sector size in bytes, usually 512 bytes.
16 : MaxLSN ? -10 bytes
17 : MaxLSN structure - 28 bytes - MaxLSN(10 byte LSN, repeated), zero value(? ? ), MaxLsnBranchID - (16 byte GUID)
18 : FirstLSN - 10 bytes
19 : CreateLSN - 10 bytes
20 : DifferentialBaseLSN - 10 bytes
21 : DifferentialBaseGuid - 16 byte GUID
22 : FileOffsetLSN - 10 bytes
23 : FileIDGuid - 16 byte GUID
24 : RestoreStatus - 4 bytes
25 : RestoreRedoStartLSN / RedoStartLSN - 10 bytes
26 : ? ? -0 bytes
27 : LogicalName - 2 * num chars - the logical name of this database file
28 : RestoreSourceGuid - 16 byte GUID
29 - 34 : HighestUpdateLSN, ReplTxfTruncationLsn, TxfBackupLsn, TxfLogContainerSize, ? ? , ? ? -0 bytes
35 : MaxLsnBranchID - 16 byte GUID
36 - 37 : SecondaryRedoStartLSN, SecondaryDifferentialBaseLSN - 0 bytes
38 : ReadOnlyLSN - 10 bytes
39 : ReadWriteLSN - 10 bytes
40 : ? ? , 28 bytes, value 0
41 : RestoreDiffBaseLSN - 10 bytes
42 : RestoreDifferentialBaseGuid - 16 byte GUID
43 : RestorePathOriginLSN, RestorePathOriginGUID - 28 bytes(? )
44 : 8 bytes
45 : remaining bytes - padding ?
#endif

#pragma pack(pop)

struct fileheader_row_meta {

    typedef_col_type_n(fileheader_row, head);
    typedef_col_type_n(fileheader_row, NumberFields);
    typedef_col_type_n(fileheader_row, FieldEndOffsets);

    typedef TL::Seq<
            head
            ,NumberFields
            ,FieldEndOffsets
    >::Type type_list;

    fileheader_row_meta() = delete;
};

struct fileheader_row_info {
    fileheader_row_info() = delete;
    static std::string type_meta(fileheader_row const &);
    static std::string type_raw(fileheader_row const &);
};

} // db
} // sdl

#endif // __SDL_SYSTEM_FILE_HEADER_H__
