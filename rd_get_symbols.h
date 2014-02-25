//
//  rd_get_symbols.h
//
//  Created by Dmitry Rodionov on 24/8/14.
//  Copyright (c) 2014 rodionovd. All rights reserved.
//
#include <stdint.h>

#ifndef RD_GET_SYMBOLS

struct rd_named_symbol {
    char *name;
    int8_t found:1;
    uintptr_t nvalue;
};

/**
 * Look up for the specified symbols inside a named image loaded into a current process' address space.
 *
 * This function will find any public or private function, unlike dlsym()/NSLookupSymbolInImage()
 * which do look up only for public available functions (but these functions may be used internally in
 * the implementation of the routine).
 *
 * @param  image_name the name if mach-o image
 * @param  count      (in/out) count of symbols to find out. Will be set to an actual count of found symbols
 * @param  symbols    a pointer/array of symbols which have the `name` field specifed;
 *
 * @return            KERN_SUCCESS if all passed symbols was found,
 *                    KERN_INVALID_ARGUMENT if any of input arguments was NULL/empty string/equal to 0,
 *                    KERN_INVALID_NAME if there arern't any mach-o image with such name loaded in current
 *                      address space,
 *                    KERN_INVALID_HOST if the found mach-o image header doesn't contain enough load commands
 *                      so we can't locate its symbols table.
 *
 */
int rd_get_symbols_from_image(const char *image_name,
                              int *count,
                              struct rd_named_symbol *symbols);
#endif
