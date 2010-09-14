#ifndef TRAMPOLINE_MANAGER_H
#define TRAMPOLINE_MANAGER_H

namespace ns3 {

class TrampolineManagerImpl;

class TrampolineManager
{
 public:
  static TrampolineManager *Instance (void);
  bool Insert (unsigned long from, unsigned long to);
 private:
  TrampolineManager (TrampolineManagerImpl *impl);
  ~TrampolineManager ();
  TrampolineManagerImpl *m_impl;
};

} // namespace ns3

#endif /* TRAMPOLINE_MANAGER_H */
