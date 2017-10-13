USE [GIS{name}]
GO 
SELECT distinct sys.Tables.schema_id, SCHEMA_NAME(sys.Tables.schema_id)
FROM sys.Tables
;
  