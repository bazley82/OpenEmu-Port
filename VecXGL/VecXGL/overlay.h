// VecXGL 1.1 (SDL/Win32)
// Overlay info
// JH 2007

#import <OpenGL/gl.h>

typedef struct								// Create A Structure
{
	GLubyte	*imageData;						// Image Data (Up To 32 Bits)
	GLuint	bpp;							// Image Color Depth In Bits Per Pixel.
	GLuint	width;							// Image Width
	GLuint	height;							// Image Height
	GLuint	upsideDown;						// If 1, then image is upside down
	GLuint	texID;							// Texture ID Used To Select A Texture
} TextureImage;
