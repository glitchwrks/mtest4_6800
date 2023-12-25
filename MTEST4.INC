;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MTEST4 -- Memory Test for Motorola 6800
;
;This program tests a range of memory, specified by the user
;on the console. It can relocate itself anywhere outside of 
;page 0.
;
;Relocation is accomplished through a small mover program,
;which prompts the user for the desired relocation base
;address at startup. The mover is executed from 0x0000, and
;transfers control to the relocated program. Once relocated,
;the mover is no longer required, so page 0 memory can be
;tested.
;
;See machine-specific source for features/caveats specific
;to a given platform.
;
;See README.md in the project root for more information on
;history, inner workings, building, etc.
;
;Copyright (c) 2011 Martin Eberhard
;Copyright (c) 2023 Glitch Works, LLC
;
;See LICENSE included in the project root for licensing
;information.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Program Equates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MAXERS	equ	64		;Max number of errors before we quit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;ASCII Equates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CTRLC	equ	03H		;Control+C
CR	equ	0DH		;Carriage return
LF	equ	0AH		;Line feed

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Page 0 variables for mover
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ORG	0

MDEST:	ds	2		;Destination address for mover
MSOURC:	ds	2		;Source address for mover
MTSTRT:	ds	2		;MTEST start address for user
MVBASE:	ds	2		;New VBASE
	ds	1		;Stack space for 1 BSR
MSTACK:	ds	1		;Initial stack pointer address
GOTOA:	ds	2		;This is actually on the stack

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;The following variables must be in this order, before
;EXEMOV, because they get initialized when the move is
;installed.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MMDEST:
MCOUNT:	ds	2		;Count for bytes of code
MTLEN:	ds	2		;MTEST ofset to last byte
EXEMOV:				;Execution address for mover

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MTEST start of execution
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ORG	0100H		;Start out low in ram

MTEST:	SEI			;Mask interrupts

	LDAB	#SOMSG-VBASE	;Print sign-on message
	BSR	PRNTC2		;...and set x=vbase

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Set up parameters to move the little mover into page 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	STX	MSOURC		;Source will need MOVER-VBASE offset
	LDAB	#MOVEND-MOVER	;B = number of bytes to move
	LDX	#MMDEST		;Destination in page 0
	STX	MDEST

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PUTMOV -- Copy little mover into page 0
;
;We initialize some of the variables during this process.
;
;Falls through to GETBEG.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PUTMOV:	LDX	MSOURC		;Get a source byte, increment ptr
	LDAA	MOVER-VBASE, X
	INX
	STX	MSOURC

	LDX	MDEST		;Put at dest, increment ptr
	STAA	0, X
	INX
	STX	MDEST

	DECB			;Count down to end
	BNE	PUTMOV		;Done when zero

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETBEG -- Compute source address of MTEST move
;
;This routine pushes the current PC onto the stack to figure
;out where we are in memory, and pulls it back -- there is
;no return to the BSR at the beginning.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETBEG:	BSR	GETBE1		;Place PC onto stack
GETBE1:	PULB			;Pull PC high byte
	PULA			;Pull PC low byte
	SUBA	#GETBE1-MTEST	;Correct for above code
	SBCB	#0		;Borrow
	STAA	MSOURC+1	;Save low byte
	STAB	MSOURC		;Save high byte

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETCA -- Get the desired run address for MTEST
;
;The mover cannot relocate MTEST into page 0, since the ROM
;monitor's ECHO flag and the little mover both use it.
;
;Falls through to MCOPY.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETCA:	LDAB	#CAMSG-VBASE	;Ask for destination MTEST address
	BSR	GETADR		;Destination address is in MDEST
				;A = low(MDEST), B = high(MDEST)
				;Z set if B = 0
	BEQ	GETCA		;Try again if page 0 specified

; Compute vbase for post-move code

	ADDA	#(VBASE-MTEST) MOD 0100H
	STAA	MVBASE+1
	ADCB	#(VBASE-MTEST)/256
	STAB	MVBASE

	LDX	MDEST		;Get MTEST start address
	STX	MTSTRT		;Save to display later

	LDAB	#INIT-MTEST	;Set up "return" to moved MTEST
	
