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

##Build a complete computer with a esp32 microcontroller

Like a C64 or Jupiter Ace but with R3 Lang

Use FabGL library for devices
	generate VGA signal
	read keyboard
	read mouse
	play Sound


##R3 Lang

A colorforth derived language

Focus in simplify programer experience.

-Remove compiler manipulation from code

-Clasic forth
	IMMEDIATE MODE
	HERE , - generate code
	CREATE DOES> - define behavior in compiler

-ColorForth
	HERE ,
	CYAN WORDS - Explicit inline

The compiler decision are make by the compiler (to bytcodes or to assembly)

-Use prefix instead of color (really coloforth have prefix in the codification)

-Rethink control flow because this is made with control compiler mechanism

-No access to ASM

Questionable decision (but is a experiment!!)

-0 terminate string
-file instead blocks
-Not use floating point numbers.
-Only a Framebuffer, no console, in esp32 use a console experiment
-Rigid interface with OS, easy convert to other OS but less flexible,
no library can be attached, need to be integrated in words


##Prefixes

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
0? |(is the TOS 0?)
```
or double test
```
1 2 <? (is 1 less 2 ?) drop 2
```

##Two constructor

###IF, when the conditional are in front of block

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

###WHILE, when the conditional are in the block


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

##Interactivity.

firt aproach

Made a incremental compiler, generate code and data in a static memory array
All the code is stored tokenized, no text code saved
can view this tokens colorized easy

I don't like rename, forget, How reorganize order of definitions?

Back to text source code
Keep the file name in memory, tokenize and execute

Edit in the filesystem and repeat cicle.

Editor in R3 (good leave the hell of C)
need word RUN, load from file and (compile-)execute.

R4 has a similar eschema.
need cut the definition in console? because not store in file

The include problem








