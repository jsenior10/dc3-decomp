#include "Memory.h"
#include "os/System.h"
#include "utl/MemMgr.h"
#include "xdk/xapilibi/xbox.h"

namespace {
    int gPhysicalUsage;
    char *gPhysicalType = (char *)gNullStr;

    char *AllocType(unsigned long p1) {
        bool notPhys = (p1 & 0x80000000) == 1;
        uint type = p1 >> 0x10 & 0xff;

        switch (type) {
        case 0:
            if (notPhys) {
                if (gPhysicalType != gNullStr) {
                    return gPhysicalType;
                }

                return "XTL:D3D";
            }

            return "XTL(phys):D3D";
        case 1:
            if (notPhys) {
                return "XTL:D3DX";
            }
            return "XTL(phys):D3DX";
        case 2:
            if (notPhys) {
                return "XTL:XAUDIO";
            }
            return "XTL(phys):XAUDIO";
        case 3:
            if (notPhys) {
                return "XTL:XAPI";
            }
            return "XTL(phys):XAPI";
        case 4:
            if (notPhys) {
                return "XTL:XACT";
            }
            return "XTL(phys):XACT";
        case 5:
            if (notPhys) {
                return "XTL:XBOXKERNEL";
            }
            return "XTL(phys):XBOXKERNEL";
        case 6:
            if (notPhys) {
                return "XTL:XBDM";
            }
            return "XTL(phys):XBDM";
        case 7:
            if (notPhys) {
                return "XTL:XGRAPHICS";
            }
            return "XTL(phys):XGRAPHICS";
        case 8:
            if (notPhys) {
                return "XTL:XONLINE";
            }
            return "XTL(phys):XONLINE";
        case 9:
            if (notPhys) {
                return "XTL:XVOICE";
            }
            return "XTL(phys):XVOICE";
        case 10:
            if (notPhys) {
                return "XTL:XHV";
            }
            return "XTL(phys):XHV";
        case 0xb:
            if (notPhys) {
                return "XTL:USB";
            }
            return "XTL(phys):USB";
        case 0xc:
            if (notPhys) {
                return "XTL:XMV";
            }
            return "XTL(phys):XMV";
        case 0xd:
            if (notPhys) {
                return "XTL:SHADERCOMPILER";
            }
            return "XTL(phys):SHADERCOMPILER";
        case 0xe:
            if (notPhys) {
                return "XTL:XUI";
            }
            return "XTL(phys):XUI";
        case 0xf:
            if (notPhys) {
                return "XTL:XASYNC";
            }
            return "XTL(phys):XASYNC";
        case 0x10:
            if (notPhys) {
                return "XTL:XCAM";
            }
            return "XTL(phys):XCAM";
        case 0x11:
            if (notPhys) {
                return "XTL:XVIS";
            }
            return "XTL(phys):XVIS";
        case 0x12:
            if (notPhys) {
                return "XTL:XIME";
            }
            return "XTL(phys):XIME";
        case 0x13:
            if (notPhys) {
                return "XTL:XFILECACHE";
            }
            return "XTL(phys):XFILECACHE";
        case 0x14:
            if (notPhys) {
                return "XTL:XRN";
            }
            return "XTL(phys):XRN";
        case 0x15:
            if (notPhys) {
                return "XTL:XMCORE";
            }
            return "XTL(phys):XMCORE";
        case 0x16:
            if (notPhys) {
                return "XTL:XMASSIVE";
            }
            return "XTL(phys):XMASSIVE";
        case 0x17:
            if (notPhys) {
                return "XTL:XAUDIO2";
            }
            return "XTL(phys):XAUDIO2";
        case 0x18:
            if (notPhys) {
                return "XTL:XAVATAR";
            }
            return "XTL(phys):XAVATAR";
        case 0x19:
            if (notPhys) {
                return "XTL:XLSP";
            }
            return "XTL(phys):XLSP";
        case 0x1a:
            if (notPhys) {
                return "XTL:D3DAlloc";
            }
            return "XTL(phys):D3DAlloc";
        case 0x1b:
            if (notPhys) {
                return "XTL:NUISPEECH";
            }
            return "XTL(phys):NUISPEECH";
        case 0x1c:
            if (notPhys) {
                return "XTL:NuiApi";
            }
            return "XTL(phys):NuiApi";
        case 0x1d:
            if (notPhys) {
                return "XTL:NuiIdentity";
            }
            return "XTL(phys):NuiIdentity";

        case 0x3e:
            if (notPhys) {
                return "XTL:NuiApi_LargePageReadWrite";
            }
            return "XTL(phys):NuiApi_LargePageReadWrite";
        default:
            if (type < 0x80) {
                if (notPhys) {
                    return "XTL:Game";
                }
                return "XTL(phys):Game";
            }
            if (0xbf < type) {
                if (notPhys) {
                    return "XTL:Middleware";
                }
                return "XTL(phys):Middleware";
            }
            return "XTL:Unknown";
        }
    }

}

void PhysicalFree(void *address) {
    if (address != 0) {
        gPhysicalUsage -= XPhysicalSize(address);
    }

    XPhysicalFree(address);
}

void PhysicalFreeTracked(void *address, const char *p2, int p3, const char *p4) {
    if (address != 0) {
        gPhysicalUsage -= XPhysicalSize(address);
    }

    XPhysicalFree(address);
    MemTrackFree(address);
}

int PhysicalUsage() { return gPhysicalUsage; }
