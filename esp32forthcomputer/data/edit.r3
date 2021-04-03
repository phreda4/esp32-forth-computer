| editor for ESP32
| PHREDA 2021

|-------------------------
#.exit 0

:exit
	1 '.exit ! ;

:onshow | vector --
	0 '.exit !
	( .exit 0? drop
		dup ex
		redraw ) 2drop ;

::min	| a b -- m
	over - dup 31 >> and + ;

::max	| a b -- m
	over swap - dup 31 >> and - ;

#buf * 16

:bb 'buf 15 + 0 over c! 1 - ;
:dec bb swap ( 10 /mod $30 + pick2 c! swap 1 - swap 1? ) drop 1 + ;
:hex bb swap ( dup $f and 9 >? ( 7 + ) $30 + pick2 c! 4 >>> swap 1 - swap 1? ) drop 1 + ;

::.r. | b nro -- b
	'buf 15 + swap -
	swap ( over >?
		1 - $20 over c!
		) drop ;

##<esc> $7b00 ##>esc< $17b00
##<ins> $7f00
##<del>	$8100
##<back>	$8300
##<home>	$8400
##<end>	$8600
##<tab> $8d00
##<ret> $8e00
##<pgup>	$9200
##<pgdn>	$9400
##<up>		$9600
##<dn>	$9800
##<le>	$9a00
##<ri>	$9c00
##<f1>	$9f00
##<f2> $a000
##<f3> $a100
##<f4> $a200
##<f5> $a300
##<f6> $a400
##<f7> $a500
##<f8> $a600
##<f9> $a700
##<f10> $a800
##<f11> $a900
##<f12> $aa00

|:emit  drop ;
|:print drop ;
|:atxy 2drop ;

|-------------------------
#here

#name * 1024

#xlinea 0
#ylinea 0	| primera linea visible
#ycursor
#xcursor

#pantaini>	| comienzo de pantalla
#pantafin>	| fin de pantalla

#fuente  	| fuente editable
#fuente> 	| cursor
#$fuente	| fin de texto

#panelcontrol

#emode
#xcode 0
#ycode 0
#wcode 39
#hcode 29

::only13 | adr -- ; remove 10..reeplace with 13
	dup
	( c@+ 1?
		13 =? ( over c@	10 =? ( rot 1 + rot rot ) drop )
		10 =? ( drop c@+ 13 <>? ( drop 1 - 13 ) )
		rot c!+ swap ) nip
	swap c! ;

::count | s1 -- s1 cnt
	dup >a
	0 ( a@+ dup $01010101 -
		swap not and
		$80808080 nand? drop 4 + )
	$80 and? ( drop ; )
	$8000 and? ( drop 1 + ; )
	$800000 and? ( drop 2 + ; )
	drop 3 + ;

::strcpy | src des --
	( swap c@+ 1? rot c!+ ) nip swap c! ;

::edload | "" --
	dup 'name strcpy
	fuente swap load 0 swap c!
	fuente only13 	|-- queda solo cr al fin de linea
	fuente dup 'pantaini> !
	count + '$fuente !
	;


:dd
	>a
	16 ( 1? 1 -
		8 ( 1? 1 -
			a> c@+ hex 32 emit print >a
			) drop cr
		) drop ;

|----- edicion
:lins  | c --
	fuente> dup 1 - $fuente over - 1 + cmove>
	1 '$fuente +!
