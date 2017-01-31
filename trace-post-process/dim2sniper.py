#! /usr/bin/env python
#!-*- encoding:utf8 -*-

import sys,os

''' tipos de records '''
CPU = 0
PIN_EVENT = 1


BURST_ID = "1"
EVENT_ID = "20"

DEFAULT_RECORD_ID = 10001
DEFAULT_PIN_EVENT_ID = 6000019

MPI_RECORD_ID = 50000003
MPI_INIT_RECORD_ID = 31
MPI_FINI_RECORD_ID = 32

EVENTS_TO_IGNORE = [42000050, 42000059, 42000000, 42000002, 42000008, 42000052, 42000052] # PAPI HWC

pin_event_id = None
record_id = None

from_time_us = 0
to_time_us = -1 # final

tmp_file = "tmp.txt"
        
class dimemas_record(object):
    
    def __init__(self,line):
        #try:
            self.value = -1
            self.event_type = -1
            
            self.raw = line
            splitted_line = line.split(":")
            self.dimemas_record_type = splitted_line[0]
            
            self.process_id = int(splitted_line[1])
            self.thread_id = int(splitted_line[2])
            
            if self.dimemas_record_type == BURST_ID:
                self.burst_duration = float(splitted_line[3])
                
            elif self.dimemas_record_type == EVENT_ID:
                self.event_type = int(splitted_line[3])
                self.value = int(splitted_line[4])
                
                if self.event_type != pin_event_id and self.event_type != MPI_RECORD_ID:
                    self.recognized = False
            else:
                self.recognized = False
            
            self.recognized = True
        #except Exception:
        #    self.recognized = False
    
     
class translator(object):
    ''' el patron son 3 rafags cpu 2 eventos pin entrelazados '''
    ''' retorno -1: no cumple el patron; 
                -2: no cumple el patron y hay que hacer flush del buffer
                 0: ok
                 1: traduccion disponible
    '''
    _pattern = []
    
    def new_record(self, dimemas_record):
        _record = dimemas_record
        
        if len(self._pattern) == 0:
            if _record.dimemas_record_type != BURST_ID:
                ''' segun el patron, ha de empezar con una rafaga de cpu '''
                return -1
            else:
                self._pattern.append(_record)
        elif self._pattern[-1].dimemas_record_type == _record.dimemas_record_type:
            ''' no se cumple el patron hemos de escribir lo que ha entrado '''
            return -2
        elif _record.dimemas_record_type == BURST_ID or \
            (_record.dimemas_record_type == EVENT_ID and _record.event_type == pin_event_id):
            ''' se cumple el patron '''
            self._pattern.append(_record)
        else:
            return -2
        
        if len(self._pattern) == 5:
            return 1
            
        return 0
            
    def translation(self):
        if len(self._pattern) == 5:
            line = "{0}:{1}:{2}:{3}\n"\
                .format(record_id, self._pattern[1].process_id, self._pattern[1].thread_id,\
                    self._pattern[1].value)
            del self._pattern[:]
            return line
        else:
            return None
        
    def records_to_flush(self):
        var = [ r.raw for r in self._pattern ]
        del self._pattern[:]
        return var
        
        
''' ----------------------------------------------------- '''


def Usage():
    print
    print "Usage(): prv2dim+sniper "
    print "     prv2dim+sniper <input.dim> <output.dim> [--event-id default:6000019] " \
        "[--record-id default:10001] "\
        "[-no-zero-out-mpi]"
    #    "[--from [begin](ns)] [--to [final](ns)]"
    print 
    

