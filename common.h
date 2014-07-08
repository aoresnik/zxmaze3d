
#ifdef NDEBUG

#define debug_printf(args) ((void)0)

#else // NDEBUG

#define debug_printf printf

#endif //NDEBUG
