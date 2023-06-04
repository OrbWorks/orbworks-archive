# A Brief History

OrbWorks was a side project of Jeremy Dewey and Kevin Cao, active between 1997 and 2007.

After re-targeting the Z80 [Small-C](https://en.wikipedia.org/wiki/Small-C) compiler to generate binaries for the TI-85 calculator, Jeremy was inspired to write a C-like compiler from scratch for the PalmPilot. At the time, Jeremy and Kevin were attending the same college and met while interning at a small local software company.

Jeremy released the first beta build of PocketC for the PalmPilot at the end of 1997, with version 1.0 arriving in April 1998. PocketC was fairly simple but strained the resources on early Palm OS devices. The PalmPilot Professional was the target device, but optimizations were done to make it work as well as possible on the PalmPilot with less memory.

In 1999, Kevin joined the project to create the Windows CE versions of PocketC. He released versions for Palm-sized PC, Pocket PC, and Handheld PC devices.

As PocketC matured, various features and tools were added.

- BuildPRC was provided to registered users. This was a Windows app that would build a Palm OS PRC (executable file) with a user-selected icon that simply launched the PocketC runtime and directed it to execute a specific pre-compiled applet. This tools was deprecated when PocketC Desktop Edition was released.
- Native add-in support was added. Originally this was used to add support for grayscale graphics, but eventually lead to the creation of the popular libraries, most notably [PToolboxLib](https://palmdb.net/search/ptoolboxlib) by Joseph H. Stadolnik III.
- PocketC Desktop Edition was created to allow development on Windows. This tool also allowed the creation of standalone apps, creating a Palm OS PRC which embedded the app, the PocketC runtime, as well as the native add-ins used by the app.

PocketC was a great tool for hobby programming and building small apps. However, the language was pretty limited, and there was a hole in the market for an easy-to-use Palm OS app designer. Jeremy and Kevin worked together to build OrbForms Designer. Jeremy wrote the compiler and virtual machine, and Kevin wrote most of the UI builder and development environment. OrbForms was first released in 2002.

OrbForms used OrbC as its language, which was derived from PocketC but added stronger typing and support for structs. As OrbForms development continued, the language advanced with the addition of objects and interfaces.

Feature development of APIs for PocketC and OrbForms continued in parallel. For example, the Network native add-ins were implemented at the same with nearly identical code.

The OrbC language was a huge advancement over PocketC, and Jeremy wanted to bring the compiler back to the device like the original PocketC. The Palm OS devices at this time had much more memory and speed, enabling them to run the much more complex compiler. PocketC Architect was the name chosen for this project, and compatibility with existing PocketC code was a primary goal. This was achieved by adding the following:

- Variant support to the OrbC compiler and VM. With this, the type of a given value in memory could be determined at runtime (like all PocketC values) instead of just at compile time.
- Support for PocketC native add-ins to be used from the OrbC VM
- Ability to run a `main()`-based app instead of just an event-based app
- API wrappers the reimplemented PocketC APIs based on OrbC APIs

As PocketC Architect was nearing completion, the Palm OS market was in steep decline. The project moved forward anyway because it was a passion project and there was sufficient demand from the existing PocketC userbase.