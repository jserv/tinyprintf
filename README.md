# A tiny `printf` for embedded applications

This package was originally from http://www.sparetimelabs.com/tinyprintf/index.html

The library consists of approximately 200 lines of code and has a low memory
footprint. It is compact enough to be used in an 8-bit microcontroller with just
a few kilobytes of memory.

To integrate the library, simply download the two provided files and include
them in the project's build process. The source code should include the header
file `printf.h`, allowing the use of the `printf` function in a manner similar
to the standard `stdio.h`. Being minimalistic, the library has certain
limitations: floating-point support is not included, and support for long
integers is optional. However, it provides the most essential features for
embedded development, such as printing characters, strings, and decimal and
hexadecimal numbers.

Before using the `printf` function, the library must be initialized by supplying
it with a character output function. Instead of relying on the traditional
approach of calling a predefined `putc` function, this library avoids presuming
the existence of any specific headers or external declarations. Instead, the user
must pass a pointer to the desired `putc` function directly. The header file
contains an example, and further details can be found in the source code.

The library implements two functions: `tfp_printf` and `tfp_sprintf`. Additionally,
it provides two macros, `printf` and `sprintf`, which expand to these function
names. Although this non-function-style macro usage, especially with lower-case
names, can be considered suboptimal, the approach was chosen for simplicity.
A more modern and clean alternative would involve variadic macros, which can be
adapted if needed.

To conserve space, the library does not include long integer support unless the
macro `PRINTF_LONG_SUPPORT` is defined. Enabling this feature causes the compiler
to pull in 32-bit math libraries (assuming `long` is 32 bits), significantly
increasing memory usage. For cases where 32-bit hex values need to be printed
without enabling long support, the following approach can be used:
```c
long v = 0xDEADBEEF;
printf("v = %04X%04X\n", v >> 16, v & 0xFFFF);
```

NOTE: The `& 0xFFFF` may be unnecessary if `int` is 16 bits.

This will output:
`v = DEADBEEF`

The library intentionally deviates from standard compliance in one area: the
return type of `printf` is `void` instead of `int`. This decision aligns with
the goal of keeping the library lightweight. In most cases, the return value of
`printf` is unused, making its inclusion unnecessary for this application.

## License

This library can be licensed either under MIT, BSD or LGPL license.

Copyright (C) 2004,2012  Kustaa Nyholm
Licenses: LGPL 2.1 or later (see `LICENSE.LGPL-2.1`)
      or  BSD               (see `LICENSE.BSD`)
      or  MIT               (see `LICENSE.MIT`)
