// VecXGL 1.2 (SDL/Win32)
// 
// TGA - loading code
// Can load 24-bit RLE compressed or uncompressed images
//
// Most of this code from NeHe example.
// RLE-decompressing portion by James Higgs 2007
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#import <OpenGL/gl.h>
#import <OpenGL/glu.h>
#include "overlay.h"

extern TextureImage g_overlay;										// Storage For One Texture

// Load TGA for texture
int LoadTGA (char *filename)			// Loads A TGA File Into Memory
{
	GLubyte		header[18];									// TGA Header
	GLubyte		tgaType;									// 2 = uncompr RGB, 10 = compr RGB
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
	
	if ( fread(header,1,sizeof(header),file)!=sizeof(header) )
	{
		fclose(file);										// If Anything Failed, Close The File
		return 0;											// Return False
	}
    
	tgaType = header[2];
	if (tgaType != 2 && tgaType != 10)
	{
		fclose(file);
		return 0;
	}

	g_overlay.width  = header[12] + 256 * header[13];			// Determine The TGA Width	(highbyte*256+lowbyte)
	g_overlay.height = header[14] + 256 * header[15];			// Determine The TGA Height	(highbyte*256+lowbyte)
    
 	if(	g_overlay.width	<=0	||								// Is The Width Less Than Or Equal To Zero
		g_overlay.height	<=0	||								// Is The Height Less Than Or Equal To Zero
		(header[16]!=24 && header[16]!=32))					// Is The TGA 24 or 32 Bit?
	{
		fclose(file);										// If Anything Failed, Close The File
		return 0;										// Return False
	}

	g_overlay.bpp	= header[16];							// Grab The TGA's Bits Per Pixel (24 or 32)
	bytesPerPixel	= g_overlay.bpp/8;						// Divide By 8 To Get The Bytes Per Pixel
	imageSize		= g_overlay.width*g_overlay.height*bytesPerPixel;	// Calculate The Memory Required For The TGA Data
	g_overlay.upsideDown = (header[17] & 0x20) ? 0 : 1;		// If TGA origin is bottom-left

	if ( 2 == tgaType )
	{
		// uncompressed RGB (24 or 32-bit)
		g_overlay.imageData=(GLubyte *)malloc(imageSize);		// Reserve Memory To Hold The TGA Data

		if(	g_overlay.imageData==NULL ||							// Does The Storage Memory Exist?
			fread(g_overlay.imageData, 1, imageSize, file)!=imageSize)	// Does The Image Size Match The Memory Reserved?
		{
			if(g_overlay.imageData!=NULL)						// Was Image Data Loaded
				free(g_overlay.imageData);						// If So, Release The Image Data

			fclose(file);										// Close The File
			return 0;											// Return False
		}

		for (i=0; i < imageSize; i += bytesPerPixel)
		{
			temp = g_overlay.imageData[i];						// Temporarily Store The Value At Image Data 'i'
			g_overlay.imageData[i] = g_overlay.imageData[i + 2];	// Set The 1st Byte To The Value Of The 3rd Byte
			g_overlay.imageData[i + 2] = temp;					// Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
		}
	}
	else if (10 == tgaType)
	{
		// RLE compressed RGB (24 or 32-bit)
		g_overlay.imageData=(GLubyte *)malloc(imageSize);		// Reserve Memory To Hold The TGA Data
		pos = 0;
		while(1)
		{
			// read packet header
			unsigned char ph; // = fgetc(file);
			if (fread(&ph, 1, 1, file)!=1)
			{
				fclose(file);
				return 0;
			}
			// packet type
			if (ph & 0x80)
			{
				// run-length packet
				unsigned int len = (ph & 0x7F) + 1;
				fread(value, 1, bytesPerPixel, file);
				for (i=0; i<len; i++)
				{
					g_overlay.imageData[pos] = value[2];
					g_overlay.imageData[pos+1] = value[1];
					g_overlay.imageData[pos+2] = value[0];
					pos += bytesPerPixel;
				}
			}
			else
			{
				// "raw" packet
				unsigned int len = (ph & 0x7F) + 1;
				for (i=0; i<len; i++)
				{
					fread(value, 1, bytesPerPixel, file);
					g_overlay.imageData[pos] = value[2];
					g_overlay.imageData[pos+1] = value[1];
					g_overlay.imageData[pos+2] = value[0];
					pos += bytesPerPixel;
				}
			}
			if (pos >= imageSize)
				break;
		}

	}

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
