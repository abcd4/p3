echo compiling grass/disksvr.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/disksvr.c
echo compiling grass/gatesvr.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/gatesvr.c
echo compiling grass/kcrt0.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/kcrt0.c
echo compiling grass/main.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/main.c
echo compiling grass/process.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/process.c
echo compiling grass/procsys.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/procsys.c
echo compiling grass/ramfilesvr.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/ramfilesvr.c
echo compiling grass/spawnsvr.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/spawnsvr.c
echo compiling grass/ttysvr.c
cc -c -DNO_UCONTEXT -DHW_MLFQ -DHW_MEASURE -DHW_PAGING /src/grass/ttysvr.c
echo linking
cc -o k.out -k kcrt0.o disksvr.o gatesvr.o main.o process.o procsys.o ramfilesvr.o spawnsvr.o ttysvr.o
