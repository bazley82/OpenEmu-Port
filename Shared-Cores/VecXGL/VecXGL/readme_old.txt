VecXGL 1.2 (SDL/Win32)

Written by James Higgs 2005/2006/2007.

This is a port of the Vectrex emulator "vecx", by Valavan Manohararajah.

This version uses OpenGL to render the vectors, and SDL to handle the keyboard/controller input, and the audio streaming.

Portions of the source code of are copyright James Higgs 2005/2007.
These portions are:
1. Ay38910 PSG (audio) emulation wave-buffering code.
2. Drawing of vectors using OpenGL.

Comand-line parsing code gratefully borrowed from vecxsdl (Thomas Mathys).
Key mapping and command-line options were also changed to be compatible 
with Thomas Mathys' vecxsdl.


Controls:

Arrow keys	Vectrex joystick
A S D F		Buttons 1 to 4 on the Vectrex controller
Q or Esc	Quit
W		Toggle audio debug output on/off
P or SPACE	Pause


Command-line options:

-h              Displays help for VecXGL command-line options.

-b <file>       Load BIOS image from file.
                If this option is omitted, VecXGL will use
                a default BIOS.

-l <#>          Set line width. The default line width
                is 1. Other values may cause slowdown.

-o <file>	Use overlay TGA file. Can be 24 or 32 bit 
                compressed or uncompressed TGA.
                
-t <#>          Overlay transparency (actually opacity).
                Must be in the range [0.0, 1.0].
                Default is 0.5.
                
-x <#>          Window width (default is 330 pixel)

-y <#>          Window height (default is 410 pixel)

Usually you'll only specify one of the -x/-y parameters.
The other one is then calculated from the given one,
so that the aspect ratio of the window is that of a
vectrex display.

This version only supports overlays which are in 24 or 32 bit 
compressed or uncompressed TGA format. The overlay is 
converted to a 512x512 texture internally.

Other vecx ports by JH:
 - VecXPS2 (Playsyation 2)
 - VecXWin32 (Windows/DirectX) (unreleased)


- JH 29/7/2007
