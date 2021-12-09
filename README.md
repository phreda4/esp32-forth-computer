# esp32 forth computer

Computer made with ESP32 microcontroler

* Use https://github.com/fdivitto/fabgl (with a modification)
* Inspired by JupiterAce computer
* Inspired by ColorForth language

## Modification of FabGl libary


m_HVSync make public to calculate sync without make color (separate components)

in: src/dispdrivers/vgabasecontroller.h
```
  void setRawPixel(int x, int y, uint8_t rgb)    { VGA_PIXEL(x, y) = rgb; }

  // contains H and V signals for visible line	//********
  volatile uint8_t       m_HVSync;				//**** from above (protected)

protected:
```

Coder https://github.com/rpsubc8
make a better solution, add a class with these things exposed

## Build a complete computer with a esp32 microcontroller

Like a C64 or Jupiter Ace but with R3 Lang

Use FabGL library for devices
	generate VGA signal
	read keyboard
	read mouse (*)
	play sound (*)


## R3 Lang

A colorforth derived language
But, Focus in simplify programer experience.

-Remove compiler manipulation from code

* Clasic forth
	IMMEDIATE MODE
	HERE , - generate code/data
	CREATE DOES> - define behavior in compiler

* ColorForth
	HERE , - generate code/data
	CYAN WORDS - Explicit inline
	
* R3/:R4
	none

the programer not need to think in how compile 
enable static code analisys (not in this implementation)
enable deep optimization when compile
-dead code elimination
-automatic inlines
-discover constant in variables
-reorder code

-Separate memory in DICC, DATA and CODE
DICC is discarded when finish compile
better performance because cache separation (when exist)
the executable memory (or bytecodes) can be readonly

-Use prefix instead of color (really colorforth have prefix in the codification)

-Rethink control flow because this is made with control compiler mechanism

-No access to ASM

Other decision in the language

-0 terminate string
-file instead blocks
-Not use floating point numbers.
-Only a Framebuffer, no console, in esp32 use a console experiment
-Rigid interface with OS, easy convert to other OS but less flexible,
no library can be attached, need to be integrated in words and compile the vm again
(similar to eforth in esp32)


## Prefixes

```
#DATA 33    		| define data, 32 bit signed inter
:CODE dup * ;       | define word

DATA	| push 33 in data stack
'DATA	| push DATA address in stack
CODE	| execute CODE
'CODE	| push CODE address in stack
"this is a string"	| push address (in data memory) of this bytes
%ff %101 	| Hexa an binary numbers
^include.r3	| recursive include system (in esp32 is not recursive)
| comment	| comment
```

Control flow are made by block of code, enclosed by ( and ) and
conditional words of two type, single test, por example
```
0? | is TOS=0?
```
or double test
```
1 2 <? | is 1<2? drop 2
```

## Control Flow structures

### IF, when the conditional are in front of block

```
0? ( "top of stack is zero" print )

var 10 <? ( "var is less 10" print ) drop
```

there are not ELSE construction (colorforth idea!!), the you need to factorize the code,
a good thing!!

```
:test10 | value --
	10 <? ( "less 10" ; )
	"greather 10" ;

:printcond
	test10 print ;
```

### WHILE, when the conditional are in the block

```
0 ( 10 <? dup "%d" print 1 + ) drop
```

for traverse string memory

```
"hola" ( c@+ 1? emit ) drop
```

for traverse memory, fill buffer with 0

```
'buffer $ff ( 1? 0 rot !+ swap ) 2drop
```

## Interactivity.

firt aproach

Made a incremental compiler, generate code and data in a static memory array
All the code is stored tokenized, no text code saved, like 80' microcomputer
can view this tokens colorized easy

I don't like rename, forget, How reorganize order of definitions?
more clear view all code in the source

Back to text source code
Keep the file name in memory, tokenize and execute from disk (flash)

Edit in the filesystem and repeat cicle.

Editor in R3 (good leave the hell of C)
need word RUN, load from file and (compile-)execute.

R4 has a similar eschema.
need remove the definition in console?
because not store in file

The include problem
no word can be use before define
you can include library (coded in r3)

sprite.r3
---------
:a .. ;
:b .. ;
::drawsprite .. a .. b ;

spaceinvader.r3
---------------
^sprite.r3 | import drawsprite, not a or b

.. drawsprite ..

Need a recursive traverse for calculate the order of definition

in ESP32 version restrict the level of include to 1 for avoid this.

add new class of words, only work in console

the framebuffer need memory from ESP32.
I Need try to optimice this
Make a console with a schema of two colors every 8x8 pixels











