#ifndef DCE_EXECUTABLE_H
#define DCE_EXECUTABLE_H

#include <stdint.h>
#include <string>
#include <vector>

namespace ns3 {

// Programs that DCE loads from an executable (a PIE) rather than from a
// shared object, such as a prebuilt web browser.

/**
 * \param handle the program, loaded
 * \returns the main() of an executable which does not export it, from its
 *          entry point: _start hands it to __libc_start_main in %rdi.
 */
void *DceExecutableMain (void *handle);

/**
 * \param filename the program
 * \param image set to the initial value of the program's thread local
 *        variables, if it is an executable: its code addresses them at fixed
 *        offsets below the thread pointer (local-exec), image.size () bytes
 *        below it to start with.
 * \returns true if the program is an executable with thread local variables.
 */
bool DceExecutableTlsImage (std::string filename, std::vector<uint8_t> &image);

/**
 * Give the current task's thread the initial value of its program's thread
 * local variables. Needs a TaskManager with PerFiberTls and the room for them
 * that DCE_RESERVE_EXECUTABLE_TLS reserves in the simulator's executable.
 */
void DceExecutableTlsInit (const std::vector<uint8_t> &image);

/**
 * \param handle the program, loaded
 * \param image the image DceExecutableTlsImage read from the file, set to
 *        the image the dynamic loader relocated (pointers in thread local
 *        variables' initial values).
 *
 * The current task's thread got the image read from the file before the
 * program's constructors ran: its relocated words are fixed too, unless the
 * constructors changed them.
 */
void DceExecutableTlsRelocate (void *handle, std::vector<uint8_t> &image);

struct ProgramAllocator;
/**
 * \param handle the program, loaded
 * \returns its own malloc(), free(), calloc() and realloc() (and
 *          posix_memalign() and malloc_usable_size() if it has them), if it
 *          defines them itself, else 0.
 */
ProgramAllocator *DceExecutableAllocator (void *handle);

} // namespace ns3

#endif /* DCE_EXECUTABLE_H */
