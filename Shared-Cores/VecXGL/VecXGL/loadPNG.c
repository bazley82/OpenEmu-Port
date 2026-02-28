// VecXGL 1.1 (SDL/Win32)
// 
// PNG - loading code
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#include "overlay.h"
#include "/usr/X11/include/png.h"

extern TextureImage g_overlay;										// Storage For One Texture

// Load PNG for texture
int LoadPNG (char *filename)			// Loads A PNG File Into Memory
{
	char		header[8];
	GLuint		bytesPerPixel;								// Holds Number Of Bytes Per Pixel Used In The TGA File
	GLuint		imageSize;									// Used To Store The Image Size When Setting Aside Ram
	GLuint		temp;										// Temporary Variable
	GLuint		type = GL_RGBA;								// Set The Default GL Mode To RBGA (32 BPP)
	GLuint		i;
	GLuint		pos = 0;
	unsigned char value[4];

	FILE *file = fopen(filename, "rb");						// Open The TGA File

	if ( file==NULL )
		return 0;

	// Check if this is indeed a PNG file
	fread(header, 1, 8, file);
	if (png_sig_cmp(header, 0, 8))
		{
		fclose(file);
		return 0;
		}

	// Initialise PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		{
		fclose(file);
		return 0;
		}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		{
		fclose(file);
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return 0;
		}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info)
		{
		fclose(file);
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return 0;
		}

	// Init PNG file i/o
	png_init_io(png_ptr, file);

	// We've already read the header (8 bytes)
	png_set_sig_bytes(png_ptr, 8);

	// Read the image into memory
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	

	g_overlay.width  = png_ptr->width;			// Determine The PNG Width
	g_overlay.height = png_ptr->height;			// Determine The PNG Height
    
 	if(	g_overlay.width	<=0	||								// Is The Width Less Than Or Equal To Zero
		g_overlay.height <=0)								// Is The Height Less Than Or Equal To Zero
	{
		fclose(file);										// If Anything Failed, Close The File
		return 0;										// Return False
	}

	g_overlay.bpp	= png_ptr->bit_depth * png_ptr->channels;	// Bits per pixel (24 or 32)
	bytesPerPixel	= g_overlay.bpp/8;						// Divide By 8 To Get The Bytes Per Pixel
	imageSize		= g_overlay.width*g_overlay.height*bytesPerPixel;	// Calculate The Memory Required For The TGA Data
	//g_overlay.upsideDown = (header[17] & 0x20) ? 0 : 1;		// If TGA origin is bottom-left

	g_overlay.imageData = (GLubyte *)png_ptr->row_buf[0];		// Reserve Memory To Hold The TGA Data



	//if ( 2 == tgaType )
	//{
	//	// uncompressed RGB (24 or 32-bit)
	//	g_overlay.imageData=(GLubyte *)malloc(imageSize);		// Reserve Memory To Hold The TGA Data

	//	if(	g_overlay.imageData==NULL ||							// Does The Storage Memory Exist?
	//		fread(g_overlay.imageData, 1, imageSize, file)!=imageSize)	// Does The Image Size Match The Memory Reserved?
	//	{
	//		if(g_overlay.imageData!=NULL)						// Was Image Data Loaded
	//			free(g_overlay.imageData);						// If So, Release The Image Data

	//		fclose(file);										// Close The File
	//		return 0;											// Return False
	//	}

	//	for (i=0; i < imageSize; i += bytesPerPixel)
	//	{
	//		temp = g_overlay.imageData[i];						// Temporarily Store The Value At Image Data 'i'
	//		g_overlay.imageData[i] = g_overlay.imageData[i + 2];	// Set The 1st Byte To The Value Of The 3rd Byte
	//		g_overlay.imageData[i + 2] = temp;					// Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
	//	}
	//}
	//else if (10 == tgaType)
	//{
	//	// RLE compressed RGB (24 or 32-bit)
	//	g_overlay.imageData=(GLubyte *)malloc(imageSize);		// Reserve Memory To Hold The TGA Data
	//	pos = 0;
	//	while(1)
	//	{
	//		// read packet header
	//		unsigned char ph; // = fgetc(file);
	//		if (fread(&ph, 1, 1, file)!=1)
	//		{
	//			fclose(file);
	//			return 0;
	//		}
	//		// packet type
	//		if (ph & 0x80)
	//		{
	//			// run-length packet
	//			unsigned int len = (ph & 0x7F) + 1;
	//			fread(value, 1, bytesPerPixel, file);
	//			for (i=0; i<len; i++)
	//			{
	//				g_overlay.imageData[pos] = value[2];
	//				g_overlay.imageData[pos+1] = value[1];
	//				g_overlay.imageData[pos+2] = value[0];
	//				pos += bytesPerPixel;
	//			}
	//		}
	//		else
	//		{
	//			// "raw" packet
	//			unsigned int len = (ph & 0x7F) + 1;
	//			for (i=0; i<len; i++)
	//			{
	//				fread(value, 1, bytesPerPixel, file);
	//				g_overlay.imageData[pos] = value[2];
	//				g_overlay.imageData[pos+1] = value[1];
	//				g_overlay.imageData[pos+2] = value[0];
	//				pos += bytesPerPixel;
	//			}
	//		}
	//		if (pos >= imageSize)
	//			break;
	//	}

	//}

	fclose (file);											// Close The File

	// resize if neccessary
	if (g_overlay.width != 512 || g_overlay.height != 512)
	{
		GLubyte* newImageData = (GLubyte *)malloc(512*512*bytesPerPixel);		// Reserve Memory To Hold The RGB Data
		gluScaleImage(GL_RGB, g_overlay.width, g_overlay.height, GL_UNSIGNED_BYTE, g_overlay.imageData, 
						512, 512, GL_UNSIGNED_BYTE, newImageData);
		free(g_overlay.imageData);
		g_overlay.imageData = newImageData;
		g_overlay.width = 512;
		g_overlay.height = 512;
	}

	// Build A Texture From The Data
	glGenTextures(1, &g_overlay.texID);					// Generate OpenGL texture IDs

	glBindTexture(GL_TEXTURE_2D, g_overlay.texID);			// Bind Our Texture
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtered
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtered
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	if (g_overlay.bpp==24)									// Was The TGA 24 Bits
	{
		type=GL_RGB;										// If So Set The 'type' To GL_RGB
	}

	glTexImage2D(GL_TEXTURE_2D, 0, type, g_overlay.width, g_overlay.height, 0, type, GL_UNSIGNED_BYTE, g_overlay.imageData);

	return 1;											// Texture Building Went Ok, Return True
}
