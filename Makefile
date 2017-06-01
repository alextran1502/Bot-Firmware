PART=TM4C1294NCPDT
DEBUG=1

ROOT=tiva-c

include ${ROOT}/makedefs

# Debug info that includes preprocessor defs
CFLAGS+=-g3

VPATH=${ROOT}/boards/ek-tm4c1294xl/drivers
VPATH+=${ROOT}/utils

IPATH=.
IPATH+=${ROOT}/boards/ek-tm4c1294xl
IPATH+=${ROOT}
IPATH+=${ROOT}/third_party

all: ${COMPILER}
all: ${COMPILER}/winchbot.axf

clean:
	@rm -rf ${COMPILER} ${wildcard *~}

flash: all
	lm4flash ${COMPILER}/winchbot.bin

${COMPILER}:
	@mkdir -p ${COMPILER}

${COMPILER}/winchbot.axf: ${COMPILER}/main.o
${COMPILER}/winchbot.axf: ${COMPILER}/pinout.o
${COMPILER}/winchbot.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/winchbot.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/winchbot.axf: ${ROOT}/utils/uartstdio.o
${COMPILER}/winchbot.axf: tm4c1294xl.ld
SCATTERgcc_winchbot=tm4c1294xl.ld
ENTRY_winchbot=ResetISR
CFLAGSgcc=-DTARGET_IS_TM4C129_RA0

ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif

