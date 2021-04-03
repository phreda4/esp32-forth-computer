
#buf * 16

:bb 'buf 15 + 0 over c! 1 - ;
:dec bb swap ( 10 /mod $30 + pick2 c! swap 1 - swap 1? ) drop 1 + ;
:hex bb swap ( dup $f and 9 >? ( 7 + ) $30 + pick2 c! 4 >>> swap 1 - swap 1? ) drop 1 + ;

::.r. | b nro -- b
	'buf 15 + swap -
	swap ( over >? 1 - $20 over c! ) drop ;

:colors
	cls
	63 ink
	0 ( 8 <?
		0 ( 8 <?
			over 3 << over or
			dup paper
			dec 3 .r. print 32 emit
			1 + ) drop
		1 + cr ) drop ;
