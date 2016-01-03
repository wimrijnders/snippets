/**********************************************************************
 *
 * Example code for reading and writing a PNG image.
 *
 * Compile with:
 *
 * g++ testpng.cpp pngimage.cpp -I/usr/include/libpng16 -lpng -o testpng
 *
 **********************************************************************/
#include <iostream>
#include <string>
#include <sstream>
#include "pngimage.h"

int main(int argc, char *argv[]) {
	const std::string &path = argv[1];

	PNGIMage img;

	// Read it in

	if (!img.read(path.c_str())) {
		std::ostringstream buf;
		buf << "Could not load in png image. Reason: " << img.error() << std::endl;
		std::cerr << buf.str();
		return 1;
  }

	// Do something useful with it

	unsigned num_pixels = img.height() * img.width();
	float sum = 0.0;

	for (unsigned h = 0; h < img.height(); ++h) {
		for (unsigned w = 0; w < img.width(); ++w) {
			sum += img.get_grey(w, h);
		}
	}

	std::cout << "Grey average over " << num_pixels << " pixels: " << sum/num_pixels << std::endl;

	// Example for inline handling of image
	//img.process();

	// Store it

	img.write("example_output.png");

	return 0;
}
