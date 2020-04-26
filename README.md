# PCECD_seek

The PC Engine is a video game machine which was released in 1987 in Japan, with
a CDROM attachment released prior to 1990, before many of the CDROM standards we
know today were created.

This CDROM was 1x speed, and a full-disc seek took over 1 second to execute.
Many of the games were written to take this timing into account, and either wait
for an interval, or even exeute some work during that period.  If the operation
were to complete more quickly - as it does on nearly all modern emulation - that
interim work would be incomplete when the operation finishes.

In either scneario, the user is left with a substandard experience.


This project details a collection of measurements and tools I used to check the
actual time required for CDROM seeks on original (but re-capped) hardware.

Notes:
------
measurements/* :
- A series of statistics measured empirically, of varying degress of seek distance,
from different offsets on disc

seektest2_huc/*:
- This is a testing program, written with HuC v3.21 and running on the CD System
Card, which was used for taking measurements
- It uses relatively large but sparse files to space out the CD image so that a
large seek can be performed on the media.  Both the fixed-size bulk data and the
final ISO file are stored in ZIP format in order to conserve space and bandwidth

seektime.c :
- A 'C' program to calculate effective seektime based on start and end sectors.
- Originally written for Linux



Use this information and software at your own risk.
