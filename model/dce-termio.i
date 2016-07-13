DCE(int, tcgetattr, (int fd, struct termios *termios_p));
DCE(int, tcsetattr, (int fd, int optional_actions, const struct termios *termios_p));