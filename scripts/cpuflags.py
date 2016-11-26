import errno
import platform
import subprocess

MACOS=(platform.system()=='Darwin')

class CPUFlags:
    def __init__(self):
        self.flags = set()

        try:
            self.flags = self.__parse_cpuinfo()
        except IOError as e:
            if e.errno == errno.ENOENT:
                return

            raise


    def __contains__(self, name):
        return name in self.flags


    def __parse_cpuinfo(self):

        def get_flags():
            if MACOS:
                return subprocess.Popen("sysctl -n machdep.cpu ".split(),  stdout=subprocess.PIPE).communicate()[0].decode("utf-8").lower().split()
            else :
                with open('/proc/cpuinfo', 'r') as f:
                    for line in f:
                        if line.startswith('flags'):
                            return line.split()[2:]
        return set(get_flags())


def main():

    import sys

    flags = CPUFlags()

    if len(sys.argv) == 2:
        if sys.argv[1] in flags:
            print ("present")


if __name__ == '__main__':
    main()
