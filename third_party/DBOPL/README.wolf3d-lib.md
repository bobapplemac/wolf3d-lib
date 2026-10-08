# DBOPL C port

`dbopl.c` and `dbopl.h` come from PrBoom+ commit
`969515162c5aebea4ad7a125ee178dcad3d576ad`. PrBoom+ identifies this as
DOSBox DBOPL revision 3635, converted from C++ to C using Chocolate Doom's
`minus-minus` conversion script and subsequent manual corrections.

Upstream source:
<https://github.com/coelckers/prboom-plus/tree/969515162c5aebea4ad7a125ee178dcad3d576ad/prboom2/src/MUSIC>

The files are licensed under GPL-2.0-or-later. wolf3d-lib changes are limited
to replacing the incidental SDL fixed-width type include with
`DBOPL_COMPAT.h`, which also supports pre-C99 Microsoft compilers.
