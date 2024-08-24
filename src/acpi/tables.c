/*
 * lux - a lightweight unix-like operating system
 * Omar Elghoul, 2024
 * 
 * Core Microkernel
 */

#include <stddef.h>
#include <string.h>
#include <kernel/boot.h>
#include <kernel/acpi.h>
#include <kernel/logger.h>

static int acpiVersion = 0;
static int tableCount = 0;
static ACPIRSDT *rsdt = NULL;
static ACPIXSDT *xsdt = NULL;

void acpiDumpHeader(ACPIStandardHeader *h) {
    KDEBUG("'%c%c%c%c' revision 0x%02X OEM ID '%c%c%c%c%c%c' at 0x%08X len %d\n",
        h->signature[0], h->signature[1], h->signature[2], h->signature[3],
        h->revision, h->oem[0], h->oem[1], h->oem[2], h->oem[3], h->oem[4], h->oem[5],
        (uint64_t)h, h->length);
}

int acpiInit(KernelBootInfo *k) {
    if(!k->acpiRSDP) {
        KWARN("system is not ACPI-compliant; power management and multiprocessing will not be available\n");
        return -1;
    }

    ACPIRSDP *rsdp = (ACPIRSDP *)k->acpiRSDP;
    KDEBUG("'RSD PTR ' revision 0x%02X OEM ID '%c%c%c%c%c%c' at 0x%08X\n", 
        rsdp->revision, rsdp->oem[0], rsdp->oem[1], rsdp->oem[2], rsdp->oem[3], rsdp->oem[4], rsdp->oem[5], k->acpiRSDP);

    if(rsdp->revision >= 2) {
        xsdt = (ACPIXSDT *)rsdp->xsdt;
        acpiVersion = 2;        // preliminary, we'll make sure of this from the FADT
    } else {
        rsdt = (ACPIRSDT *)(uintptr_t)rsdp->rsdt;
        acpiVersion = 1;        // same as above
    }

    // now dump the ACPI tables starting with the RSDT/XSDT
    if(xsdt) {
        acpiDumpHeader(&xsdt->header);
        tableCount = (xsdt->header.length - sizeof(ACPIStandardHeader)) / 8;
    } else {
        acpiDumpHeader(&rsdt->header);
        tableCount = (rsdt->header.length - sizeof(ACPIStandardHeader)) / 4;
    }

    ACPIStandardHeader *h;
    for(int i = 0; i < tableCount; i++) {
        if(xsdt) {
            h = (ACPIStandardHeader *)xsdt->tables[i];
        } else {
            h = (ACPIStandardHeader *)(uintptr_t)rsdt->tables[i];
        }

        if(!memcmp(h->signature, "FACP", 4)) {
            acpiVersion = h->revision;
        }

        acpiDumpHeader(h);
    }

    KDEBUG("total of %d tables directly listed in the %s\n", tableCount, xsdt ? "XSDT" : "RSDT");
    KDEBUG("system is compliant with ACPI revision %d\n", acpiVersion);
    return 0;
}

/* acpiFindTable(): finds an ACPI table by signature
 * params: sig - 4-character signature of the table
 * params: index - zero-based index of the table in cases of multiple tables
 * returns: pointer to the table header, NULL if not found
 */

void *acpiFindTable(const char *sig, int index) {
    int c = 0;
    ACPIStandardHeader *h;
    for(int i = 0; i < tableCount; i++) {
        if(xsdt) {
            h = (ACPIStandardHeader *)xsdt->tables[i];
        } else {
            h = (ACPIStandardHeader *)(uintptr_t)rsdt->tables[i];
        }

        if(!memcmp(h->signature, sig, 4)) {
            c++;
            if(c > index) return h;
        }
    }

    return NULL;
}
