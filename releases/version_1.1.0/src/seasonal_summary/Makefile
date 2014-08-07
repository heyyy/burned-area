#------------------------------------------------------------------------------
# Makefile for sample HDF code
#------------------------------------------------------------------------------

# Set up compile options
CC = gcc
RM = rm -f
EXTRA = -Wall -g

# Define the include files
INC = bool.h determine_max_extent.h
INCDIR  = -I. -I$(HDFINC) -I$(HDFEOS_INC) -I$(HDFEOS_GCTPINC) -I$(JPEGINC) \
          -I$(SZIPINC) -I$(ZLIBINC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the source code and object files
SRC = get_args.c                   \
      error_handler.c              \
      read_extent.c                \
      determine_max_extent.c

OBJ = $(SRC:.c=.o)

# Define the object libraries
LIB   = -L$(HDFLIB) -lmfhdf -ldf -lxdr -L$(JPEGLIB) -ljpeg \
        -L$(ZLIBLIB) -lz -L$(SZIPLIB) -lsz -lm
EOSLIB = -L$(HDFEOS_LIB) -lhdfeos -L$(HDFEOS_GCTPLIB) -lGctp

# Define the executable
EXE = determine_max_extent

# Target for the executable
all: $(EXE)

determine_max_extent: $(OBJ) $(INC)
	$(CC) $(EXTRA) -o $(EXE) $(OBJ) $(EOSLIB) $(LIB)

install:
	install -d $(BIN)
	install -m 755 $(EXE) $(BIN)

clean:
	$(RM) $(OBJ) $(EXE)

$(OBJ): $(INC)

.c.o:
	$(CC) $(NCFLAGS) -c $<

