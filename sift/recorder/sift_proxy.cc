#include "sift_proxy.h"
#include <vector>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>    /* For O_RDWR */
#include <sys/stat.h>
#include "sift_format.h"
#include <signal.h> /* For pthread_kill */
#include <errno.h>
#include <sys/wait.h> /* For waitpid */

#define VERBOSE 1
#define CHECKPOINTED 1

void run_sift_recorder(std::string pinball, int argc, char * argv[])
{
    std::ostringstream cmd;
    std::ostringstream pin_options;
    std::ostringstream pinball_options;
    std::string pin_home (getenv("PIN_ROOT"));
    std::string sniper_home (getenv("SNIPER_ROOT"));


    pin_options << "-mt -injection child -xyzzy -enable_vsm 0"
        << " -xyzzy -reserve_memory " << pinball <<".address"; // -pinplay:msgfile "%s" -replay:resultfile "%s"

    pinball_options << " -replay -replay:basename " << pinball;

    cmd << pin_home << "/intel64/bin/pinbin " << pin_options.str() << " -t "
        << sniper_home << "/sift/recorder/sift_recorder " << pinball_options.str();

    for(int i=1; i<argc; ++i)
    {
        if (!strcmp(argv[i], "-o"))
            cmd << " -o " << argv[++i] << "_proxy";
        else if (!strcmp(argv[i], "--pinball-proxy"))
            ++i;
        else
            cmd << " " << argv[i];
    }

    cmd << " " << pin_home << "/extras/pinplay/bin/intel64/nullapp";

    // -- execvp

    std::vector<char *> cmd_new;
    char * token;
    char * mutable_cmd = strdup(cmd.str().c_str());

    token = strtok(mutable_cmd, " ");
    while(token != NULL)
    {
        cmd_new.push_back(token);
        token = strtok(NULL, " ");
    }
    cmd_new.push_back(NULL);

    int status = execvp(cmd_new[0], &cmd_new[0]);
    std::cerr << "Error in execvp (" << strerror(errno) << ")" << std::endl;
    exit(1);
}

int abre_pipe(std::string filename, int oflags)
{
    struct stat stat_file;
    while (stat(filename.c_str(), &stat_file) < 0)
    {
        #if VERBOSE > 2
        std::cerr << filename << " no existe todavía. Esperando... " << std::endl;
        #endif
        sleep(2);
    }
    #if VERBOSE > 2
    std::cerr << "Opening pipe " << filename << std::endl;
    #endif
    int fd = open(filename.c_str(), oflags);
    #if VERBOSE > 2
    std::cerr << filename << " opened." << std::endl;
    #endif // VERBOSE

    return fd;
}

void print_to_hex(char * str, int size)
{
    for(int i=0; i<size; ++i)
    {
        std::cout << "[0x" << std::hex << (unsigned short)str[i] << "]";
    }
}

