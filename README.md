Loking for a way to private APIs?
======


`rd_get_symbols` is a modern API aiming to replace Apple's `nlist()`, so you're able to find out any private or public symbol in any Mach-O executable.  
Unfortunately, `dlsym()` is unable to see non-public symbols, so it can't help hacking private APIs.  
`NSLookupSymbol*` has nothing to do here too, so `nlist()` is the last hope. But wait, it  

 1)  is unavailable for x86_64 executables *(what's wrong with you, Apple? It's 2014)*  
 2)  loades executable file from disk, instead of in-memory lookup if this executable was already loaded into a current process' address space.    

###### It works on both `i386` and `x86_64` binaries.


## Exported functions  

### rd_get_symbols_from_image()
`int rd_get_symbols_from_image(const char *image_name, int *count, struct rd_named_symbol *symbols);`  

Look up for the specified symbols inside a named image loaded into a current process' address space.  

This function will find any public or private function, unlike `dlsym()`/`NSLookupSymbolInImage()`
which do look up only for public available functions (but these functions may be used internally in
the implementation of the routine).


##### Arguments  

 Argument   | Type (in/out) | Description  
 :--------  | :-----------: | :----------  
 `image_name` | in  | the name if mach-o image  
 `count` | in/out | count of symbols to find out. Will be set to an actual count of found symbols
 `symbols` | in | a pointer/array of symbols which have the `name` field specifed

##### Return values

 Value   | Condition 
 :-------- | :-----------
`KERN_SUCCESS` | if all passed symbols was found,
`KERN_INVALID_ARGUMENT` | if any of input arguments was NULL/empty string/equal to 0
`KERN_INVALID_NAME` | if there arern't any mach-o image with such name loaded in current address space
`KERN_INVALID_HOST` | if the found mach-o image header doesn't contain enough load commands so we can't locate its symbols table  


##### Usage 

_(see `main.c` for the complete example code)_
```objc
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
```

------

If you found any bug(s) or something, please open an issue or a pull request — I'd appreciate your help! `(^,,^)`

*Dmitry Rodionov, 2014*  
*i.am.rodionovd@gmail.com*
