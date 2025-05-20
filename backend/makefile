CXXFLAGS:=									\
-O2											\
-I include									\
-I ../common/include						\
-D _DEBUG									\
-ggdb3										\
-std=c++17									\
-Wall										\
-Wextra										\
-Weffc++									\
-Waggressive-loop-optimizations				\
-Wc++14-compat								\
-Wmissing-declarations						\
-Wcast-align								\
-Wcast-qual									\
-Wchar-subscripts							\
-Wconditionally-supported					\
-Wconversion								\
-Wctor-dtor-privacy							\
-Wempty-body								\
-Wfloat-equal								\
-Wformat-nonliteral							\
-Wformat-security							\
-Wformat-signedness							\
-Winline									\
-Wlogical-op								\
-Wnon-virtual-dtor							\
-Wopenmp-simd								\
-Woverloaded-virtual						\
-Wpacked									\
-Wpointer-arith								\
-Winit-self									\
-Wredundant-decls							\
-Wshadow									\
-Wsign-conversion							\
-Wsign-promo								\
-Wstrict-null-sentinel						\
-Wstrict-overflow=2							\
-Wsuggest-attribute=noreturn				\
-Wsuggest-final-methods						\
-Wsuggest-final-types						\
-Wsuggest-override							\
-Wswitch-default							\
-Wswitch-enum								\
-Wsync-nand									\
-Wundef										\
-Wunreachable-code							\
-Wunused									\
-Wuseless-cast								\
-Wvariadic-macros							\
-Wno-literal-suffix							\
-Wno-missing-field-initializers				\
-Wno-narrowing								\
-Wno-old-style-cast							\
-Wno-varargs								\
-Wstack-protector							\
-fcheck-new									\
-fsized-deallocation						\
-fstack-protector							\
-fstrict-overflow							\
-flto-odr-type-merging						\
-fno-omit-frame-pointer						\
-Wlarger-than=8192							\
-Wstack-usage=8192							\
-march=native 								\
-Werror=vla									\

BINDIR:=bin
OUTPUT:=backend
SRCDIR:=source
SOURCE:=$(wildcard ${SRCDIR}/*.cpp)
OBJECTS:=$(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SOURCE}))))
LINKED:=$(addsuffix .o,$(addprefix ../common/${BINDIR}/,$(basename $(notdir $(wildcard ../common/source/*)))))
LOGS:=../logs
OUTPUT_DIR:=../bin

all: ${OUTPUT}

${OUTPUT}:${OBJECTS} ${LOGS}
	mkdir -p ${OUTPUT_DIR}
	g++ ${CXXFLAGS} ${OBJECTS} ${LINKED} -o ${OUTPUT_DIR}/${OUTPUT}
${OBJECTS}: ${SOURCE} ${BINDIR}
	$(foreach SRC,${SOURCE},$(shell g++ -c ${SRC} ${CXXFLAGS} -o $(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SRC}))))))
clean:
	rm -rf ${BINDIR}
	rm ${OUTPUT_DIR}/${OUTPUT}
${SOURCE}:

${BINDIR}:
	mkdir -p ${BINDIR}
${LOGS}:
	mkdir -p ${LOGS}
	mkdir -p ${LOGS}/img
	mkdir -p ${LOGS}/dot