void * _comunica_directo(void * args)
{
    struct thread_args_t * my_args = (struct thread_args_t*)args;

    int from_recorder;

    from_recorder = abre_pipe(filename_from_recorder, O_RDONLY);
    if (to_sniper == 0)
        to_sniper = abre_pipe(filename_to_sniper, O_WRONLY);

    // header

    char buffer[16];
    read(from_recorder, &buffer, sizeof(buffer));
    if (my_args->primero)
    {
        write(to_sniper, &buffer, sizeof(buffer));
    }


    // data
    char large_buffer[SIFT_OTHER_SIZE];
    char little_buffer;

    uint64_t contador = 0;
    bool seen_end = false;

    while(!seen_end)
    {
        int readed = read(from_recorder, &little_buffer, sizeof(little_buffer));
atras:
        if (contador == sizeof(large_buffer))
            write(to_sniper, &large_buffer[0], 1);
        else
            contador++;

        for(int i=1; i < sizeof(large_buffer); ++i)
        {
            large_buffer[i-1] = large_buffer[i];
        }
        large_buffer[sizeof(large_buffer)-1] = little_buffer;

        Sift::Record * rr;
        rr = reinterpret_cast<Sift::Record *>(&large_buffer);

        if (rr->Other.zero == 0
            && rr->Other.type == Sift::RecOtherEnd
            && rr->Other.size == 0)
        {
            #if VERBOSE > 2
            std::cout << "[SIFT_PROXY] FINAL sift_recorder -> sniper ~ "; print_to_hex(large_buffer, sizeof(large_buffer));
            #endif

            if (read(from_recorder, &little_buffer, sizeof(little_buffer)) > 0) // falso final... chapucerio
            {
                #if VERBOSE > 2
                std::cout << " = FALSO FINAL" << std::endl;
                #endif // VERBOSE
                goto atras;
            }

            #if CHECKPOINTED > 0
            Sift::Record checkpoint;
            checkpoint.Other.zero = 0;
            checkpoint.Other.type = Sift::RecOtherCheckpoint;
            checkpoint.Other.size = 0;

            write(to_sniper, &checkpoint, sizeof(checkpoint.Other));
            #endif // CHECKPOINTED


            if (my_args->ultimo)
            {
                for(int i=0; i < sizeof(large_buffer); ++i) // flush del buffer
                {
                    write(to_sniper, &large_buffer[i], 1);
                }
                #if VERBOSE > 2
                std::cout << " = SENDED" << std::endl;
                #endif // VERBOSE
            }
            else
            {
                #if VERBOSE > 2
                std::cout << " = IGNORED" << std::endl;
                #endif // VERBOSE
            }


            close(from_recorder); // En este instante el recorder ya ha cerrado su pipe
            seen_end = true;
        }
        else if (rr->Other.zero == 0
            && rr->Other.type == Sift::RecOtherSync
            && rr->Other.size == 0)
        {
            #if VERBOSE > 1
            std::cout << " --> SINCRONIZACION sift_recorder -> sniper ~ "; print_to_hex(large_buffer, sizeof(large_buffer)); std::cout << std::endl;
            #endif
            for(int i=0; i < sizeof(large_buffer); ++i) // flush del buffer
            {
                write(to_sniper, &large_buffer[i], 1);
            }
            contador = 0;
        }
    }
    pthread_exit(NULL);
}

void __comunica_response_sigterm_handler(int signal)
{
    end_communication = true;
}

void * _comunica_response(void * args)
{
    struct thread_args_t * my_args = (struct thread_args_t*)args;

    signal(SIGTERM, __comunica_response_sigterm_handler);

    // Al abrir esta fifo recorder espera una orden especial desde sniper (Sync)
    to_recorder = abre_pipe(filename_to_recorder, O_WRONLY );
    // Sniper solo la abrira cuando la necesite, que en este caso es cuando haga el primer Sync
    // estamos bloqueados hasta entonces
    if (from_sniper == 0)
        // Ha de ser O_NONBLOCK para poder salir del bucle al recibir el SIGTERM
        from_sniper = abre_pipe(filename_from_sniper, O_RDONLY | O_NONBLOCK);


    end_communication = false;
    char large_buffer[1];

    while(!end_communication)
    {
        while(read(from_sniper, &large_buffer, sizeof(large_buffer)) <= 0)
        {
            if(end_communication) break;
        }
        write(to_recorder, &large_buffer, sizeof(large_buffer));
    }

    close(to_recorder);
    pthread_exit(NULL);
}