GETCA1:	INX			;Loop: compute X = X + B
	DECB
	BNE	GETCA1
	STX	GOTOA		;Store on mover's local stack	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MCOPY -- Do forward or reverse copy
;
;This copy routine works even if the destination address
;overlaps the source address.
;
;When falling through from GETCA, B = 0 from the last
;iteration of GETCA1 loop.
;
;post: Move MDEST < MSOURC if B = 0
;post: move MDEST > MSOURC if B = 1
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MCOPY:	LDAA	MSOURC+1	;LSB compare
	SUBA	MDEST+1
	LDAA	MSOURC		;MSB compare
	SBCA	MDEST
	BCC	MCOPY1		;Source is larger address. copy forward

	INCB			;Note reverse copy

	LDX	#MSOURC		;Compute source last addresses
	BSR	ADDLEN
	LDX	#MDEST		;Compute destination last address
	BSR	ADDLEN

MCOPY1:	LDS	#MSTACK		;Set up stack for little mover
	JMP	EXEMOV		;Move this program

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Subrountines for BSR Reach
;
;Subroutines placed here ensure that we can get to them with
;BSR instructions, enabling relocation.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETADR -- Print message and get 16-bit address from user
;
;pre: VBASE+B contains message string
;post: MDEST contains 16-bit address from user
;post: A contains low byte of address address
;post: B contains high byte of address
;post: Z flag set if address is in page 0
;post: X = VBASE
;post: ERRCNT = 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETADR:	PSHB			;Save string pointer in case of bad input
	BSR	PRNTC2		;Print prompt message, X=VBASE

	CLR	ERRCNT-VBASE, X	;No hex errors yet

	BSR	IN2HEX		;Store high byte into MDEST
	STAA	MDEST
	BSR	IN2HEX		;Store low byte into MDEST+1
	STAA	MDEST+1		;...and also A register

	PULB			;Recover prompt string pointer

	TST	ERRCNT-VBASE, X	;Any errors with hex input?
	BNE	GETADR		;Yes, try again

	LDAB	MDEST		;B = high byte of address and
	RTS			;Z set if address in page 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PRNTC2 -- Stepping stone to PRINTC
;
;This allows us to reach PRINTC with a branch, via PRINTC1.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRNTC2:	BRA	PRNTC1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;ADDLEN -- Add 16-bit (MTLEN) to (X)
;
;This subroutine cannot be used during the actual memory
;test because it uses page 0 memory.
;
;post: (X) = (X) + (MTLEN)
;post: X unchanged
;post: A undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ADDLEN:	LDAA	1, X		;Low byte
	ADDA	MTLEN+1
	STAA	1, X
	LDAA	0, X		;High byte + carry
	ADCA	MTLEN
	STAA	0, X
	RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;IN2HEX -- Get two hex digits from console
;
;Aborts to ROM monitor if a CTRL+C is received (via INHEX).
;
;Falls through to INHEX.
;
;pre: X contains VBASE
;post: A contains hex byte from console
;post: ERRCNT = 0 for no errors
;post: ERRCNT nonzero if errors detected
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IN2HEX:	BSR	INHEX		;Get first hex nybble into B
	ASLB			;Shift to high-order nybble
	ASLB
	ASLB
	ASLB
	TBA			;Save high nybble in A

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;INHEX -- Get a hex digit from console
;
;Aborts to ROM monitor if a CTRL+C is received.
;
;A and B contents are undefined if a non-hex character is
;received.
;
;post: B contains hex nybble from console
;post: A = A + B (combines two nybbles)
;post: ERRCNT = 0 for no errors
;post: ERRCNT nonzero if errors detected
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
INHEX:	JSR	INCH		;Get a char from the console
	CMPB	#CTRLC		;Got abort?
	BEQ	INHEX3		;Yes, go to monitor

	SUBB	#'0'		;Convert ASCII number to hex
	CMPB	#09H
	BLE	INHEX2		;Done already?
	CMPB	#11H
	BMI	INHEX1		;Not hex 
	SUBB	#07H		;Convert ASCII letter to hex
	CMPB	#0fH
	BLE	INHEX2		;Good hex: done

