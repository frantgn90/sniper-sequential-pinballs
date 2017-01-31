#ifndef PINPLAY_FUNCTS_HPP_INCLUDED
#define PINPLAY_FUNCTS_HPP_INCLUDED

std::string getBaseName();
VOID addRanktoBaseName(UINT32 rank);
VOID loggingAction ( THREADID thread_id, CONTEXT * ctxt, BOOL start, UINT32 rtn_id );
VOID newInstrCounterCheckpoint (THREADID thread_id, BOOL start, UINT32 rtn_id);
VOID FiniPinplay();
VOID initPinPlay(int argc, char ** argv);
bool isLogging();
string getLoggerBaseName();

#endif
