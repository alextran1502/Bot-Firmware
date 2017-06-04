PART=TM4C1294NCPDT
DEBUG=1

ROOT=tiva-c

include ${ROOT}/makedefs

CFLAGS+=-g3

VPATH=${ROOT}/boards/ek-tm4c1294xl/drivers
VPATH+=${ROOT}/third_party/lwip-1.4.1/apps/httpserver_raw
VPATH+=${ROOT}/utils
VPATH+=src

IPATH=src
IPATH+=${ROOT}/boards/ek-tm4c1294xl
IPATH+=${ROOT}
IPATH+=${ROOT}/third_party
IPATH+=${ROOT}/third_party/lwip-1.4.1/apps
IPATH+=${ROOT}/third_party/lwip-1.4.1/ports/tiva-tm4c129/include
IPATH+=${ROOT}/third_party/lwip-1.4.1/src/include
IPATH+=${ROOT}/third_party/lwip-1.4.1/src/include/ipv4

all: ${COMPILER}
all: ${COMPILER}/tucoflyer.axf

clean:
	@rm -rf ${COMPILER}

${COMPILER}:
	@mkdir -p ${COMPILER}

${COMPILER}/tucoflyer.axf: ${COMPILER}/main.o
${COMPILER}/tucoflyer.axf: ${COMPILER}/pinout.o
${COMPILER}/tucoflyer.axf: ${COMPILER}/vectors.o
${COMPILER}/tucoflyer.axf: ${COMPILER}/lwiplib.o
${COMPILER}/tucoflyer.axf: ${COMPILER}/uartstdio.o
${COMPILER}/tucoflyer.axf: ${ROOT}/driverlib/gcc/libdriver.a
${COMPILER}/tucoflyer.axf: src/tm4c1294xl.ld
SCATTERgcc_tucoflyer=src/tm4c1294xl.ld
ENTRY_tucoflyer=startup

CFLAGSgcc=-DTARGET_IS_TM4C129_RA0 -DUART_BUFFERED

ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
