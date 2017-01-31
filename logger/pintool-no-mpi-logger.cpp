#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>

#include "globals.hpp"
#include "sim_api.h"
#include "pinplay-functions.h"
#include "mpi-control.h"
#include "extrae-control.h"
#include "dyn-swindow.h"


KNOB_COMMENT pinplay_driver_knob_family ( KNOB_FAMILY, "PinPlay Driver Knobs" );

KNOB < BOOL > KnobReplayer ( KNOB_MODE_WRITEONCE, KNOB_FAMILY, "replay", "0","Replay a pinball" );
KNOB < BOOL > KnobLogger ( KNOB_MODE_WRITEONCE, KNOB_FAMILY, "log", "0","Create a pinball" );
KNOB < BOOL > KnobRoiActivated ( KNOB_MODE_WRITEONCE, KNOB_FAMILY, "roi","0","If ROI activated, only get pinballs of this region." );
KNOB < BOOL > KnobExtraeEvents ( KNOB_MODE_WRITEONCE, KNOB_FAMILY,"extrae-events", "0","The pintool generates Extrae_user events for every outper-mpi region." );

KNOB < unsigned long > KnobExtraeEventsId ( KNOB_MODE_WRITEONCE, KNOB_FAMILY,"extrae-events-id",PREDEFINED_EXTRAE_EVENT_ID,"The user events are identified with this number." );
KNOB < int >KnobVerboseLevel ( KNOB_MODE_WRITEONCE, KNOB_FAMILY, "v", "0","Level of verbosity." );

statistics_t statistics;
UINT32 mpi_rank;

LOCALFUN INT32 Usage ()
{
    cerr << "Usage: pinplay-driver-on-off Args  -- app appargs ..." << endl;
    cerr << "Arguments:" << endl;
    cerr << KNOB_BASE::StringKnobSummary ();
    cerr << endl;

    return -1;
}

VOID Fini ( INT32 code, VOID * v )
{
    FiniDSW();
    std::cout << "[PT][RANK: " << mpi_rank << "]Num pb created: " << statistics.pinball_count     << std::endl;
}



int main ( int argc, char *argv[] )
{
    if (PIN_Init (argc, argv))
        return Usage ();

    statistics.num_mpi_instrum = 0;
    statistics.pinball_count = 0;

    PIN_InitSymbols ();
    initMPIControl();
    initPinPlay(argc, argv);
    initExtraeControl();

    //initDynSlidingWindows(); /* Conocer la direccion del primer BBL */

    //PIN_AddFiniFunction (Fini, 0);

    PIN_StartProgram();
}


/*
VOID handleMagic ( CONTEXT * ctxt, ADDRINT gax, ADDRINT gbx, ADDRINT gcx )
{
    if ( gax == SIM_CMD_ROI_START )
    {
        if ( !IN_GMPI )
        {
            throw_error ( "The beggining of ROI must be after MPI_Init." );
        }

        in_roi = true;
        logging_action ( ctxt, EVENT_START, "PtRoiStart", 0 );
        print_log ( "Start ROI.", 0 );
    }
    else if ( gax == SIM_CMD_ROI_END )
    {
        if ( !IN_GMPI )
        {
            throw_error ( "The end of ROI must be before MPI_Finalize." );
        }

        in_roi = false;
        logging_action ( ctxt, EVENT_STOP, "PtRoiEnd", 0 );
        print_log ( "Stop ROI.", 0 );
    }
}


    if ( KnobRoiActivated.Value () && is_main_executable )
    {
        // Looking for ROI markers. Only in the main binary

        for ( SEC section = IMG_SecHead (img); SEC_Valid (section); section = SEC_Next (section) ) {
            for ( RTN rtn = SEC_RtnHead (section); RTN_Valid (rtn); rtn = RTN_Next (rtn) ) {
                RTN_Open ( rtn );
                for ( INS ins = RTN_InsHead (rtn); INS_Valid (ins); ins = INS_Next (ins) ) {
                    // Simics-style magic instruction: xchg bx, bx
                    if ( INS_IsXchg ( ins ) && INS_OperandReg ( ins, 0 ) == REG_BX && INS_OperandReg ( ins, 1 ) == REG_BX )
                    {
                        INS_InsertPredicatedCall ( ins, IPOINT_BEFORE, ( AFUNPTR )
                                handleMagic,
                                IARG_CONTEXT,
                                IARG_REG_VALUE, REG_GAX,
#ifdef TARGET_IA32
                                IARG_REG_VALUE, REG_GDX,
#else
                                IARG_REG_VALUE, REG_GBX,
#endif
                                IARG_REG_VALUE, REG_GCX,
                                IARG_END );

                        statistics.num_roi_markers++;
                    }
                }
                RTN_Close ( rtn );
            }
        }

        if ( statistics.num_roi_markers == 0 )
        {
            throw_warning ( "No ROI markers has been detected." );
        }
        else if ( statistics.num_roi_markers == 1 )
        {
            throw_warning ( "Only one marker has been detected. Revise your ROI." );
        }
    }


    print_log ( ".. Inspected.", 2 );
}
*/