INHEX1:	INC	ERRCNT-VBASE, X	;Flag error

INHEX2:	ABA			;Combine B with high digit in A
	RTS
INHEX3:	JMP	MONIT		;Abort to ROM monitor

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GPARMS -- Print program addresses and get test parameters
;
;The start and end addresses of the moved MTEST program are
;printed, and the user is prompted for memory test address
;range.
;
;MTSTRT, MTLEN remain unchanged.
;
;Falls through to GPARM1.
;
;pre: MTSTRT contains first address of this program
;pre: MTLEN contains byte length of this program
;post: MCOUNT contains last address of this program
;post: ERRCNT = 0
;post: X = VBASE
;post: A, B undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GPARMS:	LDAB	#RNGMSG-VBASE	;Print "mtest:" string
	BSR	PRNTC1

	LDX	#MTSTRT		;X = first program address
	BSR	OUTADR		;Print address as 4 hex digits

	LDX	MTSTRT		;Compute last program address
	STX	MCOUNT		;Store result in MCOUNT
	LDX	#MCOUNT
	BSR	ADDLEN		;(MCOUNT)=(MSTRT)+(MTLEN)
	BSR	OUTADR		;Print address as 4 hex digits
	
; Get memory test parameters. gtsadr returns with the user-
; entered address in mdest and also in a & b, and x=vbase.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GPARM1 -- Get memory test address paramters
;
;Bounds checks are made and enforced:
; * End address must be after or equal to the start address
; * Start address must be beyond MTEST code
; * End address must be before MTEST code (no wraparound)
;
;GETADR returns with the user provided address in MDEST, A
; and B, and X = VBASE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GPARM1:	LDAB	#SAMSG-VBASE	;Print 'start address?' string
	BSR	GETADR
	STAA	TSTART-VBASE+1, X	
	STAB	TSTART-VBASE, X	

	LDAB	#EAMSG-VBASE	;Print 'end address?' string
	BSR	GETADR
	STAA	TEND-VBASE+1, X	;Low byte is in A	
	STAB	TEND-VBASE, X	;High byte is in B

	SUBA	TSTART-VBASE+1, X
	SBCB	TSTART-VBASE, X
	BCS	GPARM1		;Retry if end address < start address

	LDAA	MCOUNT+1
	SUBA	TSTART-VBASE+1, X
	LDAA	MCOUNT
	SBCA	TSTART-VBASE, X
	BCS	GPARM3		;Retry if range is in MTEST code
	
	LDAA	TEND-VBASE+1, X
	SUBA	MTSTRT+1
	LDAA	TEND-VBASE, X
	SBCA	MTSTRT
	BCC	GPARMS		;Retry if range is in MTEST code

GPARM3:	RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETVB2 -- Stepping stone to GETVB
;
;This allows us to reach GETVB with a branch, via GETVB1.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETVB2:	BRA	GETVB1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PRNTC1 -- Stepping stone to PRINTC
;
;This allows us to reach PRINTC with a branch.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRNTC1:	BRA	PRINTC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;OUTADR -- Print (X) on console as space, 4 hex digits
;
;Returns through OUT2H (ROM monitor subroutine).
;
;This subroutine cannot be used during the actual memory
;test.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OUTADR:	JSR	OUTS		;Print leading space
	LDAA	0, X		;High byte
	BSR	OUTAD1
	LDAA	1, X		;Low byte
OUTAD1:	JMP	OUT2H		;Use ROM monitor hex printer

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;INIT -- Initialize variables for actual memory test
;
;We return to this point after MTEST code has been
;relocated.
;
;Falls through to NXTPAS.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
INIT:	LDX	MVBASE		;New VBASE for new code
	STX	0, X

	LDAB	#STACK-VBASE	;Compute address of stack
