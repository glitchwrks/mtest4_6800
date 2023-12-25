RMFLAGS		= -f

ASM		= a68
RM		= rm

all: altair680

altair680:
	$(ASM) MTEST4.ASM -s MTEST4.S -l MTEST4.PRN

clean:
	$(RM) $(RMFLAGS) *.S *.PRN

distclean: clean
