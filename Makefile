IDIR=include
CCXX=g++
CFLAGS=-I$(IDIR)

SRCDIR=src
OUTDIR=bin
OBJDIR=$(OUTDIR)/obj
LDIR=lib
LIBS=-lpthread

_DEPS = Common.h Epilogue.h EpilogueT.h ParallelDomainTransform.h Prologue.h PrologueT.h Util.h
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJ = Epilogue.o EpilogueT.o Main.o ParallelDomainTransform.o Prologue.o PrologueT.o Util.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

all: parallel-domain-transform

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	$(shell mkdir -p $(@D))
	$(CCXX) -c -o $@ $< $(CFLAGS)

parallel-domain-transform: $(OBJ)
	$(shell mkdir -p $(@D))
	$(CCXX) -o $(OUTDIR)/$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -r $(OUTDIR)
