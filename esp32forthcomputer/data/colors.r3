| view colors

^lib/base.r3

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