:lover | c --
	fuente> c!+ dup 'fuente> !
	$fuente >? ( dup '$fuente ! ) drop
:0lin | --
	0 $fuente c! ;

#modo 'lins

:back
	fuente> fuente <=? ( drop ; )
	dup 1 - swap $fuente over - 1 + cmove
	-1 '$fuente +!
	-1 'fuente> +! ;

:del
	fuente>	$fuente >=? ( drop ; )
    1 + fuente <=? ( drop ; )
	dup 1 - swap $fuente over - 1 + cmove
	-1 '$fuente +! ;

:<<13 | a -- a
	( fuente >=?
		 dup c@
		13 =? ( drop ; )
		drop 1 - ) ;

:>>13 | a -- a
	( $fuente <?
		 dup c@
		13 =? ( drop 1 - ; ) | quitar el 1 -
		drop 1 + )
	drop $fuente 2 - ;

:khome	fuente> 1 - <<13 1 + 'fuente> ! ;

:kend	fuente> >>13  1 + 'fuente> ! ;

:scrollup | 'fuente -- 'fuente
	pantaini> 1 - <<13 1 - <<13  1 + 'pantaini> !
	ylinea 1? ( 1 - ) 'ylinea ! ;

:scrolldw
	pantaini> >>13 2 + 'pantaini> !
	pantafin> >>13 2 + 'pantafin> !
	1 'ylinea +! ;

:colcur
	fuente> 1 - <<13 swap - ;

:karriba
	fuente> fuente =? ( drop ; )
	dup 1 - <<13		| cur inili
	swap over - swap	| cnt cur
	dup 1 - <<13			| cnt cur cura
	swap over - 		| cnt cura cur-cura
	rot min + fuente max
	'fuente> ! ;

:kabajo
	fuente> $fuente >=? ( drop ; )
	dup 1 - <<13 | cur inilinea
	over swap - swap | cnt cursor
	>>13 1 +    | cnt cura
	dup 1 + >>13 1 + 	| cnt cura curb
	over -
	rot min +
	'fuente> ! ;

:kder	fuente> $fuente <? ( 1 + 'fuente> ! ; ) drop ;
:kizq	fuente> fuente >? ( 1 - 'fuente> ! ; ) drop ;
:kpgup	27 ( 1? 1 - karriba ) drop ;
:kpgdn  27 ( 1? 1 - kabajo ) drop ;

|-------------------------
:mode!edit 	0 'emode ! ;
:mode!find  2 'emode ! ;
:mode!error 3 'emode ! ;

|-------------------------
:iniline
	xlinea
	( 1? 1 - swap
		c@+ 0? ( drop nip 1 - ; )
		13 =? ( drop nip 1 - ; )
		drop swap ) drop ;



:spfill | cnt adr -- adr
	swap ( 1? 1 - 32 emit ) drop  ;

:emitline | adr -- adr'
	35 ( 1? 1 - swap
		c@+ 0? ( drop 1 - spfill ; )
		13 =? ( drop spfill ; )
		9 =? ( 32 nip )
		emit
		swap ) drop ;

|ccx xsele <? ( drop ; ) drop
|	( c@+ 1? 13 <>? drop ) drop 1 -		| eat line to cr or 0
|	39 gotox
|	$ffffff 'ink ! "." print

:drawline
|	iniline
	emitline ;

:linenro | lin -- lin
	dup ylinea + dec 3 .r. print 32 emit drop ;

:drawcode
	pantaini>
	1 ( 28 <?
		0 over atxy
		linenro
		swap drawline
		swap 1 + ) drop
	$fuente <? ( 1 - ) 'pantafin> !
|	fuente>
|	( pantafin> >? scrolldw )
|	( pantaini> <? scrollup )
|	drop
	;

|..............................
:emitcur
	13 =? ( drop 1 'ycursor +! 0 'xcursor ! ; )
	9 =? ( drop 4 'xcursor +! ; )
	1 'xcursor +! ;

:drawcursor
	ylinea 'ycursor ! 0 'xcursor !
	pantaini> ( fuente> <? c@+ emitcur ) drop
	| hscroll
	xcursor
	xlinea <? ( dup 'xlinea ! )
	xlinea wcode + 4 - >=? ( dup wcode - 5 + 'xlinea ! ) | 4 linenumber
	drop
	xcursor ycursor atxy
	;

:editmodeke
|	panelcontrol 1? ( drop controlkey ; ) drop

	key
	0? ( drop ; )
	dup $ff and =? ( 32 >=? ( modo ex drawscreen ; ) )

	<back> =? ( back )
	<del> =? ( del )
	<up> =? ( karriba ) <dn> =? ( kabajo )
	<ri> =? ( kder ) <le> =? ( kizq )
	<home> =? ( khome ) <end> =? ( kend )
	<pgup> =? ( kpgup ) <pgdn> =? ( kpgdn )
	<ins> =? (  modo
				'lins =? ( drop 'lover 'modo ! ; )
				drop 'lins 'modo ! )
	<ret> =? (  13 modo ex )
	<tab> =? (  9 modo ex )
	>esc< =? ( exit )

|	<ctrl> =? ( controlon ) >ctrl< =? ( controloff )
|	<shift> =? ( 1 'mshift ! ) >shift< =? ( 0 'mshift ! )

	drop
	drawscreen
	;
|-------------------------
:barrac | control+
	"^Xcut " print
	"^Copy " print
	"^Vpaste" print
	"^Find" print
|	'findpad
|	dup c@ 0? ( 2drop ; ) drop
|	" [%s]" print
	;

:barra
	21 paper 63 ink
	0 0 atxy
	":r3 editor " print
	xcursor 1 + dec print ":" print
	ycursor 1 + dec print

	0 28  atxy
	panelcontrol 1? ( drop barrac ; ) drop
|	'name print
	" name " print
	0 paper
	;

:drawscreen
	barra drawcode ;

:editando
	editmodeke ;

|	emode
|	0? ( editmodeke )
|	2 =? ( findmodekey )
|	3 =? ( errmodekey )
|	drop ;

::edrun
	0 'xlinea !
	0 'ylinea !
	mode!edit
	cls
	drawscreen
	'editando onshow
	;

::edram
	mem	| --- RAM
	dup 'fuente !
	dup 'fuente> !
	dup '$fuente !
	dup 'pantaini> !
	$3fff +				| 16KB
	'here  ! | -- FREE
	0 here !
	;

:ed
	edram
	"/colors.r3" edload
	edrun ;