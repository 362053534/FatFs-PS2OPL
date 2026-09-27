/* Host-only replacement for the PS2SDK debug macro. */
#define M_DEBUG(...) ((void)0)
typedef uint64_t u64;
#define DEBUG_U64_2XU32(value) ((void)(value))
