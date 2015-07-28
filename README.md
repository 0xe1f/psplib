psplib
======

**psplib** is a lightweight library written to ease porting of existing
software (mainly emulators) to Sony PSPs capable of running homebrew software.

It includes routines to do native sound and video rendering, as well as basic
UI features such as menus, scrolling lists, labels and status bars.


This repository is a re-release of the library under a more permissive license.
The original was licensed under GPL, which is likely to be problematic given
the spectrum of licenses that accompany existing software. Except for the
license, this is the same version as the one that can be found in the
[original SVN repository](http://svn.akop.org/psp/trunk/libpsp).

Dependencies
------------

In addition to the standard C libraries, programs using psplib should be linked
against the following PSP libraries: 

psplib depends on the following PSP libraries:

```
pspgu pspaudio psprtc psppower pspwlan pspnet_adhoc
pspnet_adhocctl pspnet_adhocmatching
```

as well as:

```
png z
```

Existing Software
-----------------

A number of emulators use various versions of psplib:

[fMSX PSP](http://psp.akop.org/fmsx)  
[ColEm PSP](http://psp.akop.org/colem)  
[SMS Plus PSP](http://psp.akop.org/smsplus)  
[Atari800 PSP](http://psp.akop.org/atari800)  
[NeoPop PSP](http://psp.akop.org/neopop)  
[Handy PSP](http://psp.akop.org/handy)  
[Caprice32 PSP](http://psp.akop.org/caprice32)  
[Fuse PSP](http://psp.akop.org/fuse)  
[RACE! PSP](http://psp.akop.org/race)  
[VICE PSP](http://psp.akop.org/vice)  

Vita Port
---------

[Francisco José García García](https://github.com/frangarcj) has ported the
library to [PSP Vita](https://github.com/frangarcj/psplib4vita).

License
-------
psplib is written by Akop Karapetyan.
Licensed under the
[Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).
