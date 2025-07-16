#include "../../cpu/cpu.h"
#include "cpuid.h"
#include "string.h"
#include "dev/acpi/acpi.h"

int cpu_get_info(struct cpu* cpuinf)
{
    cpuinf->arch = CPU_ARCH_X86_64;
    cpuinf->byteorder = CPU_BYTEORDER_LE;
    cpuinf->sockets = 1; // пока что константа
    cpuinf->cpus = acpi_get_cpus_apic_count();

    memset(cpuinf->vendor_string, 0, CPU_VENDOR_STR_LEN);
    memset(cpuinf->model_string, 0, CPU_MODEL_STR_LEN);

    cpuid_get_vendor_string(cpuinf->vendor_string);
    cpuid_get_model_string(cpuinf->model_string);

    return 0;
}