def main(argc, argv):
    if argc < 3:
        Usage()
        exit(-1)
        
    infile = argv[1]
    outfile = argv[2]
    
    global pin_event_id, record_id
    pin_event_id = DEFAULT_PIN_EVENT_ID
    record_id = DEFAULT_RECORD_ID
    zero_out_mpi = True
    ''' Params '''
    
    if "--record-id" in argv: record_id = argv[argv.index("--record-id") + 1]
    if "--event-id" in argv: pin_event_id = argv[argv.index("--event-id") + 1]
    if "--from" in argv: from_time_us = argv[argv.index("--from") + 1]
    if "--to" in argv: to_time_us = argv[argv.index("--to") + 1]
    if "-no-zero-out-mpi" in argv: zero_out_mpi=False
        
    '''Final params'''
        
    ''' Init files '''
    
    file_in = open(infile, "r")
    file_out = open(outfile, "w")
    
    header_blank_line_size = 100
    file_out.write(" "*header_blank_line_size + "\n") #blank line for header
    header_blank_line_size += 1
    
    print "+ Reading input file..."
    header = file_in.readline().split(":")
    offset = int(header[2].split(",")[1])
    num_procs = int(header[3].split("(")[0])
    
    ''' Reading file '''
    
    offsets_info = [0]*num_procs
    pinballs_num = [0]*num_procs
    bursts_num = [0]*num_procs
    in_gmpi = [False]*num_procs
    
    offset_offsets = 0
    
    ''' Cada proceso ha de tener un traductor propio '''
    translators_set = [translator()]*num_procs
    
    for line in file_in:
        line_object = dimemas_record(line)
        process_id = line_object.process_id
            
        ''' Los offsets no los tratamos, generamos nuevos '''
        if line_object.dimemas_record_type == "s": break
        
        ''' Definicion de objetos, se queda tal y como esta '''
        if line_object.dimemas_record_type == "d": file_out.write(line); continue  
        if int(line_object.event_type) in EVENTS_TO_IGNORE: continue
        if line_object.dimemas_record_type == BURST_ID: bursts_num[process_id] += 1
        
        ''' Primera ocurrencia de una record de este proceso '''
        
        if offsets_info[process_id] == 0:
            offsets_info[process_id] = file_out.tell()
            
        if zero_out_mpi:
            ''' si todavia no hemos entrado a GMPI, copiamos los elementos tal cual
                menos los burst que pasan a tener una duración de 0s. Lo mismo si
                ya hemos salido '''
            if not in_gmpi[process_id]:
                if line_object.dimemas_record_type == BURST_ID:
                    new_burst = ":".join(line.split(":")[:3]) + ":0.000000000\n"
                    file_out.write(new_burst)
                else:
                    file_out.write(line)
                    
                in_gmpi[process_id] = (line_object.event_type == MPI_RECORD_ID and line_object.value == MPI_INIT_RECORD_ID)
                continue
                
            elif line_object.event_type == MPI_RECORD_ID and line_object.value == MPI_FINI_RECORD_ID:
                in_gmpi[process_id] = False
                file_out.write(line)
                continue
            
        ''' Comprobamos si se cumple el patron '''
        state = translators_set[process_id].new_record(line_object)
        if state == -1:
            file_out.write(line)
        elif state == -2:
            for rf in translators_set[process_id].records_to_flush():
                file_out.write(rf)
                
            file_out.write(line)
        elif state == 1:
            pinballs_num[process_id] += 1
            file_out.write(translators_set[process_id].translation())
        

    ''' flush all lines in translator buffer '''
    for tr in translators_set:
        for l in tr.records_to_flush():
            file_out.write(l)
            
    ''' generating header '''
    print "+ Generating new header..."
    header = header[0] + ":\"" + outfile + "\":1," + str(file_out.tell()) + ":"\
        + header[3][:-1]

    ''' Write offsets '''
    for i in range(0, len(offsets_info)):
        file_out.write("s:{0}:{1}\n".format(i, offsets_info[i]))

    file_out.seek(0,0)
    file_out.write(header)


#    file_in.close()
#    file_out.close()

#    ''' Rewritting the file with aim to insert the header at the beggining '''
#    file_out = open(tmp_file, "r")
#    ofile = open(outfile, "w")
#    ofile.write(header)

#    print "+ Rewrite file from tmp file..."
#    for line in file_out: ofile.write(line)
    
#    file_out.close()
#    ofile.close()

#    os.remove(tmp_file)

    ''' Some statistics '''
    print 
    print "[STATISTICS]"
    print "* Total burst per process:"
    print "    " + " ".join(map(str, bursts_num))
    print "* Total pinballs per process:"
    print "    " + " ".join(map(str, pinballs_num))
    print "[END PROCESS SUCCESSFULY]"
    print
    
    return 0

if __name__ == "__main__":
    exit(main(len(sys.argv), sys.argv))