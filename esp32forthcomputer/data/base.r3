#seed 495090497

:rand
  seed 3141592621 * 1 + dup 'seed ! ;

#.exit 0

:exit
	1 '.exit ! ;

:onshow | vector --
	0 '.exit !
	( .exit 0? drop
		dup ex
		redraw ) 2drop ;

#buf * 16

:bb 'buf 15 + 0 over c! 1 - ;

:dec bb swap ( 10 /mod $30 + pick2 c! swap 1 - swap 1? ) drop 1 + ;

:hex bb swap ( dup $f and 9 >? ( 7 + ) $30 + pick2 c! 4 >>> swap 1 - swap 1? ) drop 1 + ;


