#ifndef PNGIMAGE_H
#define PNGIMAGE_H

#include <string>
#include <png.h>


/**
 * @brief Read and write PNG images
 *
 * Internally, the images are stored as 8-bit RGBA.
 *
 * Retained here because it is anything but trivial.
 *
 * Adapted from various sources.
 * Starting point: https://gist.github.com/niw/5963798
 */
class PNGIMage {
private:

  unsigned   m_width;
  unsigned   m_height;
  png_byte   m_color_type;
  png_byte   m_bit_depth;
  png_bytep *m_row_pointers;

  std::string m_error;

public:
  PNGIMage();
  ~PNGIMage();

  bool read(const char *filename);
  bool write(const char *filename);
  void process();

  unsigned width()  const { return m_width; }
  unsigned height() const { return m_height; }
  unsigned get_grey(unsigned x, unsigned y) const;

  const std::string &error() const { return m_error; }
  bool has_error() const { return m_error.empty(); }
};

#endif // PNGIMAGE_H
