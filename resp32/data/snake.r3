| simple snake game
| PHREDA 2021
^lib/base.r3
^lib/keys.r3

#px 10 #py 10	| player pos
#xv #yv			| player velocity
#ax 15 #ay 15	| fruit pos
#trail * 1024	| tail array
#trail> 'trail
#tail 5			| tail size

|----------------
#seed 495090497

:rand | -- r32
  seed 3141592621 * 1 + dup 'seed ! ;

:lrand rand swap mod abs ;


|-----------------
:pack | x y -- xy
	16 << or ;

:unpack | xy -- x y
	dup $ffff and swap 16 >> ;

:rpush | v --
	trail> !+ 'trail> ! ;

:rshift | --
	'trail dup 4 + trail> over - 2 >> move -4 'trail> +! ;

:drawbox | x y --
	atxy 127 emit ;

:hit? | x y -- x y
	py <>? ( ; )
	swap px <>? ( swap ; ) swap
	5 'tail ! ;

:keyboard
	key
	$9600 =? ( -1 'yv ! 0 'xv ! )
	$9800 =? ( 1 'yv ! 0 'xv ! )
	$9a00 =? ( -1 'xv ! 0 'yv ! )
	$9c00 =? ( 1 'xv ! 0 'yv ! )
	27 =? ( exit )
	drop ;

:snakemove
	px xv + 0 <? ( drop 39 ) 39 >? ( drop 0 ) 'px !
	py yv + 0 <? ( drop 28 ) 28 >? ( drop 0 ) 'py ! ;

:newbug
	1 'tail +! 39 lrand 'ax ! 28 lrand 'ay ! ;

:game
	cls
	7 ink
	'trail ( trail> <? @+ unpack hit? drawbox ) drop
	px py pack rpush
	tail ( trail> 'trail - 2 >> <? rshift ) drop
	px ax - py ay - or 0? ( newbug ) drop
	15 ink ax ay atxy 15 emit
	63 paper 3 ink
	0 29 atxy " SNAKE - ESP32 Forth Computer - " print
    0 paper
	keyboard
	snakemove
	20 fps
	;

:snake 'game onshow ;