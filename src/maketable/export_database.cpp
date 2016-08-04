// export_database.cpp
//
#include "common/common.h"
#include "export_database.h"
#include "generator_util.h"
#include <fstream>

namespace sdl { namespace db { namespace make { namespace {

const char INSERT_TEMPLATE[] = R"(
SET IDENTITY_INSERT %s{TABLE_DEST} ON;
GO
INSERT INTO %s{TABLE_DEST} (%s{COL_TEMPLATE})
       SELECT %s{COL_TEMPLATE}
       FROM %s{TABLE_SRC};
SET IDENTITY_INSERT %s{TABLE_DEST} OFF;
GO
)";

const char TABLE_TEMPLATE[] = R"(%s{database}.dbo.%s{table})";

} // namespace 

bool export_database::make_file(std::string const & in_file, std::string const & out_file)
{
    return false;
}

} // make
} // db
} // sdl
