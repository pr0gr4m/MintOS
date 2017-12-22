# environment

* OS : Fedora 27 Workstation
* Kernel : 4.14.7-300.fc27.x86_64
* CC : gcc 7.2.1 (Red Hat 7.2.1-2)

# package

* binutils bison flex gcc libtool make patchutils libiconv libgmp-devel libmpfr-devel nasm glibc-devel

```
# sudo dnf install binutils bison flex gcc libtool make patchutils
# sudo dnf install glibc-devel glibc-static binutils-m32r-... gcc-m32r-...
# sudo dnf install glibc-devel(%{__isa_name}-32) glibc-static(%{__isa_name}-32)
# wget https://forensics.cert.org/cert-forensics-tools-release-27.rpm
# rpm -Uvh cert-forensics-tools-re...
# sudo dnf update
# sudo dnf install gmp-devel mpfr-devel
```

# emulator

* QEMU

```
# sudo dnf install qemu
```
