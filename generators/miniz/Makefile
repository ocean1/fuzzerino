CC         = ../../afl/bin/gfz-clang-fast
CFLAGS     = -g -I./amalgamation -I.
OBJDIR     = obj

all: cleangfz
	mkdir $(OBJDIR)
	$(CC) $(CFLAGS) -c miniz.c -o $(OBJDIR)/miniz_instr.o
	$(CC) $(CFLAGS) -c miniz_tdef.c -o $(OBJDIR)/miniz_tdef_instr.o
	$(CC) $(CFLAGS) -c miniz_tinfl.c -o $(OBJDIR)/miniz_tinfl_instr.o
	$(CC) $(CFLAGS) -c miniz_zip.c -o $(OBJDIR)/miniz_zip_instr.o
	$(CC) $(CFLAGS) -c example6.c -o $(OBJDIR)/ex6.o
	$(CC) $(CFLAGS) $(OBJDIR)/miniz_instr.o $(OBJDIR)/miniz_tdef_instr.o $(OBJDIR)/miniz_zip_instr.o $(OBJDIR)/miniz_tinfl_instr.o $(OBJDIR)/ex6.o -o miniz -lm

cleangfz:
	rm -rf /dev/shm/gfzidfile

clean:
	rm -rf $(OBJDIR)
	rm -rf miniz
	rm -rf out
	rm -rf *.log
	rm -rf gfz_plot_data
	rm -rf gfz_bnc_ban
	rm -rf gfz_num_ban
	rm -rf gfz_ptr_ban