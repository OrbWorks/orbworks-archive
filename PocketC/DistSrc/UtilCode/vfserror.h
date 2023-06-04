/$ vfserror.h
// vfserror(int err) - returns a string representation of the error
// code returned by any of the VFS functions

vfserror(int err) {
    string res = "Unknown VFS error";
    switch (err) {
        case 0x2a08: res = "vfsErrFileNotFound"; break;
        case 0x2a05: res = "vfsErrFilePermissionDenied"; break;
        case 0x2907: res = "vfsErrCardReadOnly"; break;
        case 0x2a02: res = "vfsErrFileGeneric"; break;
        case 0x2a04: res = "vfsErrFileStillOpen"; break;
        case 0x2a06: res = "vfsErrFileAlreadyExists"; break;
        case 0x2a07: res = "vfsErrFileEOF"; break;
        case 0x2a0d: res = "vfsErrDirNotEmpty"; break;
        case 0x2a0e: res = "vfsErrBadName"; break;
        case 0x2a0f: res = "vfsErrVolumeFull"; break;
        case 0x2a11: res = "vfsErrNotADirectory"; break;
        case 0x2a12: res = "vfsErrIsADirectory"; break;
        case 0x2a13: res = "vfsErrDirectoryNotFound"; break;
        case 0x2a14: res = "vfsErrNameShortened"; break;
    }
    return res;
}

// open modes
#define vfsModeRead 2
#define vfsModeWrite 5
#define vfsModeReadWrite 7
#define vfsModeCreate 8
#define vfsModeTruncate 16

// attributes
#define vfsAttrReadOnly 1
#define vfsAttrHidden 2
#define vfsAttrSystem 4
#define vfsAttrVolumeLabel 8
#define vfsAttrDirectory 16
#define vfsAttrArchive 32
#define vfsAttrLink 64

// dates
#define vfsDateCreated 1
#define vfsDateModified 2
#define vfsDateAccessed 3
