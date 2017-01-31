#ifndef __SIFT_PROXY_H
#define __SIFT_PROXY_H

#include <cstring>
#include <string>
#include <cstdlib>

#include <sys/stat.h>
//#include <sys/types.h>

//#include "sift_format.h"
#include "zfstream.h"
#include "itostr.h"

#include <iostream>
#include <fstream>

#include <stdlib.h>     /* getenv */

#define SIFT_OTHER_SIZE 6

int to_recorder;
int to_sniper = 0;
int from_sniper = 0;

// Indica el final del bucle response
bool end_communication;

std::string filename_from_sniper;
std::string filename_to_sniper;

std::string filename_from_recorder;
std::string filename_to_recorder;

std::string OutputFile = "trace"; // -o
uint64_t Blocksize = 0; // -b
uint64_t UseROI = 0; // -roi
uint64_t MPIImplicitROI = 0; // -roi-mpi
uint64_t FastForwardTarget = 0; //-f
uint64_t DetailedTarget = 0; // -d
uint64_t UseResponseFiles = 0; // -r
uint64_t EmulateSyscalls = 0; // -e
bool SendPhysicalAddresses = 0; //-pa
uint64_t FlowControl = 1000; //-flow
uint64_t FlowControlFF = 100000; //-flowff
int64_t SiftAppId = 0; //-s
bool RoutineTracing = 0; //-rtntrace
bool RoutineTracingOutsideDetailed = 0; //-rtntrace_outsidedetail
bool Debug = 0; //-debug
bool Verbose = 0; //-verbose
uint64_t StopAddress = 0; //-stop
uint64_t ExtraePreLoaded = 0; //-extrae
bool Replayer = 0; //-replay
std::string pinballs_proxy;

// struct para parsear argumentos al thread
struct thread_args_t{
    bool primero;
    bool ultimo;
};
#endif
