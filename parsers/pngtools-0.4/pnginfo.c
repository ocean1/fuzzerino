/******************************************************************************
DOCBOOK START

FUNCTION pnginfo
PURPOSE display information on the PNG files named

SYNOPSIS START
pnginfo [-t] [-d] [-D] <filenames>
SYNOPSIS END

DESCRIPTION START
This command dumps information about the PNG files named on the command line. This command's output is based on the output of the <command>tiffinfo</command> command, which is part of the <command>libtiff</command> distribution. Each line output by the command represents a value that has been set within the PNG file.
</para>
<para>
The <command>-t</command> command line option forces pnginfo to use <emphasis>libtiff</emphasis> <command>tiffinfo</command> style lables, instead of the more relevant png names. The <command>-d</command> command line option dumps the bitmap contained by the image to standard out, whilst the <command>-D</command> command merely checks that the image bitmap could be extracted from the file. If nothing is reported by <command>-D</command>, then there was no error.
</para>
<para>
The format for the output bitmaps is hexadecimal, with each pixel presented as a triple -- for instance [red green blue]. This means that paletted images et cetera will have their bitmaps expanded before display.
DESCRIPTION END

RETURNS Nothing

EXAMPLE START
%bash: pnginfo -t toucan.png basn3p02.png basn6a16.png
toucan.png...
  Image Width: 162 Image Length: 150
  Bits/Sample: 8
  Samples/Pixel: 1
  Pixel Depth: 8
  Colour Type (Photometric Interpretation): PALETTED COLOUR with alpha (256 colours, 256 transparent) 
  Image filter: Single row per byte filter 
  Interlacing: Adam7 interlacing 
  Compression Scheme: Deflate method 8, 32k window
  Resolution: 0, 0 (unit unknown)
  FillOrder: msb-to-lsb
  Byte Order: Network (Big Endian)
  Number of text strings: 0 of 0

basn3p02.png...
  Image Width: 32 Image Length: 32
  Bits/Sample: 2
  Samples/Pixel: 1
  Pixel Depth: 2
  Colour Type (Photometric Interpretation): PALETTED COLOUR (4 colours, 0 transparent) 
  Image filter: Single row per byte filter 
  Interlacing: No interlacing 
  Compression Scheme: Deflate method 8, 32k window
  Resolution: 0, 0 (unit unknown)
  FillOrder: msb-to-lsb
  Byte Order: Network (Big Endian)
  Number of text strings: 0 of 0

basn6a16.png...
  Image Width: 32 Image Length: 32
  Bits/Sample: 16
  Samples/Pixel: 4
  Pixel Depth: 64
  Colour Type (Photometric Interpretation): RGB with alpha channel 
  Image filter: Single row per byte filter 
  Interlacing: No interlacing 
  Compression Scheme: Deflate method 8, 32k window
  Resolution: 0, 0 (unit unknown)
  FillOrder: msb-to-lsb
  Byte Order: Network (Big Endian)
  Number of text strings: 0 of 0

EXAMPLE END
SEEALSO libpng libtiff tiffinfo pngchunkdesc pngcp
DOCBOOK END
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <unistd.h>

void pnginfo_displayfile (char *, int, int, int);
void pnginfo_error (char *);
void *pnginfo_xmalloc (size_t);
void usage (void);

#define pnginfo_true 1
#define pnginfo_false 0

int
main (int argc, char *argv[])
{
  int i, optchar, extractBitmap = pnginfo_false, displayBitmap =
    pnginfo_false, tiffnames = pnginfo_false;

  // Initialise the argument that filenames start at
  i = 1;

  // Use getopt to determine what we have been asked to do
  while ((optchar = getopt (argc, argv, "tDd")) != -1)
    {
      switch (optchar)
	{
	case 't':
	  tiffnames = pnginfo_true;
	  i++;
	  break;

	case 'd':
	  displayBitmap = pnginfo_true;
	  extractBitmap = pnginfo_true;
	  i++;
	  break;

	case 'D':
	  extractBitmap = pnginfo_true;
	  i++;
	  break;

	case '?':
	default:
	  usage ();
	  break;
	}
    }

  // Determine if we were given a filename on the command line
  if (argc < 2)
    usage ();

  // For each filename that we have:
  for (; i < argc; i++)
    pnginfo_displayfile (argv[i], extractBitmap, displayBitmap, tiffnames);
}

void
pnginfo_displayfile (char *filename, int extractBitmap, int displayBitmap, int tiffnames)
{
  FILE *image;
  unsigned long imageBufSize, width, height, runlen;
  unsigned char signature;
  int bitdepth, colourtype;
  png_byte channels;
  int interlace_type, compression_type, filter_type;
  png_uint_32 res_x, res_y;
  int unit_type;
  png_uint_32 i, j, rowbytes;
  png_structp png;
  png_infop info;
  unsigned char sig[8];
  png_bytepp row_pointers = NULL;
  char *bitmap;

  printf ("%s%s...\n", filename, \
	  tiffnames == pnginfo_true? " (tiffinfo compatible labels)" : "");

  // Open the file
  if ((image = fopen (filename, "rb")) == NULL)
    pnginfo_error ("Could not open the specified PNG file.");

  // Check that it really is a PNG file
  fread (sig, 1, 8, image);
  if (!png_check_sig (sig, 8))
    {
      printf ("  This file is not a valid PNG file\n");
      fclose (image);
      return;
    }

  // Start decompressing
  if ((png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL,
				     NULL, NULL)) == NULL)
    pnginfo_error ("Could not create a PNG read structure (out of memory?)");

  if ((info = png_create_info_struct (png)) == NULL)
    pnginfo_error ("Could not create PNG info structure (out of memory?)");

  // If pnginfo_error did not exit, we would have to call 
  // png_destroy_read_struct

  if (setjmp (png_jmpbuf (png)))
    pnginfo_error ("Could not set PNG jump value");

  // Get ready for IO and tell the API we have already read the image signature
  png_init_io (png, image);
  png_set_sig_bytes (png, 8);
  png_read_info (png, info);
  png_get_IHDR (png, info, &width, &height, &bitdepth, &colourtype, &interlace_type, &compression_type, &filter_type);
  channels = png_get_channels(png, info);

  ///////////////////////////////////////////////////////////////////////////
  // Start displaying information
  ///////////////////////////////////////////////////////////////////////////

  printf ("  Image Width: %d Image Length: %d\n", width, height);
  if(tiffnames == pnginfo_true){
    printf ("  Bits/Sample: %d\n", bitdepth);
    printf ("  Samples/Pixel: %d\n", channels);
    printf ("  Pixel Depth: %d\n",channels * bitdepth);	// Does this add value?
  }
  else{
    printf ("  Bitdepth (Bits/Sample): %d\n", bitdepth);
    printf ("  Channels (Samples/Pixel): %d\n", channels);
    printf ("  Pixel depth (Pixel Depth): %d\n",  channels * bitdepth);	// Does this add value?
  }

  png_bytep trans_alpha;
  int num_trans;
  png_color_16p trans_color;
  png_colorp palette;
  int num_palette;

  png_get_tRNS(png, info, &trans_alpha, &num_trans, &trans_color);
  png_get_PLTE(png, info, &palette, &num_palette);

#if PNG_LIBPNG_VER_MAJOR >= 1 && PNG_LIBPNG_VER_MINOR >= 4
 png_get_pHYs_dpi(png, info, &res_x, &res_y, &unit_type);
#else
 res_x = info->x_pixels_per_unit;
 res_y = info->y_pixels_per_unit;
#endif

  // Photometric interp packs a lot of information
  printf ("  Colour Type (Photometric Interpretation): ");

  switch (colourtype)
    {
    case PNG_COLOR_TYPE_GRAY:
      printf ("GRAYSCALE ");
      break;

    case PNG_COLOR_TYPE_PALETTE:
      printf ("PALETTED COLOUR ");
      if (num_trans > 0)
	printf ("with alpha ");
      printf ("(%d colours, %d transparent) ",
	      num_palette, num_trans);
      break;

    case PNG_COLOR_TYPE_RGB:
      printf ("RGB ");
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
      printf ("RGB with alpha channel ");
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      printf ("GRAYSCALE with alpha channel ");
      break;

    default:
      printf ("Unknown photometric interpretation!");
      break;
    }
  printf ("\n");

  printf ("  Image filter: ");
  switch (filter_type)
    {
    case PNG_FILTER_TYPE_BASE:
      printf ("Single row per byte filter ");
      break;

    case PNG_INTRAPIXEL_DIFFERENCING:
      printf ("Intrapixel differencing (MNG only) ");
      break;

    default:
      printf ("Unknown filter! ");
      break;
    }
  printf ("\n");

  printf ("  Interlacing: ");
  switch (interlace_type)
    {
    case PNG_INTERLACE_NONE:
      printf ("No interlacing ");
      break;

    case PNG_INTERLACE_ADAM7:
      printf ("Adam7 interlacing ");
      break;

    default:
      printf ("Unknown interlacing ");
      break;
    }
  printf ("\n");

  printf ("  Compression Scheme: ");
  switch (compression_type)
    {
    case PNG_COMPRESSION_TYPE_BASE:
      printf ("Deflate method 8, 32k window");
      break;

    default:
      printf ("Unknown compression scheme!");
      break;
    }
  printf ("\n");

  printf ("  Resolution: %d, %d ",res_x, res_y);
  switch (unit_type)
    {
    case PNG_RESOLUTION_UNKNOWN:
      printf ("(unit unknown)");
      break;

    case PNG_RESOLUTION_METER:
      printf ("(pixels per meter)");
      break;

    default:
      printf ("(Unknown value for unit stored)");
      break;
    }
  printf ("\n");

  // FillOrder is always msb-to-lsb, big endian
  printf ("  FillOrder: msb-to-lsb\n  Byte Order: Network (Big Endian)\n");

  png_textp text_ptr;
  int num_text;
  png_get_text(png, info, &text_ptr, &num_text);

  // Text comments
  printf ("  Number of text strings: %d\n",
	  num_text);

  for (i = 0; i < num_text; i++)
    {
      printf ("    %s ", text_ptr[i].key);

      switch (text_ptr[1].compression)
	{
	case -1:
	  printf ("(tEXt uncompressed)");
	  break;

	case 0:
	  printf ("(xTXt deflate compressed)");
	  break;

	case 1:
	  printf ("(iTXt uncompressed)");
	  break;

	case 2:
	  printf ("(iTXt deflate compressed)");
	  break;

	default:
	  printf ("(unknown compression)");
	  break;
	}

      printf (": ");
      j = 0;
      while (text_ptr[i].text[j] != '\0')
	{
	  if (text_ptr[i].text[j] == '\n')
	    printf ("\\n");
	  else
	    fputc (text_ptr[i].text[j], stdout);

	  j++;
	}

      printf ("\n");
    }

  // Print a blank line
  printf ("\n");

  // Do we want to extract the image data? We are meant to tell the user if
  // there are errors, but we don't currently trap any errors here -- I need
  // to look into this
  if (extractBitmap == pnginfo_true)
    {
      // This will force the samples to be packed to the byte boundary
      if(bitdepth < 8)
	png_set_packing(png);

      if (colourtype == PNG_COLOR_TYPE_PALETTE)
	png_set_expand (png);

      // png_set_strip_alpha (png);
      png_read_update_info (png, info);

      rowbytes = png_get_rowbytes (png, info);
      bitmap = (unsigned char *) pnginfo_xmalloc ((rowbytes * height) + 1);
      row_pointers = pnginfo_xmalloc (height * sizeof (png_bytep));

      // Get the image bitmap
      for (i = 0; i < height; ++i)
	row_pointers[i] = bitmap + (i * rowbytes);
      png_read_image (png, row_pointers);
      free (row_pointers);
      png_read_end (png, NULL);

      // Do we want to display this bitmap?
      if (displayBitmap == pnginfo_true)
	{
	  int bytespersample;

	  bytespersample = bitdepth / 8;
	  if(bitdepth % 8 != 0)
	    bytespersample++;

	  printf ("Dumping the bitmap for this image:\n");
	  printf ("(Expanded samples result in %d bytes per pixel, %d channels with %d bytes per channel)\n\n", 
		  channels * bytespersample, channels, bytespersample);

	  // runlen is used to stop us displaying repeated byte patterns over and over --
	  // I display them once, and then tell you how many times it occured in the file.
	  // This currently only applies to runs on zeros -- I should one day add an
	  // option to extend this to runs of other values as well
	  runlen = 0;
	  for (i = 0; i < rowbytes * height / channels; i += channels * bytespersample)
	    {
	      int scount, bcount, pixel;

	      if ((runlen != 0) && (bitmap[i] == 0) && (bitmap[i] == 0)
		  && (bitmap[i] == 0))
		runlen++;
	      else if (runlen != 0)
		{
		  if (runlen > 1)
		    printf ("* %d ", runlen);
		  runlen = 0;
		}

	      // Determine if this is a pixel whose entire value is zero
	      pixel = 0;
	      for(scount = 0; scount < channels; scount++)
		for(bcount = 0; bcount < bytespersample; bcount++)
		  pixel += bitmap[i + scount * bytespersample + bcount];

	      if ((runlen == 0) && !pixel)
		{
		  printf ("[");
		  for(scount = 0; scount < channels; scount++){
		    for(bcount = 0; bcount < bytespersample; bcount++) printf("00");
		    if(scount != channels - 1) printf(" ");
		  }
		  printf ("] ");
		  runlen++;
		}

	      if (runlen == 0){
		printf ("[");
		for(scount = 0; scount < channels; scount++){
		  for(bcount = 0; bcount < bytespersample; bcount++)
		    printf("%02x", (unsigned char) bitmap[i + scount * bytespersample + bcount]);
		  if(scount != channels - 1) printf(" ");
		}
		printf("] ");
	      }
	      
	      // Perhaps one day a new row should imply a line break here?
	    }
	  
	  printf("\n");
	}
    }

  // This cleans things up for us in the PNG library
  fclose (image);
  png_destroy_read_struct (&png, &info, NULL);
}

// You can bang or head or you can drown in a hole
//                                                    -- Vanessa Amarosi, Shine
void
pnginfo_error (char *message)
{
  fprintf (stderr, "%s\n", message);
  exit (42);
}

// Allocate some memory
void *
pnginfo_xmalloc (size_t size)
{
  void *buffer;

  if ((buffer = malloc (size)) == NULL)
    {
      pnginfo_error ("pnginfo_xmalloc failed to allocate memory");
    }

  return buffer;
}

void
usage ()
{
  pnginfo_error ("Usage: pnginfo [-d] [-D] <filenames>");
}
