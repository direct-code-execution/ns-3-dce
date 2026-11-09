# gdb: "source utils/dce-fibers-gdb.py", then "dce-fibers [depth]" prints the
# stack of every DCE task (UcontextFiberManager), from the context it saved
# when it last switched away. The process is left with its registers changed:
# use it to inspect a stopped simulation, not to continue it.
import gdb

REG_RBP, REG_RSP, REG_RIP = 10, 15, 16
# struct UcontextFiber { uint8_t *stack; uint32_t stackSize; ucontext_t
# context; ... }: the context is at 16, its general registers 40 further.
GREGS = 16 + 40


class DceFibers(gdb.Command):
    def __init__(self):
        super().__init__("dce-fibers", gdb.COMMAND_STACK)

    def invoke(self, arg, from_tty):
        depth = int(arg) if arg else 12
        count = int(gdb.parse_and_eval("*(int *) &g_dceFiberCount"))
        fibers = gdb.parse_and_eval("(unsigned long *) &g_dceFibers")
        frame = gdb.selected_frame()
        saved = [frame.read_register(r) for r in ("rsp", "rip", "rbp")]
        for i in range(count):
            fiber = int(fibers[i])
            gregs = [int(gdb.parse_and_eval("*(unsigned long *) %d" % (fiber + GREGS + 8 * r)))
                     for r in range(23)]
            print("=== fiber %d (0x%x)" % (i, fiber))
            gdb.execute("set $rsp = %d" % int(gregs[REG_RSP]))
            gdb.execute("set $rbp = %d" % int(gregs[REG_RBP]))
            gdb.execute("set $pc = %d" % int(gregs[REG_RIP]))
            try:
                gdb.execute("bt %d" % depth)
            except gdb.error as e:
                print("  (%s)" % e)
        gdb.execute("set $rsp = %d" % int(saved[0]))
        gdb.execute("set $pc = %d" % int(saved[1]))
        gdb.execute("set $rbp = %d" % int(saved[2]))


DceFibers()
