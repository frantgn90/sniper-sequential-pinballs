#ifndef EXTRAE_HPP_INCLUDED
#define EXTRAE_HPP_INCLUDED


VOID initExtraeControl();
VOID callExtrae_event(const CONTEXT * ctxt, THREADID threadid,
    unsigned long event, unsigned long value);

BOOL inExtrae(ADDRINT addr);

#endif