INIT1:	INX
	DECB
	BNE	INIT1

	TXS			;Set up MTEST stack

	BSR	GPARMS		;Get/validate memory test params

	CLR	PFLAG-VBASE, X	;We need to print 'pass'
	CLR	PASSNO-VBASE, X	;Start with pass number 0
	LDAA	#PATLEN+1	;Initial pattern start
	STAA	PXSTRT-VBASE, X

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;NXTPAS -- Main memory test loop
;
;This routine performs one full pass through the user-
;specified range of memory for each element in the test
;pattern, plus one -- thirty passes in total.
;
;Initial pass just fills with the pattern, quickly
;complementing it a few times with each write to torture
;test each location.
;
;Subsequent passes test previous pass written values and
;fill with the next entry.
;
;NO PAGE 0 MEMORY MAY BE USED AFTER THIS POINT!
;
;Falls through to NXTBYT.
;
;pre: X = VBASE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
NXTPAS:	LDAA	TSTART-VBASE, X	;Set pointer to the start of tested memory
	STAA	TPTR-VBASE, X
	LDAA	TSTART-VBASE+1, X
	STAA	TPTR-VBASE+1, X

	LDAA	PXSTRT-VBASE, X	;Set pattern pointer to this pass's offset
	STAA	PXPTR-VBASE, X

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;NXTBYT - Test and write next byte in memory
;
;Compute pointer to the next pattern entry + 1
;
;pre: X = VBASE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
NXTBYT:	LDAA	PXPTR-VBASE, X	;Point to pattern entry
	DECA
	BNE	NXTBY1		;Modulo bump
	LDAA	#PATLEN
NXTBY1:	STAA	PXPTR-VBASE, X

NXTBY2:	INX			;Get pattern entry The Hard way, compute
	DECA			;X = X + A to get this pattern entry. This
	BNE	NXTBY2		;code is the worst penalty for relocation.

	LDAB	PATRN-VBASE-1, X
	LDAA	PATRN-VBASE, X	;Previous pattern entry

	PSHB			;Save next data

	BSR	GETVB1		;Get address of VBASE
	LDAB	PASSNO-VBASE, X	;First pass?
	LDX	TPTR-VBASE, X	;Get current memory address
				;A = expected value, next value on stack
				;X = address under test
	TSTB			;B = PASSNO, check for first pass
	BEQ	NXTBY3		;Don't test if first pass

	LDAB	0, X		;Read memory into B
	CBA			;Compare to pattern in A
	BEQ	NXTBY3		;OK - keep going
	BSR	MEMERR		;Report error on console
NXTBY3:	PULB			;Pull next data off the stack

	STAB	0, X		;B != 0: fill memory this pass

	COM	0, X		;Use indexed complement to directly
	COM	0, X		;flip bits in memory. Be fast: don't
	COM	0, X		;use a loop. At the end, we have written
	COM	0, X		;the true value into the current location.

; Bump memory pointer and see if we've reached the end

	BSR	GETVB1		;Get address of VBASE
	INC	TPTR-VBASE+1, X	;Bump current memory address
	BNE	NXTBY4
	INC	TPTR-VBASE, X	;16-bit bump

NXTBY4: LDAA	TEND-VBASE+1, X	;Compare address low bytes
	SUBA	TPTR-VBASE+1, X	;Set C on borrow
	LDAA	TEND-VBASE, X	;Compare address high bytes
	SBCA	TPTR-VBASE, X	;Including borrow

	BCC	NXTBYT		;Not done with this pass, next pattern

	BSR	PPASS		;Reached the ending test address, print
				;pass information

	DEC	PXSTRT-VBASE, X	;Bump pattern start point
	BNE	NXTPAS		;Not done, do another pass through memory

	LDAB	#CRMSG-VBASE	;Done, print ending CRLF
	BRA	EXIT		;Return to ROM monitor

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETVB1 -- Stepping stone to GETVB
;
;This allows us to reach GETVB with a branch.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETVB1:	BRA	GETVB

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PRINTC -- Print CR, LF, and a string to console
;
;Falls through to PRINTB.
;
;Aliased by PRINTC1, PRINTC2 as branch stepping stones.
;
;pre: B contains string offset from VBASE
;post: string printed to console
;post: X = VBASE
;post: B contents undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRINTC:	PSHB			;Save string reference
	BSR	GETVB		;X = VBASE
	LDAB	#CRMSG-VBASE	;Print CR, LF
	BSR	PRINTB
	PULB			;Pull string address off stack

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PRINTB -- Print a NULL-terminated string to console
;
;Returns through GETVB.
;
;pre: B contains string offset from VBASE
;pre: X = VBASE
;post: X = VBASE
;post: B contents undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRINTB:	INX
	DECB
	BNE	PRINTB		;Loop until X points to string

