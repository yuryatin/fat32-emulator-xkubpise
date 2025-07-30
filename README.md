# Simple FAT32 emulator _xkubpise_
This is a simple FAT32 emulator for the terminal, designed for quasi-POSIX-compliant environments (macOS, Linux, etc.). It supports short (8.3) filenames but does not allow internal spaces (e.g., `MY FILE.TXT` is not permitted). The emulator interacts with 20 MB FAT32 file-backed volumes with strict parameters:
- no mirroring is supported
- only the first FAT table can be active
- only one sector is allowed per data cluster
- long filenames are not supported
- the number of reserved sectors and FAT table sectors is fixed


<img src="https://github.com/user-attachments/assets/01c0e71d-3255-4eba-af1d-13743fa16b2a" alt="fat32 demo" width="618"/>

# What you can do in the emulator
- navigate folders with `cd` in two modes:
  - default mode: only absolute paths are accepted
  - extended mode: relative paths are also accepted, including `.` and `..`
- create new folders with `mkdir`
- create new empty files with `touch`
- list folder contents with `ls` or `dir`
- print the current path with `pwd` (although it's always visible in the command prompt)
- format a volume (for security reasons, the emulator does not initialize or format files whose size differs from exactly 20 MB)

# How to use
This terminal utility is launched from the command line and requires one of the following as the first argument:
- the name of an existing 20 MB file that is a FAT32 volume  
- the name of a 20 MB file the user wants to convert into a FAT32 volume  
- the name of a new file to be created as a FAT32 volume  
As a second argument (the order doesn't matter), you can pass `-p` to activate navigation mode with relative paths (including `.` and `..`).

# How does it treat input files
The FAT32 emulator _xkubpise_ attempts to determine whether it is working with a valid FAT32 volume and, if so, whether the candidate volume meets its stricter requirements (or, we might say, its limited capabilities).  
If the volume size deviates from exactly 20 MB, the emulator stops any interaction with the file (for data integrity reasons).  
If the file fails the conformity test but has the correct size, the user is cautiously advised to abstain from manipulating it. However, if the user chooses to proceed, the volume is initialized (which may result in some data loss), and the user can then explicitly format it using the `format` command.


Below is a screenshot showing the inconsistencies that the FAT32 emulator _xkubpise_ detects in a volume candidate.

<img width="730" height="371" alt="non_compliant_fat32" src="https://github.com/user-attachments/assets/0840c0a8-7f24-4761-83a8-8434943743a4" />
