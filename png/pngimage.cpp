#include <stdlib.h>
#include <iostream>  // For cout
#include <stdio.h>
#include "pngimage.h"

using std::cout;
using std::endl;


struct png_data {
private:
  bool create_info() {
    info = png_create_info_struct(png);
    if (!info) {
      m_error = "Could not create info struct";
      return false;
    }

/* Doesn't work with given write() - we don't need it anyway
    end_info = png_create_info_struct(png);
    if (!end_info) {
      m_error = "Could not create end info struct";
      return false;
    }
*/
    return true;
  }


public:
  std::string m_error;

  FILE *fp;
  png_structp png;
  png_infop   info;
  png_infop   end_info;
  bool        m_reading;

  png_data(): fp(0), png(0), info(0), end_info(0), m_reading(true) {}

  ~png_data() {
    png_structpp param1 = (png_structpp) 0;
    png_infopp   param2 = (png_infopp) 0;
    png_infopp   param3 = (png_infopp) 0;

    if (png) param1 = &png;
    if (info) param2 = &info;
    if (end_info) param3 = &end_info;

    if(m_reading) {
      png_destroy_read_struct(param1, param2, param3);
    } else {
      png_destroy_write_struct(param1, param2);
    }

    png  = 0;
    info = end_info = 0;

    if (fp != 0) {
      fclose(fp);
      fp = 0;
    }

  }


  bool create_read() {
    m_reading = true;
    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png) {
      m_error = "Could not create read png struct";
      return false;
    }

    return create_info();
  }


  bool create_write() {
    m_reading = false;
    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png) {
      m_error = "Could not create read png struct";
      return false;
    }

    return create_info();
  }
};


PNGIMage::PNGIMage() :
  m_width(0),
  m_height(0),
  m_row_pointers(0)
{}


PNGIMage::~PNGIMage() {
  if (m_row_pointers == 0) {
    return;
  }

  for(unsigned y = 0; y < m_height; y++) {
    free(m_row_pointers[y]);
  }

  free(m_row_pointers);
}


/**
 *
 * Source: http://cr.yp.to/2004-494/libpng/libpng-1.2.5/libpng.txt
 */
int read_chunk_callback(png_structp ptr, png_unknown_chunkp chunk) {
  /* The unknown chunk structure contains your
     chunk data:

       png_byte name[5];
       png_byte *data;
       png_size_t size;
   */
  /* Note that libpng has already taken care of
     the CRC handling */

  /* put your code here.  Return one of the
      following:

   		return (-n); // chunk had an error
   		return (0);  // did not recognize
   		return (n);  // success
	 */

   cout << "Unknown chunk: " << chunk->name << "; length: " << chunk->size << endl;

   return (0); // did not recognize
}


bool PNGIMage::read(const char *filename) {
  png_data data;

  data.fp = fopen(filename, "rb");
  if(!data.fp) {
    m_error = "Could not open file for reading.";
    return false;
  }

  // Check if file read in is png
	const unsigned HEADER_SIZE = 8;
  unsigned char header[HEADER_SIZE];
  size_t num_read = fread(header, 1, HEADER_SIZE, data.fp);

  // Follow returns zero if png detected
  if (png_sig_cmp(header, 0, HEADER_SIZE) ) {
    m_error = "Not a PNG file";
    return false;
  }

  // All is well - skip header in further processing

  // Following from docs, doesn't work - lngjmp 'not a png file'
  //png_set_sig_bytes(data.png, num_read);
  //fseek(data.fp, 0 , SEEK_SET);

  if (!data.create_read()) {
    m_error = data.m_error;
    return false;
  }

  // Following crummy, obsolete construction is used by libpng
  // to signal errors during processing.
  if(setjmp(png_jmpbuf(data.png))) {
    m_error = "jmpbuf returned with an error";
    return false;
  }

  png_init_io(data.png, data.fp);

  // Skip header bytes used for detecting png.
  png_set_sig_bytes(data.png, num_read);

  png_set_read_user_chunk_fn(data.png, png_get_user_chunk_ptr(data.png), read_chunk_callback);

  //png_read_png(data.png, data.info, PNG_TRANSFORM_IDENTITY, NULL);
  png_read_info(data.png, data.info);

  m_width      = png_get_image_width(data.png, data.info);
  m_height     = png_get_image_height(data.png, data.info);
  m_color_type = png_get_color_type(data.png, data.info);
  m_bit_depth  = png_get_bit_depth(data.png, data.info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(m_bit_depth == 16)
    png_set_strip_16(data.png);

  if(m_color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(data.png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(m_color_type == PNG_COLOR_TYPE_GRAY && m_bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(data.png);

  if(png_get_valid(data.png, data.info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(data.png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(m_color_type == PNG_COLOR_TYPE_RGB ||
     m_color_type == PNG_COLOR_TYPE_GRAY ||
     m_color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(data.png, 0xFF, PNG_FILLER_AFTER);

  if(m_color_type == PNG_COLOR_TYPE_GRAY ||
     m_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(data.png);

  /* int number_of_passes = */ png_set_interlace_handling(data.png);

  png_read_update_info(data.png, data.info);

  m_row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * m_height);
  for(unsigned y = 0; y < m_height; y++) {
    m_row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(data.png,data.info));
  }

  png_read_image(data.png, m_row_pointers);
  png_read_end(data.png, data.end_info);

  return true;
}


bool PNGIMage::write(const char *filename) {
  png_data data;

  data.fp = fopen(filename, "wb");
  if(!data.fp) {
    m_error = "Could not open file for writing.";
    return false;
  }

  if (!data.create_write()) {
    m_error = data.m_error;
    return false;
  }

  if(setjmp(png_jmpbuf(data.png))) {
    m_error = "jmpbuf returned with an error during write";
    return false;
  }

  png_init_io(data.png, data.fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    data.png,
    data.info,
    m_width, m_height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(data.png, data.info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  png_write_image(data.png, m_row_pointers);
  png_write_end(data.png, data.info);

  return true;
}


/**
 * @brief Return greyscale value from given coordinate.
 */
unsigned PNGIMage::get_grey(unsigned x, unsigned y) const {

  if (x >= width()) return 0;
  if (y >= height()) return 0;


  png_bytep row = m_row_pointers[y];
  png_bytep px  = &(row[x * 4]);

  // Alpha channel ignored
  return (px[0]+ px[1]+ px[2])/3;
}


/*
 * Example from original
 */
void PNGIMage::process() {
  for(int y = 0; y < m_height; y++) {
    png_bytep row = m_row_pointers[y];
    for(int x = 0; x < m_width; x++) {
      png_bytep px = &(row[x * 4]);
      // Do something awesome for each pixel here...
      printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
    }
  }
}