PRNTB1:	LDAB	0, X		;Get a string char
	BEQ	GETVB		;Exit through GETVB if NULL
	INX			;Point to next string char
	JSR	OUTCH		;Call to ROM monitor to print char in B
	BRA	PRNTB1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MEMERR -- Report a memory error
;
;Exits via OUTSHX.
;
;pre: A contains expected data
;pre: B contains found data
;pre: X = TPTR = address of error
;post: ERRCNT is decremented, abort if 0
;post X = TPTR
;post: B contents undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MEMERR:	BSR	GETVB		;Get address of VBASE

	PSHB			;Put found data onto stack
	PSHA			;Put expected data onto stack

	INC	ERRCNT-VBASE, X	;Too many errors?
	LDAB	#MAXERS		;Max allowed errors
	CMPB	ERRCNT-VBASE, X
	BEQ	ERDONE		;Quit if too many errors

	CLR	PFLAG-VBASE, X	;We need to print 'pass' message

	LDAB	#ERRMS1-VBASE	;Print 'error at' string
	BSR	PRINTC

	LDAA	TPTR-VBASE, X	;Print high byte of address
	BSR	OUTSHX
	LDAA	TPTR-VBASE+1, X	;Print low byte of address
	BSR	OUTHEX

	LDAB	#ERRMS2-VBASE	;Print ' wrote' string
	BSR	PRINTB

	PULA			;Pull expected value off stack
	BSR	OUTSHX

	LDAB	#ERRMS3-VBASE	;Print ' read' string
	BSR	PRINTB

	LDX	TPTR-VBASE, X	;Recover address pointer

	PULA			;Pull found value off stack
	BRA	OUTSHX		;Exit via OUTSHX

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PPASS -- Print pass number
;
;Print the pass number to the console, with 'pass' prefix if
;needed.
;
;Falls through to OUTSHX.
;
;pre: X = VBASE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PPASS:	DEC	PFLAG-VBASE, X	;Enough entries on this line?
	BPL	PPASS1
	LDAB	#15		;Max 16 entries per line
	STAB	PFLAG-VBASE, X
	
	LDAB	#PASMSG-VBASE	;Print CR, LF, 'pass'
	BSR	PRINTC

PPASS1:	LDAA	PASSNO-VBASE, X	;Print pass number
	INC	PASSNO-VBASE, X	;Increment pass number

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;OUTSHX -- Print a space and two hex digits
;
;Preserves A register, clobbers B register.
;
;pre: A register contains byte to print
;post: space and hex representation of byte in A printed
;post: A register contains byte printed
;post: B contents undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OUTSHX:	JSR	OUTS		;Print leading space

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;OUTHEX -- Print two hex digits
;
;This implementation uses the hex printer in the MITS ROM
;monitor.
;
;pre: A register contains byte to print
;post: hex representation of byte in A printed
;post: A register contains byte printed
;post: B contents undefined
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
OUTHEX:	JMP	OUT2H		;Use ROM monitor routine

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;ERDONE -- Quit with too many errors
;
;Falls through to EXIT.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ERDONE:	LDAB	#MORMSG-VBASE	;Print '>'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;EXIT -- Quit for successful run
;
;pre: B contains offset to exit message
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
EXIT:	BSR	PRINTC		;Print with preceeding CR LF
	LDAA	ERRCNT-VBASE, X	;Print error count
	BSR	OUTHEX
	LDAB	#DONMSG-VBASE
	BSR	PRINTB		;Print final message

	JMP	MONIT		;Return to ROM monitor

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;GETVB -- Get the value of VBASE into X
;
;This subroutine must be at the end, right before strings
;and variables.
;
;This nasty little piece of self-modifying code makes the
;rest of the code relocatable.
;
;post: X = VBASE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GETVB:	db	0CEH		;LDX immediate
VBASE:	dw	VBASE		;This gets modified when the code
	RTS			;...gets moved

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;NULL-Terminated Strings
;
;NULLs after CR, LF have the parity bit set.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PASMSG:	db	"PASS:", 0
SOMSG:	db	"680 MTEST 1.4", CR, LF, 80H
	db	"BY M. EBERHARD"
