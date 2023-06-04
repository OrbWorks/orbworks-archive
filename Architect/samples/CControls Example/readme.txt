CControls with color
--------------------
The code in this folder is an updated version of CControls that is
optimized for color devices. When run on OS 3.5 or higher, it uses the
system defined colors for drawing all controls. On devices that support
color schemes, CControls now uses the user selected colors. The only
other minor feature is that it uses getsi() when selecting an edit
control to display the input form in place.

To use this updated version of CControls, all you need to do is add a
call to Cinit() before drawing any controls.


CControls Example
-----------------
The updated example contains an animated image control, demonstrates the
call to Cinit(), and calls drawscreen() in response to the redraw event
added in PocketC v7.


License
-------
The updated code does not change the licensing terms - see the docs
included with the original version.