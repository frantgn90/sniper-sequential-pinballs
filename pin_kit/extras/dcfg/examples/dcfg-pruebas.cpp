/*BEGIN_LEGAL 
BSD License 

Copyright (c) 2015 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

// Simple example program to read a DCFG and print some statistics.

#include "dcfg_api.H"
#include "dcfg_trace_api.H"

#include <stdlib.h>
#include <assert.h>
#include <iomanip>

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <algorithm>

using namespace std;
using namespace dcfg_api;
using namespace dcfg_trace_api;

void nodosConMasEntradas(DCFG_DATA_CPTR dcfg, string tracefile, vector<pair<DCFG_ID, UINT64> > * bbl_nex)
{
    // Cada DCFG contiene info. de solo 1 proceso/thread
    DCFG_ID_VECTOR proc_ids;
    dcfg->get_process_ids(proc_ids);
    DCFG_PROCESS_CPTR pinfo = dcfg->get_process_info(proc_ids[0]);
    
    // Images.
    DCFG_ID_VECTOR image_ids;
    pinfo->get_image_ids(image_ids);
    
    // Solo me interesa la imagen principal
    DCFG_IMAGE_CPTR iinfo = pinfo->get_image_info(image_ids[0]);
    assert(iinfo);
    
    DCFG_ID_VECTOR bb_ids;
    iinfo->get_basic_block_ids(bb_ids);
    
    // Recorremos todos los BBLs
    for (size_t bi = 0; bi < bb_ids.size(); bi++) {
	if (pinfo->is_special_node(bb_ids[bi])) // No es un BBL, puede ser el nodo start, end o unknown
	    continue;
	
	DCFG_BASIC_BLOCK_CPTR bbinfo = pinfo->get_basic_block_info(bb_ids[bi]);
	assert(bbinfo);
	
	DCFG_ID bbl_id = bb_ids[bi];
	UINT64 num_exec = bbinfo->get_exec_count();
	
	bbl_nex->push_back(make_pair(bbl_id, num_exec));
     }
}

int main(int argc, char* argv[]) {

    // First argument should be a DCFG file.
    string filename = argv[1];

    // Make a new DCFG object.
    DCFG_DATA* dcfg = DCFG_DATA::new_dcfg();
    
    string errMsg;
    if (!dcfg->read(filename, errMsg)) {
        cerr << "error: " << errMsg << endl;
        return 1;
    }
    
    vector<pair<DCFG_ID, UINT64> > bbl_nex;
    
    //string tracefile = argv[2];
    string tracefile;
    nodosConMasEntradas(dcfg, tracefile, &bbl_nex);
    
    sort(bbl_nex.begin(), bbl_nex.end(), 
	 boost::bind(&pair<DCFG_ID, UINT64>::second, _1) < boost::bind(&pair<DCFG_ID, UINT64>::second, _2));
    reverse(bbl_nex.begin(), bbl_nex.end());
    
    for (int i=0; i < min(bbl_nex.size(), (UINT64)5); ++i)
    {
      cout << bbl_nex[i].first << ":" << bbl_nex[i].second << endl;
    }
    
    // free memory.
    delete dcfg;

    return 0;
}

