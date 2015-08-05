
#ifdef NDEBUG

#define debug_printf(args) ((void)0)

#else // NDEBUG

#define debug_printf printf

#endif //NDEBUG

// Statically allocated address ranges

// 0xF000 - 0xF7FF (2048 bytes)
#define ADDR_ALIGNED_FSQRT_LUT		0xF000

// 0xFDFD - 0xFDFF (3 bytes)
#define ADDR_IRQ_TRAMPOLINE			0xFDFD
// 0xFE00 - 0xFF01 (257 bytes)
#define ADDR_IRQ_VECTOR_TABLE		0xFE00


