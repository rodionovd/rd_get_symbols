//
//  rd_get_symbols.m
//
//  Created by Dmitry Rodionov on 24/8/14.
//  Copyright (c) 2014 rodionovd. All rights reserved.
//

#include <stdio.h>          // fprintf()
#include <string.h>         // strlen()
#include <mach-o/dyld.h>    // NSLookupSymbolInImage/NSAddressOfSymbol/NSSymbol
#include <mach-o/nlist.h>   // nlist/nlist_64

#include "rd_get_symbols.h"

#if defined(__x86_64__)
    typedef struct mach_header_64     mach_header_t;
    typedef struct segment_command_64 segment_command_t;
    #define LC_SEGMENT_ARCH_INDEPENDENT   LC_SEGMENT_64
    typedef struct nlist_64 nlist_t;
#else
    typedef struct mach_header        mach_header_t;
    typedef struct segment_command    segment_command_t;
#define LC_SEGMENT_ARCH_INDEPENDENT   LC_SEGMENT
    typedef struct nlist nlist_t;
#endif


static int _lookup_symbol_via_NSLookup(const char *image_name, const char *symbol_name, uintptr_t *pointer);


int rd_get_symbols_from_image(const char *image_name,
                              int *count,
                              struct rd_named_symbol *symbols)
{
    if (strlen(image_name) == 0 || count == NULL || symbols == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    if (*count == 0) {
        return KERN_INVALID_ARGUMENT;
    }

    /* An actual number of founded symbols */
    int real_count = 0;
    /**
     * First, try to find symbols via dyld API.
     */
    for (int i = 0; i < *count; i++) {
        uintptr_t tmp = 0;
        int err = _lookup_symbol_via_NSLookup(image_name, symbols[i].name, &tmp);
        if (err == KERN_SUCCESS) {
            symbols[i].nvalue = tmp;
            symbols[i].found = true;
            ++real_count;
        }
    }

    if (real_count == *count) {
        return KERN_SUCCESS;
    }

    /**
     * So NSLookup() failed and we have to parse a mach-o image to find
     * these symbols in its symbols list.
     */
    uint32_t image_count = _dyld_image_count();
    const mach_header_t *header = NULL;
    uintptr_t vm_slide = 0;

    for (uint32_t i = 0; i < image_count; i++) {
        if (0 == strcmp(image_name, _dyld_get_image_name(i))) {
            header = (const mach_header_t *)_dyld_get_image_header(i);
            vm_slide = _dyld_get_image_vmaddr_slide(i);
            break;
        }
    }
    if (header == NULL) {
        fprintf(stderr, "Image <%s> could not be found in the current process's address space. "
            "Please, check if it was even loaded correctly\n", image_name);
        return KERN_INVALID_NAME;
    }

    struct symtab_command *symtab = NULL;
	segment_command_t *seg_linkedit = NULL;
	segment_command_t *seg_text = NULL;

	struct load_command *command = (struct load_command *)(header+1);
	for (uint32_t i = 0; i < header->ncmds; i++) {
		switch(command->cmd) {
			case LC_SEGMENT_ARCH_INDEPENDENT:
			{
				if (0 == strcmp(((segment_command_t *)command)->segname, SEG_TEXT))
					seg_text = (segment_command_t *)command;
				else if (0 == strcmp(((segment_command_t *)command)->segname, SEG_LINKEDIT))
					seg_linkedit = (segment_command_t *)command;
				break;
			}
			case LC_SYMTAB:
			{
				symtab = (struct symtab_command *)command;
				break;
			}
			default: {}
		}

		command = (struct load_command *)((unsigned char *)command + command->cmdsize);
	}

	if ((NULL == seg_text) || (NULL == seg_linkedit) || (NULL == symtab)) {
		return KERN_INVALID_HOST;
	}

    intptr_t file_slide = ((intptr_t)seg_linkedit->vmaddr - (intptr_t)seg_text->vmaddr) - seg_linkedit->fileoff;
	intptr_t strings = (intptr_t)header + (symtab->stroff + file_slide);

	nlist_t *sym = (nlist_t *)((intptr_t)header + (symtab->symoff + file_slide));

    for (uint32_t i = 0; i < symtab->nsyms; i++, sym++)
	{
		if (!sym->n_value) continue;
        for (uint32_t k = 0; k < *count; k++) {
            if (symbols[k].found == true) {
                continue;
            }
            if (strcmp((const char *)strings + sym->n_un.n_strx, symbols[k].name) == 0)
            {
                symbols[k].nvalue = sym->n_value + vm_slide;
                ++real_count;
                break;
            }
        }
        continue;
	}

    /* Return KERN_SUCCESS if we've found all requested symbols */
    int result = (real_count == *count) ? KERN_SUCCESS : KERN_FAILURE;

    *count = real_count;
    return result;
}

/**
 * Private functions
 */
#pragma mark
#pragma mark Private

/**
 * Searches for a symbol in a named mach-o image loaded to the current task's
 * adress space. The searching is up to dyld's rouintes.
 *
 * @param  image_name  the name if mach-o image
 * @param  symbol_name the name of a requested symbol
 * @param  pointer     (out) address of the symbol (if found)
 *
 * @return             KERN_SUCESS upon success,
 *                     KERN_FAILURE if the symbol not be found,
 *                     KERN_INVALID_ARGUMENT if any of `image_name` or
 *                         `symbol_name` is invalid string
 */
static int _lookup_symbol_via_NSLookup(const char *image_name, const char *symbol_name, uintptr_t *pointer)
{
    if (strlen(image_name) == 0 || strlen(symbol_name) == 0) {
        KERN_INVALID_ARGUMENT;
    }
    bool did_found_image = false;
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        /**
         * This dyld API is deprecated as for 10.5-10.9.
         * @todo replace it with dlopen()/dlsym()
         */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"

        if (0 == strcmp(image_name, _dyld_get_image_name(i))) {
            did_found_image = true;
            NSSymbol target = NSLookupSymbolInImage(_dyld_get_image_header(i),
                symbol_name, NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);

            if (target) {
                *pointer = (uintptr_t)NSAddressOfSymbol(target);
            } else {
                return KERN_FAILURE;
            }
#pragma clang diagnostic pop

            break;
        }
    }

    return (did_found_image) ? KERN_SUCCESS : KERN_FAILURE;
}