CRMSG:	db	CR, LF, 80H, 0
CAMSG:	db	"CODE ADDR (>00FF)? ", 0
SAMSG:	db	"BEG ADDR? ", 0
EAMSG:	db	"END ADDR? ", 0
RNGMSG:	db	"MTEST:", 0
ERRMS1:	db	"ERR AT", 0
ERRMS2:	db	" WROTE:", 0
ERRMS3:	db	" READ:", 0
DONMSG:	db	" ERRORS"	;Fall into NULMSG
NULMSG:	db	80H, 80H, 0	;NULLs protect for ACIA reset
MORMSG:	db	"> ", 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;PATRN -- Test Pattern Sequence
;
;This sequence is a prime number in length and designed to
;cause a lot of bit-flipping, to catch any bit-coupling and
;address-coupling problems.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PATRN:	db	001H, 0FEH, 002H, 0FDH, 004H, 0FBH, 008H, 0F7H
	db	010H, 0EFH, 020H, 0FDH, 040H, 0BFH, 080H, 07FH
	db	000H, 0FFH, 055H, 0AAH, 033H, 0CCH, 0F0H, 00FH
	db	0C3H, 03CH, 066H, 099H, 078H
PATEND:	db	001H		;Wrap pattern one byte
	
PATLEN	equ	PATEND-PATRN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;RAM Variables
;
;These variables are not in page 0 so that we can test RAM
;located in page 0.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ds	15	;Reserve space for MTEST stack
STACK:	ds	1	;Stack needs to be before RAM variables

TSTART:	ds	2	;First memory address to test
PXSTRT:	ds	1	;Pattern starting point
TPTR:	ds	2	;Current memory test location
PXPTR:	ds	1	;Current place in pattern
TEND:	ds	2	;Last memory address to test
ERRCNT:	ds	1	;Error count
PFLAG:	ds	1	;0 means we need to print 'pass'
PASSNO:	ds	1	;Current pass number

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MOVER -- Little Code Mover
;
;This routine copies code to a new location. It tests for
;good writes, and aborts to MONIT if not. It gets put in
;page 0 before running.
;
;This routine must be here so that it is close to VBASE.
;
;pre: B = 0 for forward copy (MDEST < MSOURC)
;pre: B = 1 for reverse copy (MDEST > MSOURC)
;pre: MSOURCE contains first address of source
;pre: MDEST contains first address of destination
;pre: MCOUNT contains number of bytes to copy
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
MOVER:	dw	MOVEND-MTEST	;This initializes MCOUNT
	dw	MOVEND-MTEST-1	;This initializes MTLEN

MOVER1:	LDX	MSOURC	;Get a source byte
	LDAA	0, X
	BSR	MOVER3	;Bump and save pointer
	STX	MSOURC

	LDX	MDEST	;Save byte at destination
	STAA	0, X

	CMPA	0, X	;Make sure it wrote, abort if not
	BNE	MONIT+MOVER-EXEMOV

	BSR	MOVER3	;bump and save pointer
	STX	MDEST

	LDX	MCOUNT	;Bump and test loop count
	DEX
	STX	MCOUNT
	BNE	MOVER1

MOVER2:	INX		;A little code recycling
	RTS		;Jump to new code

MOVER3:	TSTB		;Test contents of B
	BEQ	MOVER2	;B = 0, increment X
	DEX		;B = 1, decrement X
	RTS
MOVEND:
