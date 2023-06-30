My first big project that started as a follow through with the Handmade hero series (https://guide.handmadehero.org/), then a game, and then just a lot of experimentation and stuff I wanted to try before moving on to a new one.

[WARNING] This project is an evolution of the way I code. You will find a lot of outdated comments (I don't read my comments that much, which discovered thanks to this), inconsistent and confusing naming (and how I went from ThisCase to this_case), declarations/definitions in weird places, and lots of unused code.

You will find a lot of code for things like:

· Png decoder and encoder (incomplete) (agu_png.c/h).

· Bmp decoder and decoder with only the lastest version (alpha supported).

· Ttf decoder (also incomplete, but works for some files like Arial) (purpose_ttf.c).

· A Math header with matrices and quaternions (purpose_math.h, and some struct definitionsin global_definitions.h).

· A bit of use of the Direct3D11 API (win32_d3d11.c) (the opengl and software are incomplete).

· Some stuff that Casey introduces like memory arenas.

· An immediate mode ui inspired by Ryan Fleury's articles (https://www.rfleury.com/p/ui-part-1-the-interaction-medium). in purpose_ui.h/h.

· "Random" number generator with noise (agu_random.h).

· The code I used by following Handmade Ray (also here https://guide.handmadehero.org/)

The list is incomplete, but I plan to complete as much as I can since someone might some stuff useful. I hope the file names are enough help in the meantime.
