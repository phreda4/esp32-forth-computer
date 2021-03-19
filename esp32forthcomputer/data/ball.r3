#x 0
#y 0
#xv 1
#yv 1
:ball x y atxy 15 emit ;
:hitx xv neg 'xv ! ;
:hity yv neg 'yv ! ;
:add xv 'x +! yv 'y +! ;
:hit x 0? ( hitx ) 39 =? ( hitx ) drop y 0? ( hity ) 28 =? ( hity ) drop ;
:frame ball add hit redraw ;
:demo ( key 27 <>? drop cls frame ) drop ;
