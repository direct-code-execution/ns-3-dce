#include <stdint.h>
#include <sys/socket.h>

namespace ns3 {

class Cmsg
{
public:
  Cmsg (struct msghdr *msg);
  void Add (int level, int type, int len, const uint8_t *buffer);
  int GetNext (int *level, int *type, int *len, uint8_t **buffer);
  void Finish (void);
private:
  struct msghdr *m_msg;
  uint8_t *m_current;
  int m_len;
};

} // namespace ns3
