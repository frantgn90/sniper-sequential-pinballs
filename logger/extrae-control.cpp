#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "globals.hpp"
#include "extrae-control.h"
#include "aux-functions.h"
#include "pinplay-functions.h"

static AFUNPTR extrae_event_rtn = NULL;
extrae_image_t extrae_image;

BOOL inExtrae(ADDRINT addr)
{
    if (extrae_image.loaded)
        return (addr >= extrae_image.low_addr && addr < extrae_image.high_addr);
    else
        return false;
}

VOID callExtrae_event(const CONTEXT * ctxt, THREADID threadid, unsigned long event, unsigned long value)
{
    assert(extrae_event_rtn != NULL);

    CALL_APPLICATION_FUNCTION_PARAM param;
    memset ( &param, 0, sizeof(param));
    param.native = 1; // PIN won't instrument this call

    PIN_CallApplicationFunction ( ctxt, threadid,
            CALLINGSTD_DEFAULT,
            extrae_event_rtn,
            &param,
            PIN_PARG ( void ),
            PIN_PARG ( unsigned long ), event,
            PIN_PARG ( unsigned long ), value,
            PIN_PARG_END () );
}

VOID rtnExtraeCallback(RTN rtn, VOID * v)
{
    std::string rtn_name = RTN_Name(rtn);

    if (   strcmp ( rtn_name.c_str (), "Extrae_event" )   == 0
        || strcmp ( rtn_name.c_str (), "OMPtrace_event" ) == 0 // F apps
        || strcmp ( rtn_name.c_str (), "SEQtrace_event" ) == 0 // C apps
    )
    {
        cout << "Extrae_event detected: " + rtn_name + " 0x" << hex << to_string(RTN_Address(rtn)) << endl;
        extrae_event_rtn = RTN_Funptr(rtn);
    }
}

VOID imgExtraeCallback(IMG img, VOID * v)
{
    std::string img_name = IMG_Name(img);

    if (img_name.find("libmpitrace") != std::string::npos)
    {
        extrae_image.low_addr = IMG_LowAddress(img);
        extrae_image.high_addr = IMG_HighAddress(img);
        extrae_image.loaded = true;
    }
}

VOID initExtraeControl()
{
    extrae_image.low_addr = 0;
    extrae_image.high_addr = 0;
    extrae_image.loaded = false;

    IMG_AddInstrumentFunction(imgExtraeCallback, 0);
    RTN_AddInstrumentFunction(rtnExtraeCallback, 0);
}
