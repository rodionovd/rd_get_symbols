#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <sys/param.h>
#include <mach-o/dyld.h>
#include "rd_get_symbols.h"

#define kLibDyldPath "/usr/lib/system/libdyld.dylib"

int main(int argc, char const *argv[])
{
	struct rd_named_symbol symbols[2] = {
		{"__dyld_get_image_header"}, // note an extra "_" prefix: that's how symbols are actually
		{"__dyld_get_image_name"}    // named inside an executable;
	};
	int count = 2;
	int err = rd_get_symbols_from_image(kLibDyldPath, &count, symbols);

	if (err != 0 || count != 2) {
		fprintf(stderr, "rd_get_symbols_from_image(%d) failed: %d\n",
			count, err);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Found [_dyld_get_image_header] at %p\n"
		"Found [_dyld_get_image_name] at %p\n",
		(void *)symbols[0].nvalue, (void *)symbols[1].nvalue);

	void* (*my_get_image_header)(uint32_t) = (void *)symbols[0].nvalue;
	void* (*my_get_image_name)  (uint32_t) = (void *)symbols[1].nvalue;

	uint32_t idx = 0;
	assert(my_get_image_header(idx) ==_dyld_get_image_header(idx));
	assert(0 == strcmp(my_get_image_name(idx), _dyld_get_image_name(idx)));

	fprintf(stderr, "All test passed (^^,)\n");

	return EXIT_SUCCESS;
}
