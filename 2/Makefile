LIBS = -lpthread
OBJS = dsdv.o dsdvmsgmgr.o
dsdv: ${OBJS}
	gcc -o dsdv ${OBJS} ${LIBS}
clean:
	rm -f dsdv ${OBJS}
