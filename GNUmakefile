# Insertions and deletions work:  "crossovers" and duplications don't.

all: sampler

# Hurts:
# -mfpmath=sse     1:23 -> 1:30
# -mfpmath=387,sse 1:22->1:24

# No effect:
# -malign-double
# -mmmx, -msse, -msse2 (w/ or w/o -mfpmath=sse)
# -O3

# Helps:
# -fomit-frame-pointer: 1:23.7 -> 1:23.2
# -ffast-math 
# -fprefetch-loop-arrays
# -march=pentium4

# -fexpensive-optimizations
# -fno-strength-reduce?
# -fno-exceptions -fno-rtti


# try -ffast-math, -march=pentium4, -malign-double, -mfpmath=sse,387
#     -msse2

# -fomit-frame-pointer -pipe -fexpensive-optimizations -fpic -frerun-cse-after-loop -frerun-loop-opt -foptimize-register-move
# -freorder-blocks

#-mfpmath=sse,387 ?

#----------------- Definitions
LANGO = fast-math  tracer omit-frame-pointer prefetch-loop-arrays
DEBUG = pipe #g3 #gdwarf-2 #pg 
EXACTFLAGS = --param max-inline-insns-single=1000 --param max-inline-insns-auto=150
DEFS = NDEBUG NDEBUG_DP # USE_UBLAS
WARN = all no-sign-compare overloaded-virtual
OPT =  march=pentium3 O3 # malign-double msse mmmx msse2 
LDFLAGS = #-pg # -static 
LI=${CXX}

#------------------- Main 
PROGNAME = sampler
NAME = sampler
SOURCES = sequence.C tree.C alignment.C substitution.C moves.C \
          rng.C branch-sample.C exponential.C \
          eigenvalue.C parameters.C likelihood.C mcmc.C \
	  choose.C sequencetree.C branch-lengths.C arguments.C \
	  util.C randomtree.C alphabet.C smodel.C sampler.C \
	  tri-sample.C dpmatrix.C 3way.C 2way.C branch-sample2.C \
	  node-sample2.C imodel.C 5way.C topology-sample2.C inverse.C \
	  setup.C rates.C matcache.C

LIBS = gsl gslcblas m 
GSLLIBS = ${LIBS:%=-l%}
SLIBS =  #lapack cblas atlas # gsl gslcblas m 
LINKLIBS = ${LIBS:%=-l%} ${SLIBS:%=lib%.a} /usr/local/lib/liblapack.a /usr/local/lib/libcblas.a /usr/local/lib/libatlas.a
PROGNAMES = ${NAME} 
ALLSOURCES = ${SOURCES} 

${NAME} : ${SOURCES:%.C=%.o} ${LINKLIBS}

bin/alignment-blame: alignment.o arguments.o alphabet.o sequence.o util.o rng.o \
	tree.o sequencetree.o bin/optimize.o bin/findroot.o bin/alignmentutil.o \
	setup.o smodel.o rates.o exponential.o eigenvalue.o ${GSLLIBS}

bin/alignment-reorder: alignment.o arguments.o alphabet.o sequence.o util.o rng.o \
	tree.o sequencetree.o bin/optimize.o bin/findroot.o setup.o smodel.o \
	rates.o exponential.o eigenvalue.o ${GSLLIBS}

bin/truckgraph: alignment.o arguments.o alphabet.o sequence.o util.o rng.o ${LIBS:%=-l%}

bin/truckgraph2: alignment.o arguments.o alphabet.o sequence.o util.o \
		bin/alignmentutil.o rng.o ${GSLLIBS}

bin/truckgraph3d: alignment.o arguments.o alphabet.o sequence.o util.o rng.o ${LIBS:%=-l%}

bin/treecount: tree.o sequencetree.o arguments.o util.o rng.o bin/statistics.o ${LIBS:%=-l%}

bin/treedist: tree.o sequencetree.o arguments.o

bin/tree-to-srq: tree.o sequencetree.o arguments.o

bin/srqtoplot: arguments.o

bin/srqanalyze: arguments.o rng.o bin/statistics.o ${LIBS:%=-l%}

bin/reroot: tree.o sequencetree.o arguments.o

bin/make_random_tree: tree.o sequencetree.o arguments.o util.o\
	 rng.o  ${LIBS:%=-l%}

bin/drawalignment: tree.o alignment.o sequencetree.o arguments.o \
	rng.o alphabet.o sequence.o util.o setup.o smodel.o exponential.o \
	eigenvalue.o rates.o ${LINKLIBS}

bin/phy_to_fasta: alignment.o sequence.o arguments.o alphabet.o \
	rng.o util.o ${LIBS:%=-l%}

bin/analyze_distances: alignment.o alphabet.o sequence.o arguments.o alphabet.o \
	util.o sequencetree.o substitution.o eigenvalue.o tree.o sequencetree.o \
	parameters.o exponential.o smodel.o imodel.o rng.o likelihood.o \
	dpmatrix.o choose.o bin/optimize.o inverse.o setup.o rates.o matcache.o ${LINKLIBS}

bin/statreport: bin/statistics.o

bin/findalign: alignment.o alphabet.o arguments.o sequence.o bin/alignmentutil.o \
	rng.o ${GSLLIBS} util.o

#-----------------Other Files
OTHERFILES += 

#------------------- End
DEVEL = ../..
includes += /usr/local/include/
includes += .
src      += 
include $(DEVEL)/GNUmakefile
# CC=gcc-3.4
# CXX=g++-3.4