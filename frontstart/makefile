FLAGS:=-I ../common/include				\
	   -I ./include 					\
	   -Wshadow							\
	   -Winit-self						\
	   -Wredundant-decls				\
	   -Wcast-align						\
	   -Wundef							\
	   -Wfloat-equal					\
	   -Winline 						\
	   -Wunreachable-code 				\
	   -Wmissing-declarations 			\
	   -Wmissing-include-dirs 			\
	   -Wswitch-enum 					\
	   -Wswitch-default 				\
	   -Weffc++ 						\
	   -Wmain 							\
	   -Wextra 							\
	   -Wall 							\
	   -g -pipe -fexceptions 			\
	   -Wcast-qual 						\
	   -Wconversion 					\
	   -Wctor-dtor-privacy 				\
	   -Wempty-body 					\
	   -Wformat-security 				\
	   -Wformat=2 						\
	   -Wignored-qualifiers 			\
	   -Wlong-long						\
	   -Wno-missing-field-initializers 	\
	   -Wnon-virtual-dtor 				\
	   -Woverloaded-virtual 			\
	   -Wpointer-arith 					\
	   -Wsign-promo 					\
	   -Wstack-protector 				\
	   -Wstrict-aliasing				\
	   -Wtype-limits					\
	   -Wwrite-strings 					\
	   -Werror=vla 						\
	   -D_DEBUG 						\
	   -D_EJUDGE_CLIENT_SIDE
BINDIR:=bin
OUTPUT:=../frontstart.out
SRCDIR:=source
SOURCE:=$(wildcard ${SRCDIR}/*.cpp)
OBJECTS:=$(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SOURCE}))))
LINKED:=$(addsuffix .o,$(addprefix ../common/${BINDIR}/,$(basename $(notdir $(wildcard ../common/source/*)))))
LOGS:=../logs

all: ${OUTPUT}

${OUTPUT}:${OBJECTS} ${LOGS}
	g++ ${FLAGS} ${OBJECTS} ${LINKED} -o ${OUTPUT}
${OBJECTS}: ${SOURCE} ${BINDIR}
	$(foreach SRC,${SOURCE},$(shell g++ -c ${SRC} ${FLAGS} -o $(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SRC}))))))
clean:
	$(foreach OBJ,${OBJECTS}, $(shell rm ${OBJ}))
	rm ${OUTPUT}
	rmdir ${BINDIR}
${SOURCE}:

${BINDIR}:
	mkdir ${BINDIR}
${LOGS}:
	mkdir ${LOGS}
	mkdir ${LOGS}/img
	mkdir ${LOGS}/dot
