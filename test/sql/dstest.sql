/*
    sqlcmd script, creates dstest.mdf in specified dir

    usage example:
    sqlcmd -i dstest.sql -v database_dir="d:\work"
*/

USE master;

-- Drop database
IF DB_ID('dstest') IS NOT NULL
    DROP DATABASE dstest;

-- If database could not be created due to open connections, abort
IF @@ERROR = 3702
    RAISERROR('Database cannot be dropped because there are still open connections.', 127, 127) WITH NOWAIT, LOG;

-- Create database
CREATE DATABASE dstest ON PRIMARY
(
                                  NAME = dstest_dat,
                                  FILENAME = '$(database_dir)/dstest.mdf',
                                  SIZE = 5 MB,
                                  MAXSIZE = UNLIMITED,
                                  FILEGROWTH = 4096 KB
) log ON
(
                                  NAME = dstest_log,
                                  FILENAME = '$(database_dir)/dstest.ldf',
                                  SIZE = 2 MB,
                                  MAXSIZE = UNLIMITED,
                                  FILEGROWTH = 2048 KB
);
GO

USE dstest;

-------------------------------------------------------------------------------
-- SupportedTypes
-------------------------------------------------------------------------------
CREATE TABLE dbo.SupportedTypes
(
-- with PKEY assert fail in database.cpp
Id              INT IDENTITY(1, 1) NOT NULL, -- CONSTRAINT PK_TypesExactNumeric PRIMARY KEY,
 
c_tinyint       TINYINT,
c_smallint      SMALLINT,
c_bigint        BIGINT,
 
-- generated *.h can't be compiled
-- c_decimal5b  DECIMAL(9, 2),
-- c_decimal9b  DECIMAL(19, 6),
-- c_decimal13b DECIMAL(28, 8),
-- c_decimal17b DECIMAL(38, 14),
 
c_smallmoney    SMALLMONEY,
-- generated *.h can't be compiled
-- c_money      MONEY,
-- assert fail in datatable.cpp
-- c_bit1       BIT,
-- c_bit2       BIT

c_float53       FLOAT(53), -- 8 bytes
c_real          REAL, -- 4 bytes

c_smalldatetime SMALLDATETIME DEFAULT CONVERT(SMALLDATETIME, SYSUTCDATETIME()),
-- generated *.h can't be compiled
-- c_time           TIME DEFAULT CONVERT(TIME, GETDATE()),
-- c_date           DATE DEFAULT CONVERT(DATE, GETDATE()), 
-- c_datetime       DATETIME DEFAULT GETDATE(),
-- c_datetime2      DATETIME2 DEFAULT SYSUTCDATETIME(),
-- c_datetimeoffset DATETIMEOFFSET DEFAULT SYSDATETIMEOFFSET(),

c_char100       CHAR(100),
c_varchar100    VARCHAR(100),
c_varchar_max   VARCHAR(MAX),

c_text          TEXT,

c_nchar100      NCHAR(100),
c_nvarchar100   NVARCHAR(100),
c_nvarchar_max  NVARCHAR(MAX),

c_ntext         NTEXT,

-- assert fail in datatable.cpp
-- c_binary100     BINARY(100),
-- c_varbinary100  VARBINARY(100),
-- c_varbinary_max VARBINARY(MAX),
);
GO

--INSERT INTO dbo.SupportedTypes
--DEFAULT VALUES;

--SELECT *
--FROM dbo.SupportedTypes;

-------------------------------------------------------------------------------
-- Table1
-- Simple data for basic SELECT tests
-------------------------------------------------------------------------------
CREATE TABLE dbo.Table1
(Id     INT IDENTITY(1, 1) NOT NULL,
 c_vchar VARCHAR(255),
);
GO

INSERT INTO dbo.Table1(c_vchar)
VALUES('one'), ('two'), ('three'), ('four'), ('five'), ('six'), ('seven'), ('eight'), ('nine'), ('ten');

--SELECT *
--FROM dbo.Table1;

-------------------------------------------------------------------------------
-- Table2
-- CHAR, VARCHAR, TEXT columns
-- For tests of NULL and empty strings
-------------------------------------------------------------------------------
CREATE TABLE dbo.Table2
(Id     INT IDENTITY(1, 1) NOT NULL,
 c_char CHAR(10),
 c_vchar VARCHAR(10),
 c_text TEXT
);
GO

INSERT INTO dbo.Table2(c_char, c_vchar, c_text)
VALUES(NULL, NULL, NULL), ('', 'vchar', 'text'), ('0123456789', '0123456789', '0123456789')

--SELECT *
--FROM dbo.Table2;

-------------------------------------------------------------------------------
-- Table3
-- table with CHAR column, containing content of varying length
-------------------------------------------------------------------------------
CREATE TABLE dbo.Table3
(Id     INT IDENTITY(1, 1) NOT NULL,
 c_char CHAR(32)
);
GO

INSERT INTO dbo.Table3(c_char)
VALUES('0000000000');


--SELECT *
--FROM dbo.Table3;

UPDATE dbo.Table3
  SET
      c_char = '11111'
WHERE Id = 1;

--SELECT *
--FROM dbo.Table3;

-------------------------------------------------------------------------------
-- Detach
-------------------------------------------------------------------------------
USE master;

EXEC sp_detach_db 'dstest';
GO
