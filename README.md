# idleinject - Userspace CPU Idle Injector
Used to effectively "pause" all usespace applications when the CPU is at a dangerous temperature extreme. This is written and intended for use on a38x CPUs which do not have proper thermal throttling in-kernel at this time. However, this tool can be used on nearly any CPU just the same.

This tool monitors the temperature of themal zone 0, and if it exceeds the set temperature, will pause userspace applications by injecting idle applications until the temperature is reduced.

## Build instructions
Install dependencies:

    apt-get update && apt-get install git build-essential meson -y

Download, build, and install on the target device:

    git clone https://github.com/embeddedTS/idleinject.git
    cd idleinjecct
    meson setup builddir
    cd builddir
    meson compile
    meson install
