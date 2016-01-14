#include <stdint.h>
#include <sys/socket.h>
#include <iostream>

namespace ns3 {

/**
 * \brief Structure used for storage of ancillary data object information (Control message header wrapper)
 *
 * These messages are not a part of the socket payload and may include the interface
 * the packet was received on, various rarely used header fields,
 * an extended error description, a set of file descriptors or UNIX credentials.
 * Ancillary data is a sequence of struct cmsghdr structures with appended data.
 */
class Cmsg
{
public:
  Cmsg (struct msghdr *msg);

  /**
   * Generate the cmsghdr accordingly
   *
   * \return True if there is enough space to add ancillary data
   */
  bool Add (int level, int type, int len, const uint8_t *buffer);

  /**
   * Get next control message
   *
   * \return <0 in case of failure, 0 if successful
   */
  int GetNext (int *level, int *type, int *len, uint8_t **buffer);

  /**
   * Updates parameters of m_msg
   */
  void Finish (void);


  void Print (std::ostream &os) const;

private:
  struct msghdr *m_msg; /**< describes messages sent by `sendmsg' and received by `recvmsg' */
  uint8_t *m_current;   /**< points at where to add anciliary data */
  int m_len;            /**< Length as defined in constructor NOTE: socket.h has a warning about this datatype */
};

std::ostream& operator<< (std::ostream& os, const Cmsg & address);

} // namespace ns3