void comunica_fifos(bool primero, bool ultimo)
{
    // necesito dos threads, cada uno se encarga de una dirección
    pthread_t th_d;
    pthread_t th_r;

    struct thread_args_t thargs;
    thargs.primero = primero;
    thargs.ultimo = ultimo;

    pthread_create(&th_d, NULL, _comunica_directo, (void *)&thargs);
    pthread_create(&th_r, NULL, _comunica_response,(void *)&thargs);

    int j_th_d = pthread_join(th_d, NULL);

    #if VERBOSE > 2
    if (j_th_d != 0)
    {
        std::cerr << "[SIFT_PROXY] Thread th_d .. FAIL" << std::endl;
    }
    else
    {
        std::cerr << "[SIFT_PROXY] Thread join th_d .. OK" << std::endl;
    }
    #endif // VERBOSE

    // significa que se ha llegado al final de la pipe. hay que ordenar al thread response
    // que termine
    pthread_kill(th_r, SIGTERM);
    int j_th_r = pthread_join(th_r, NULL);
    #if VERBOSE > 2
    if (j_th_r != 0)
    {
        std::cerr << "[SIFT_PROXY] Thread join th_r .. FAIL" << std::endl;
    }
    else
    {
        std::cerr << "[SIFT_PROXY] Thread join th_r .. OK" << std::endl;
    }
    #endif // VERBOSE
}

int main(int argc, char ** argv)
{
    // paso de parametros
    for(int i=0; i<argc; ++i)
    {
        if(!strcmp(argv[i],"-o"))
        {
            OutputFile = strdup(argv[++i]);
        }
        else if(!strcmp(argv[i],"-s"))
        {
            SiftAppId = atoi(argv[i+1]);
        }
        else if(!strcmp(argv[i],"--pinball-proxy"))
        {
            pinballs_proxy = strdup(argv[i+1]);
        }
    }

    std::ostringstream ss;

    ss << OutputFile <<  "_response.app" << itostr(SiftAppId) << ".th0.sift";
    filename_from_sniper = ss.str(); ss.str(""); ss.clear();
    ss << OutputFile << ".app" << itostr(SiftAppId) << ".th0.sift";
    filename_to_sniper = ss.str(); ss.str(""); ss.clear();

    ss << OutputFile << "_proxy.app" << itostr(SiftAppId) << ".th0.sift";
    filename_from_recorder = ss.str(); ss.str(""); ss.clear();
    ss << OutputFile << "_proxy_response.app" << itostr(SiftAppId) << ".th0.sift";
    filename_to_recorder = ss.str(); ss.str(""); ss.clear();

    // recorremos el fichero de pinballs y lanzamos sift_record
    ss << pinballs_proxy << ".pprox";

    struct stat stat_buf;
    int rc = stat(ss.str().c_str(), &stat_buf);
    long file_size = stat_buf.st_size;

    int nline = 0;
    std::ifstream pb_file(ss.str().c_str());
    for( std::string line; getline( pb_file, line ); ++nline)
    {
        pid_t pid = fork();
        if (pid == 0) // child
        {
            if (nline == 0) // creamos las fifo que conectarán con sift_recorder
            {
                mkfifo(filename_from_recorder.c_str(), 0600);
                mkfifo(filename_to_recorder.c_str(), 0600);
            }

            #if VERBOSE > 0
            std::cout << "[SIFT_PROXY] Ejecutando sift_recorder (" << line << ")" << std::endl;
            #endif // VERBOSE
            run_sift_recorder(line, argc, argv);
        }
        else if (pid > 0) // parent
        {
            comunica_fifos((nline==0), pb_file.tellg() == file_size);

            int exit_status;
            waitpid(pid, &exit_status, 0);
            int status = WEXITSTATUS(exit_status);
            if (status != 0) // ha habido un error
            {
                std::cerr << "[SIFT_PROXY] Sift_recorder ha terminado con error (" << status << ")" << std::endl;
                exit(1);
            }
            #if VERBOSE > 1
            else if (status == 0) // todo bien
            {
                std::cout << "[SIFT_PROXY] Sift_recorder ends with code (" << status << ")" << std::endl;
            }
            #endif // VERBOSE

        }
        else // error on fork
        {
            std::cerr << "Error en fork()";
            exit(1);
        }
    }
    //sleep(5);

    close(from_sniper);
    close(to_sniper);
    #if VERBOSE > 0
    std::cout << "[SIFT_PROXY] Exit" << std::endl;
    #endif // VERBOSE
    return 0;
}
