# OrbWorks Archive

This archive contains most of the source code from [OrbWorks](https://orbworks.com) products for Palm OS. This includes:

- PocketC for Palm OS
- PocketC Desktop Edition for Palm OS
- PocketC Architect (both desktop and device)
- OrbForms Designer compiler and tools

Notably missing from this archive are:

- PocketC for Windows CE. The source code for this platform has been lost.
- OrbForms Designer IDE. Since this builds so heavily on 3rd party commercial frameworks, its open source value is very limited.

## Documentation

In addition to the code, this archive includes some helpful documentation for understanding the projects:

- [A Brief History](History.md) of OrbWorks and the projects
- An overview of the [Compilers](Compilers.md)
- Detailed information about the [PocketC VM](PocketC-VM.md) and [OrbC VM](OrbC-VM.md)
- A [Readme](Tools/Readme.md) in Tools covering usage and examples of the command-line tools
- Released documentation for each product can be found in the archive

## Build

- The code in this archive has not been built since it was last released in 2007
- Windows code was built with Visual Studio 2005
- Palm OS code was built with [Metrowerks Codewarrior](https://palmdb.net/app/codewarrior)
- As noted in the Directory Info below, 3rd party commercial libraries were used for the Windows IDEs. The 3rd party code is not in the archive, so the Windows IDEs cannot be built as is.

## Directory Info

The layout of the archive is described below.

### Architect

The [PocketC Architect](https://orbworks.com/architect/index.html) source code. PocketC Architect takes the OrbC compiler from OrbForms, makes it compatible with legacy PocketC source code, and allows it to be built directly on a Palm device. Because of this heritage, much of the code for PocketC Architect is actually built from the OrbForms directory.

| Directory    | Description                                                  |
| ------------ | ------------------------------------------------------------ |
| Desktop      | Windows IDE. The IDE is built on BCGPro controls, which are not open source. As a result, this code cannot be built without additional files. |
| Device       | Support files needed on the Palm device. This is just MathLib. |
| PalmCompiler | Palm-based compiler app built from the included CodeWarrior project. This folder contains the app user interface as well as some replacements for STL types. The sources for the compiler itself are pulled from OrbForms\compiler. |
| PDoc         | Tool to convert text files to and from PDOC format. Published here: [OrbPDOC](https://orbworks.com/other/orbpdoc.html) |
| PocketC      | Legacy PocketC compatibility headers which implement the PocketC APIs using OrbC APIs. |
| Samples      | Sample code shipped with the compiler.                       |
| Setup        | Build scripts and other bits that end up in the release.     |
| UtilCode     | Utility code for end-users. The build script merges this with OrbForms\UtilCode. |

### OrbForms

The [OrbForms Designer](https://orbworks.com/orbforms/index.html) (and core PocketC Architect) source code. The source code for the IDE is heavily built on 3rd party libraries, such as BCGPro. Because of this, the source of the IDE itself is not included in this archive.

| Directory     | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| add-in source | Implementation of the native libraries shipped with the compiler. Each folder contains a CodeWarrior project for a library. |
| add-ins       | Built versions of the native libraries along with their OrbC library files. |
| bc            | Bytecode printer. This internal tool prints the metadata and bytecode from a .prc or .vm file. This tool was invaluable for finding and fixing code generation bugs. See the [Readme](Tools/Readme.md) in Tools. |
| compiler      | OrbC compiler. This is embedded in PCA.prc for the Palm compiler and produces compiler.dll for Windows. This folder also contains PalmDB, a utility class for manipulating Palm databases. This class is used by many of the projects in this archive. The compiler also exposes a QuickParse method used by the IDE to generate code completions. |
| deprc         | Internal tool for splitting resources from a PRC file. This is mostly a command-line wrapper over a subset of the PalmDB class. |
| docs          | Source and scripts for both PocketC Architect and OrbForms docs. These rely on Microsoft's HtmlHelp SDK, MSXML, as well as the internal DocTool |
| doctool       | Internal tool for processing/generating docs.                |
| emuctrl       | Emulator control library to allow OrbForms and PocketC Architect to install and launch apps. IIRC, the source code was inspired by a Perl implementation. |
| oc            | Command-line wrapper around the compiler DLL. The DEBUG build has additional features for testing QuickParse as well as generating skeleton documentation for APIs. See the [Readme](Tools/Readme.md) in Tools. |
| OrbFormsRT    | OrbC runtime containing the VM and all built-in APIs. This can be built as Release, Debug, and FAT. The FAT version is deconstructed and included in the compiler for generating standalone apps. |
| OrbLaunch     | Stub app for launching OrbFormsRT. This is deconstructed and included in the compiler for generating non-standalone apps. |
| OrbPDB        | Tool for converting between CSV files and OrbC/PocketC-style PDBs. Published here: [OrbPDB](https://orbworks.com/other/orbpdb.html) |
| samples       | Samples shipped with the compiler.                           |
| setup         | Build scripts and other bits that end up in the release.     |
| tests         | A bunch of source files used for testing the compiler and runtime. Some are written as BVTs (build verification tests). Others are simple code that reproduced a bug. |
| utilcode      | Utility code for end-users shipped with the compiler.        |
| vm            | A Windows-based VM for testing. This contains only enough built-in APIs to validate the compiler. See the [Readme](Tools/Readme.md) in Tools. |

### PocketC

The [PocketC for Palm OS](https://orbworks.com/pcpalm/index.html) source code.

| Directory    | Description                                                  |
| ------------ | ------------------------------------------------------------ |
| bytecode     | Bytecode printer. This internal tool prints the metadata and bytecode from a .pdb file. This tool was invaluable for finding and fixing code generation bugs. The PocketC version of this tool has fewer features than the OrbC version. See the [Readme](Tools/Readme.md) in Tools. |
| DatGen       | Tool to generate the funcs.dat file used by PDE for displaying API help. |
| DistSrc      | Contains all the static files used to make PocketC distributions. |
| memo         | A tool for managing memos. It can enter text into a memo by simulating keystrokes into the Palm OS Emulator, or it can manipulate a MemoDB file. |
| pc2html      | A tool for generating a syntax-highlighted HTML file from PocketC source code. |
| pde          | PocketC Desktop Edition Windows app. The app is built on BCGPro controls, which are not open source. As a result, this code cannot be built without additional files. |
| PocketC-Palm | PocketC compiler and runtime for Palm OS.                    |
| setup        | Files needed for the PDE installer.                          |
| tests        | A bunch of source files used for testing the compiler and runtime. Some are written as BVTs (build verification tests). Others are simple code that reproduced a bug. |
| .            | Build scripts                                                |

## Release

The Release directory contains the latest binary releases of the products.

| File                         | Description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| BuildPRC.zip                 | Early tool for generating a PRC stub for a PocketC applet. This tool was created before PocketC Desktop Edition and later deprecated. |
| OrbForms.zip                 | OrbForms Designer v4.1.3. To register, use email `github-orbforms-user@orbworks.com` and registration code `611c5c7a21a279447aca72e408941b44`. |
| OrbPDOC.zip                  | OrbPDOC v1.1                                                 |
| PDE.zip                      | PocketC Desktop Edition v7.1.4                               |
| PocketC.zip                  | PocketC v7.1.4. Use the PowerShell script `Generate-PocketCCode.ps1 <username>` to generate a registration code. |
| PocketC_rt.zip               | PocketC runtime v7.1.4                                       |
| PocketCArchitectDestktop.zip | PocketC Architect (desktop) v4.1.3. To register, use email `github-architect-user@orbworks.com` and registration code `30ad739847d707dc364850861d73857e`. |
| PocketCArchitectDevice.zip   | PocketC Architect (device) v4.1.3. Use the PowerShell script `Generate-PocketCArchitectCode.ps1 <username>` to generate a registration code. |
| PocketCScan.zip              | PocketC Desktop Edition v7.1.4 for scanners. This includes the PocketC compiler and runtime with support for Symbol scanners. This product was only made available to select partners. |

## Notes

Random notes

- The OrbFormsRT still contains commented-out code for dynamic form generation. Unfortunately, Palm OS had a bug in the ROM that would sometimes cause memory corruption when these APIs were used, so the OrbForms compiler had to be rewritten to generate static UI resources.
- A lot of effort went into making the apps compatible with Palm OS 6 (Cobalt), which never ended up shipping.
- There are scattered references to SrcEd in the code. PocketC Desktop Edition was also built as a general purpose text editor for internal use.
