| simple snake game
| PHREDA 2021

#tc 30	| arena size
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

|----------------
#.exit 0

:exit
	1 '.exit ! ;

:onshow | vector --
	0 '.exit !
	( .exit 0? drop
		dup ex
		redraw ) 2drop ;

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
	5 'tail !
	;

:vlimit | v -- v
	0 <? ( drop tc 1 - ; )
	tc >=? ( drop 0 ; )
	;

:randtc | -- r
	rand tc mod abs ;

:game
	cls

	px xv + vlimit 'px !
	py yv + vlimit 'py !

	7 ink
	'trail ( trail> <?
		@+ unpack hit? drawbox ) drop

	px py pack rpush
	tail ( trail> 'trail - 2 >> <? rshift ) drop

	px ax - py ay - or 0? (
		1 'tail +! randtc 'ax ! randtc 'ay !
		) drop

	15 ink
	ax ay atxy 15 emit

	key
	<up> =? ( -1 'yv ! 0 'xv ! )
	<dn> =? ( 1 'yv ! 0 'xv ! )
	<le> =? ( -1 'xv ! 0 'yv ! )
	<ri> =? ( 1 'xv ! 0 'yv ! )
	27 =? ( exit )
	drop
	;


:snake 'game onshow ;