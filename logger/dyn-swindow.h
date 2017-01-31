#ifndef DSW_HPP_INCLUDED
#define DSW_HPP_INCLUDED

#define MIN_SIZE_SLICING_WINDOWS 10
//#define TEMP_PEAK_BOUNDARY 200000000000000
#define TEMP_PEAK_BOUNDARY 130000

VOID addPinballInstrCount(unsigned int ninstr);
VOID initDynSlidingWindows();
VOID FiniDSW();

#endif
