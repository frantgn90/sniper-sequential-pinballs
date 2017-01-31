#include <fstream>
#include <iostream>
#include <vector>
#include <stdlib.h>

#include "globals.hpp"
#include "extrae-control.h"

#include "dyn-swindow.h"
#include "pinplay-functions.h"

#include "pinplay.H"

vector<unsigned long> pinballs_instr_count;

bool is_first_bbl=false;

ADDRINT main_image_low;
ADDRINT main_image_high;

ofstream first_addr_outfile;

vector<unsigned int> first_bbl_addr;

VOID addPinballInstrCount(unsigned int ninstr)
{
    pinballs_instr_count.push_back(ninstr);
}

VOID generatePDSWFile(std::string filename)
{
    std::vector<UINT64> dsw_offsets;
    UINT32 counter = 0;

    for (unsigned int i = 0; i < pinballs_instr_count.size(); ++i)
    {
        counter++;
        if (pinballs_instr_count[i] > TEMP_PEAK_BOUNDARY && pinballs_instr_count[i] > MIN_SIZE_SLICING_WINDOWS)
        {
            dsw_offsets.push_back(i);
            counter = 0;
        }
    }

    ofstream dsw_file;
    dsw_file.open(filename.c_str());

    UINT64 low_boundary = 0;
    for(std::vector<UINT64>::iterator it = dsw_offsets.begin(); it != dsw_offsets.end(); ++it)
    {
        dsw_file  << low_boundary << ":" << *it <<"\n";
        low_boundary = *it;
    }
    dsw_file << low_boundary << ":" << statistics.pinball_count << "\n";
    dsw_file.close();
}

VOID FiniDSW()
{
    first_addr_outfile.close();
    generatePDSWFile(getLoggerBaseName().append(".pdsw").c_str());
}

VOID FirstbblAnalysis(ADDRINT addr)
{
    // Data gathered for first instr. address analysis
    if (is_first_bbl)
    {
        first_addr_outfile << addr << "\n";
        is_first_bbl = false;
    }
}

VOID traceCallback(TRACE trace, VOID * v)
{
    for( BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl) )
    {
        ADDRINT bbl_addr = BBL_Address(bbl);

        if (   bbl_addr >= main_image_low
            && bbl_addr < main_image_high)
        {
            BBL_InsertCall(bbl, IPOINT_BEFORE, AFUNPTR(FirstbblAnalysis), IARG_ADDRINT,
                bbl_addr, IARG_END);
        }
    }
}

VOID imgDSWCallback(IMG img, VOID * v)
{
    if (IMG_IsMainExecutable(img))
    {
        main_image_low = IMG_LowAddress(img);
        main_image_high = IMG_HighAddress(img);
    }
}

VOID initDynSlidingWindows()
{
    IMG_AddInstrumentFunction(imgDSWCallback, 0);
    TRACE_AddInstrumentFunction(traceCallback, 0);

    first_addr_outfile.open (getLoggerBaseName().append(".fpia").c_str()
        , ios::out | ios::app);
}
