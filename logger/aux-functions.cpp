#include <iostream>
#include <stdlib.h>

#include "globals.hpp"
#include "aux-functions.h"

VOID print_log ( std::string message, int level )
{
    if ( level <= KnobVerboseLevel.Value () )
    {
        int process_id = LEVEL_PINCLIENT::PIN_GetPid ();
        std::cout << "[PT][" << process_id << "]" << message << endl;
    }
}

VOID throw_error ( string message )
{
    print_log ( "E:"+message, 0 );
    exit ( -1 );
}

VOID throw_warning ( string message )
{
    print_log ( "W:"+message, 0 );
}
