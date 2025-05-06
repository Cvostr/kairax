#include "kairax/types.h"

uint64_t arch_get_msi_address(uint64_t *data, size_t vector, uint32_t processor, uint8_t edgetrigger, uint8_t deassert) {
	vector += 32;
	*data = (vector & 0xFF) | (edgetrigger == 1 ? 0 : (1 << 15)) | (deassert == 1 ? 0 : (1 << 14));
	return (0xFEE00000 | (processor << 12));
}

uint64_t arch_get_msix_address(uint64_t *data, size_t vector, uint32_t processor, uint8_t edgetrigger, uint8_t deassert)
{
	vector += 32;
	*data = (vector & 0xFF) | (edgetrigger == 1 ? 0 : (1 << 15)) | (deassert == 1 ? 0 : (1 << 14));
	return (0xFEE00000 | (processor << 12));
}