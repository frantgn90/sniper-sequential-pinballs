#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "globals.hpp"
#include "aux-functions.h"
#include "pinplay-functions.h"
#include "dyn-swindow.h"

#include "control_manager.H"
#include "pinplay.H"
#include "instlib.H"

using namespace INSTLIB;
using namespace CONTROLLER;
using namespace std;

PINPLAY_ENGINE pinplay_engine;
CONTROL_MANAGER * control;

#if OFFSETS>0
string outdir;
#endif

ofstream offsets_outfile;

string getLoggerBaseName()
{
    return pinplay_engine.LoggerGetBaseName();
}

bool isLogging()
{
    return pinplay_engine.IsLoggerRecording();
}

string getBaseName()
{
    return pinplay_engine.LoggerGetBaseName();
}

VOID addRanktoBaseName(UINT32 rank)
{
    /*  Este snipet de código cambia el basename de las pinballs para añadirle
     * el rank_id. El problema es que las pinballs generadas antes del
     * MPI_Comm_get_rank tienen el basename antiguo */

  std::stringstream ss;
  string actual_name = pinplay_engine.LoggerGetBaseName();

  ss << actual_name << "_rank_" << rank;
  pinplay_engine.LoggerSetBaseName(ss.str());

  std::cout << ss.str() << std::endl;

    /*  Ahora se genera el fichero pidtorank.txt que guarda la relación
     *        PID > RANK_ID */
/*    ofstream rank_file;
    rank_file.open (pinplay_engine.LoggerGetBaseName().append(".mpirank").c_str()
        , ios::out | ios::app);
    rank_file << getpid() << ">" << rank << "\n";
    rank_file.close();
*/
}

VOID loggingAction ( THREADID thread_id, CONTEXT * ctxt, BOOL start, UINT32 rtn_id )
{
    assert(pinplay_engine.IsLoggerActive());

    if (!in_mpi)
    {
        /*  Es posible que alguna llamada de Extrae llegue aquí sin haber alcanzado
         *  aún MPI_Init, como por ejemplo MPI_Distribute_XML_File. Para estos casos
         *  simplemente retorno sin hacer caso */
        return;
    }

    VOID * ip = (VOID *)PIN_GetContextReg(ctxt, REG_INST_PTR);

    if (start)
    {
        is_first_bbl = true;
        assert(!pinplay_engine.IsLoggerRecording());
        control->Fire (EVENT_START, ctxt, ip, thread_id, false);
    }
    else
    {
        assert(pinplay_engine.IsLoggerRecording());
        newInstrCounterCheckpoint(thread_id, start, rtn_id);
        control->Fire (EVENT_STOP, ctxt, ip, thread_id, false);
    }
}

//#if OFFSETS>0
VOID newInstrCounterCheckpoint (THREADID thread_id, BOOL start, UINT32 rtn_id)
{
    if (!in_mpi)
    {
        /*  Es posible que alguna llamada de Extrae llegue aquí sin haber alcanzado
         *  aún MPI_Init, como por ejemplo MPI_Distribute_XML_File. Para estos casos
         *  simplemente retorno sin hacer caso */
        return;
    }

    #if OFFSETS>0
    if (start)
    {
        offsets_outfile << ">" << pinplay_engine.LoggerGetICount(thread_id)
            << " # " << rtn_id << "\n";
    }
    else
    {
        offsets_outfile << "<" << pinplay_engine.LoggerGetICount(thread_id)
            << " # " << rtn_id << "\n";
    }
    #else
    offsets_outfile << pinplay_engine.LoggerGetICount(thread_id) << "\n";
    addPinballInstrCount(pinplay_engine.LoggerGetICount(thread_id));
    #endif // OFFSETS
    offsets_outfile.flush();
}
//#endif // OFFSETS

VOID FiniPinplay()
{
    #if OFFSETS>0
    offsets_outfile.close();
    #endif
}

VOID initPinPlay(int argc, char ** argv)
{
    pinplay_engine.Activate (argc, argv, true, false);
    control = pinplay_engine.LoggerGetController();

    /*  Creo el fichero de offsets */
    #if OFFSETS>0
    offsets_outfile.open (pinplay_engine.LoggerGetBaseName().append(".offsets").c_str()
        , ios::out | ios::app);
    #else
    offsets_outfile.open (pinplay_engine.LoggerGetBaseName().append(".instr_count").c_str()
        , ios::out | ios::app);
    #endif
}
