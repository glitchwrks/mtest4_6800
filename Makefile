RMFLAGS		= -f

ASM		= a68
RM		= rm

all: altair680

altair680:
	$(ASM) MTEST680.ASM -s MTEST680.S -l MTEST680.PRN

clean:
	$(RM) $(RMFLAGS) *.S *.PRN

distclean: clean
