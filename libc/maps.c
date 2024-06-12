#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

int main() {
	int fd = open("/proc/self/maps", O_RDONLY);
	char* heap = sbrk(0);
	int count = 0;
	char buf[10] = {0}; 
	int num = 1, hyphen = 0, new_line = 0, perms = 0;;
	while (1) {
		char out;
		int ret = read(fd, &out, 1);
		if (ret < 0) {
			write(fd, "Read failure\n", 13);
			return 1;
		}
		else if (!ret) {
			break;
		}
		else {
			if (((num && isdigit(out)) || (perms && isalpha(out))) && !new_line) {
				buf[count] = out;
				count++;
			}
			else if (out == '-' && !perms && !new_line) 
				hyphen = 1;
			else if (isspace(out) && !new_line) {
				if (hyphen) {
					hyphen = 0;
					perms = 1;
				}
				else {
					perms = 0;
					new_line = 1;
				}
			}
			else if (out == '\n') {
				new_line = 0;
			}
		}
	}
	while (1) {}

}
