i686-pc-linux-gnu-gcc-4.0.2 -c -O3 -mtune=pentiumpro -fexpensive-optimizations -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -Wall -fPIC -ansi -pthread -m32 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DLINUX -DLINUX_X86 -DX86 -DEVIL_ENDIAN -D_GNU_SOURCE -D_REENTRANT -DSYSV -DSVR4 -Dinline=__inline__ -DHYPERTHREAD -DNV_CG -D__NO_CTYPE -D_FILE_OFFSET_BITS=64 -I. -I.  -I/usr/X11R6/include ./contourshade.c
i686-pc-linux-gnu-gcc-4.0.2 -c -O3 -mtune=pentiumpro -fexpensive-optimizations -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -Wall -fPIC -ansi -pthread -m32 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DLINUX -DLINUX_X86 -DX86 -DEVIL_ENDIAN -D_GNU_SOURCE -D_REENTRANT -DSYSV -DSVR4 -Dinline=__inline__ -DHYPERTHREAD -DNV_CG -D__NO_CTYPE -D_FILE_OFFSET_BITS=64 -I. -I.  -I/usr/X11R6/include ./outimgshade.c
i686-pc-linux-gnu-gcc-4.0.2 -c -O3 -mtune=pentiumpro -fexpensive-optimizations -finline-functions -funroll-loops -fomit-frame-pointer -frerun-cse-after-loop -fstrength-reduce -fforce-addr -Wall -fPIC -ansi -pthread -m32 -DQMC -DMI_MODULE= -DMI_PRODUCT_RAY -DLINUX -DLINUX_X86 -DX86 -DEVIL_ENDIAN -D_GNU_SOURCE -D_REENTRANT -DSYSV -DSVR4 -Dinline=__inline__ -DHYPERTHREAD -DNV_CG -D__NO_CTYPE -D_FILE_OFFSET_BITS=64 -I. -I.  -I/usr/X11R6/include ./outpsshade.c
i686-pc-linux-gnu-g++-4.0.2 -shared -export-dynamic -static-libgcc -Wl,-Bsymbolic,--whole-archive,--allow-shlib-undefined -o contour.so contourshade.o outimgshade.o outpsshade.o -Wl,--no-whole-archive
