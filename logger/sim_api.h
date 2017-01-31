#ifndef __SIM_API
#define __SIM_API

#define SIM_CMD_ROI_START       1
#define SIM_CMD_ROI_END         2


#if defined(__i386)
   #define MAGIC_REG_A "eax"
   #define MAGIC_REG_B "edx" // Required for -fPIC support
   #define MAGIC_REG_C "ecx"
#else
   #define MAGIC_REG_A "rax"
   #define MAGIC_REG_B "rbx"
   #define MAGIC_REG_C "rcx"
#endif


#define SimMagic0(cmd) ({                    \
   unsigned long _cmd = (cmd), _res;         \
   __asm__ __volatile__ (                    \
   "mov %1, %%" MAGIC_REG_A "\n"             \
   "\txchg %%bx, %%bx\n"                     \
   : "=a" (_res)           /* output    */   \
   : "g"(_cmd)             /* input     */   \
      );                   /* clobbered */   \
   _res;                                     \
})



#define PtRoiStart()             SimMagic0(SIM_CMD_ROI_START)
#define PtRoiEnd()               SimMagic0(SIM_CMD_ROI_END)

#endif /* __SIM_API */
