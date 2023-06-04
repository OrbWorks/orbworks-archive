LIBNAME = PocketCLib
CREATOR = PCL?

OBJECTS = $(LIBNAME).o $(LIBNAME)Dispatch.o

CC = m68k-palmos-gcc -O2

XTRALIBS = -lnfm -lgcc -lPalmOSGlue
BUILDPRC = build-prc

main : $(LIBNAME).prc size

#build the PRC
$(LIBNAME).prc : $(LIBNAME)
	$(BUILDPRC) -L $@ $(LIBNAME) $(CREATOR) $^

#build library intermediate file
$(LIBNAME): $(OBJECTS) Makefile
	$(CC) -shared -nostartfiles -nostdlib -o $(LIBNAME) $(OBJECTS) $(XTRALIBS)

#generic rule for building .o files from respective .cpp files
%.o: %.cpp $(LIBNAME).h
	$(CC) -c $<

mostly_clean :
	rm -f *.o $(LIBNAME)

clean : mostly_clean
	rm -f $(LIBNAME).prc

size:
	@ls -l $(LIBNAME).prc


