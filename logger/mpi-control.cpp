#include <fstream>
#include <iostream>
#include <string.h>
#include <stdlib.h>

#include "globals.hpp"
#include "mpi-control.h"
#include "extrae-control.h"
#include "aux-functions.h"
#include "pinplay-functions.h"

#include <boost/algorithm/string/find.hpp> // ifind_first

UINT32 last_mpi_func = 0;
UINT64 instr_counter = 0;

BOOL in_mpi = false;
BOOL must_trace = false;
BOOL have_rank = false;

using namespace std;

VOID initMpi(CONTEXT * ctxt, THREADID thread_id, BOOL init, UINT32 rtn_id)
{
    if (init)
    {
        cout << "!! INICIO" << endl;
        must_trace = true;
        callExtrae_event(ctxt, thread_id, KnobExtraeEventsId.Value(), statistics.pinball_count);

        in_mpi = true;

        #if OFFSETS>0
        newInstrCounterCheckpoint(thread_id, must_trace, rtn_id);
        #endif

        loggingAction(thread_id, ctxt, must_trace, rtn_id);
    }
    else
    {
        cout << "!! FINAL" << endl;
        must_trace = false;

        loggingAction(thread_id, ctxt, must_trace, rtn_id);
        callExtrae_event(ctxt, thread_id, KnobExtraeEventsId.Value(), PINBALL_EVENT_FINI);

        #if OFFSETS>0
        newInstrCounterCheckpoint(thread_id, must_trace, rtn_id);
        #endif

        in_mpi = false;
    }
}
VOID boundaryMPI(CONTEXT * ctxt, THREADID thread_id, BOOL entering, UINT32 rtn_id)
{
    if (!in_mpi)
    {
        must_trace = false;
    }
    else if (must_trace && entering)
    {
        must_trace = false;
        callExtrae_event(ctxt, thread_id, KnobExtraeEventsId.Value(), PINBALL_EVENT_FINI);
        last_mpi_func = rtn_id;
    }
    else if (!must_trace && !entering && last_mpi_func == rtn_id)
    {
        must_trace = true;
        statistics.pinball_count++;
        callExtrae_event(ctxt, thread_id, KnobExtraeEventsId.Value(),statistics.pinball_count);
    }
    else
    {
        return;
    }

    #if OFFSETS>0
    newInstrCounterCheckpoint(thread_id, must_trace, rtn_id);
    #else
    loggingAction(thread_id, ctxt, must_trace, rtn_id);
    #endif
}

VOID getRank(CONTEXT * ctxt, THREADID thread_id, int * rank, UINT32 rtn_id)
{
    if (!have_rank)
    {
        mpi_rank = *rank;
        addRanktoBaseName(mpi_rank);
        have_rank = true;
    }
}

VOID rtnCallback(RTN rtn, VOID * v)
{
    string  rtn_name = RTN_Name(rtn);
    UINT32  rtn_id = RTN_Id(rtn);
    ADDRINT rtn_addr = RTN_Address(rtn);

    if (    !distance(boost::algorithm::ifind_first(rtn_name, "MPI_Init").begin(), rtn_name.begin())
         && !distance(boost::algorithm::ifind_first(rtn_name, "MPI_Initialized").begin(), rtn_name.end())
         && !distance(boost::algorithm::ifind_first(rtn_name, "_Wrapper").begin(), rtn_name.end())
         && inExtrae(rtn_addr))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)initMpi, IARG_CONTEXT,
                       IARG_THREAD_ID, IARG_BOOL, true, IARG_ADDRINT, rtn_id, IARG_END);
        RTN_Close(rtn);
        statistics.num_mpi_instrum++;
    }
    else if (!distance(boost::algorithm::ifind_first(rtn_name, "MPI_Finalize").begin(), rtn_name.begin())
          && !distance(boost::algorithm::ifind_first(rtn_name, "_Wrapper").begin(), rtn_name.end())
          && inExtrae(rtn_addr))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)initMpi, IARG_CONTEXT,
                       IARG_THREAD_ID, IARG_BOOL, false, IARG_ADDRINT, rtn_id, IARG_END);
        RTN_Close(rtn);
        statistics.num_mpi_instrum++;
    }
    else if (!distance(boost::algorithm::ifind_first(rtn_name, "MPI_").begin(), rtn_name.begin())
         &&  !distance(boost::algorithm::ifind_first(rtn_name, "_Wrapper").begin(), rtn_name.end())
         &&  inExtrae(rtn_addr))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_BEFORE, AFUNPTR(boundaryMPI), IARG_CONTEXT,
                       IARG_THREAD_ID,IARG_BOOL, true,  IARG_ADDRINT, rtn_id,  IARG_END);
        RTN_InsertCall(rtn, IPOINT_AFTER,  AFUNPTR(boundaryMPI), IARG_CONTEXT,
                       IARG_THREAD_ID,IARG_BOOL, false, IARG_ADDRINT, rtn_id, IARG_END);
        RTN_Close(rtn);
        statistics.num_mpi_instrum++;
    }
    /*else if (rtn_name.find("MPI_Comm_rank") != string::npos
          && !inExtrae(rtn_addr))
    {
        RTN_Open(rtn);
        RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)getRank, IARG_CONTEXT,
                        IARG_THREAD_ID, IARG_FUNCARG_ENTRYPOINT_VALUE, 1, IARG_ADDRINT, rtn_id, IARG_END);
        RTN_Close(rtn);
        statistics.num_mpi_instrum++;
    }*/
}

VOID initMPIControl()
{
    RTN_AddInstrumentFunction(rtnCallback, 0);
}
