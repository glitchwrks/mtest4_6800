;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MTEST680 -- Altair 680 Memory Test Program
;
;This program tests a range of Altair 680 memory, specified
;by the user on the console. It can relocate itself anywhere
;outside of page 0.
;
;See README.md in the project root for more information on
;history, inner workings, building, etc.
;
;BEWARE! The current revision cannot test all of page 0
;since it uses the MITS ROM monitor I/O routines, which need
;some page 0 variables to operate!
;
;Copyright (c) 2011 Martin Eberhard
;Copyright (c) 2023 Glitch Works, LLC
;
;See LICENSE included in the project root for licensing
;information.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;MITS Altair 680 ROM Monitor Equates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
INCH	equ	0FF00H		;Get char from console in A
OUT2H	equ	0FF6DH		;Print A to console in hex, clobber B
OUTCH	equ	0FF81H		;Print char in B on console
OUTS	equ	0FF82H		;Print a space on the console
MONIT	equ	0FFD8H		;ROM monitor cold start
ECHO	equ	0F3H		;ROM monitor echo flag

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Start of actual object code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	CPU	6800		;Use only M6800 opcodes

	ORG	ECHO		;Disable echo during loading
	db	80H

	INCLUDE	"MTEST4.INC"	;Main memory test include

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Turn character echo on after loading
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ORG	ECHO
	db	0

	END
