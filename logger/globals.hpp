#ifndef GLOBALS_HPP_INCLUDED
#define GLOBALS_HPP_INCLUDED

#include <dirent.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include "pin.H"

#define OFFSETS 0

extern bool is_first_bbl;

extern BOOL in_mpi;

// Indicates the rank of the MPI processes set that this instance of the
// pintool is being executed.
extern UINT32 mpi_rank;

// Indicates whether the app must be traced or not
// i.e. must not be traced when is in MPI communication
extern BOOL must_trace;

// Auxiliar for counting and enumering the BBLs
extern UINT32 bbl_count;

// Auxiliar for counting and enumering the potential pinballs
extern UINT64 pinball_count;

struct pinball_node_t {
    pinball_node_t * prev;
    pinball_node_t * next;
    UINT64 pinball_id;
    UINT64 instructions;
    UINT32 rank_id;
    std::vector<ADDRINT> bbls;
};

// Queue of pinballs
extern pinball_node_t * pinballs_queue_tail;

// Head of queue
extern pinball_node_t * pinballs_queue_head;

// Last MPI function that has been executed
extern UINT32 last_mpi_func;

struct extrae_image_t {
    ADDRINT low_addr;
    ADDRINT high_addr;
    BOOL loaded;
};

extern extrae_image_t extrae_image;
// Contains the mean of instructions per pinball
// that will be used as threshold at the peak recognition process
// Precission is not a problem
extern UINT64 instr_mean;

struct statistics_t
{
    UINT32 num_roi_markers;
    UINT32 num_mpi_instrum;
    UINT32 pinball_count;
} ;

extern statistics_t statistics;

#define KNOB_FAMILY "pintool-no-mpi-logger"
#define PREDEFINED_EXTRAE_EVENT_ID "6000019"
#define PINBALL_EVENT_FINI 0

extern KNOB_COMMENT pinplay_driver_knob_family;
extern KNOB <BOOL> KnobReplayer;
extern KNOB <BOOL> KnobLogger;

extern KNOB <BOOL> KnobRoiActivated;
extern KNOB <BOOL> KnobExtraeEvents;
extern KNOB <unsigned long> KnobExtraeEventsId;
extern KNOB <int>KnobVerboseLevel;

#endif // GLOBALS_HPP_INCLUDED
