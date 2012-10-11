all:
	echo "<make user> install libraries and drivers in user mode"
	echo "<make kernel> installs libraries and drivers in kernel mode"

user: devif_user usbif_user pwcmech_user pwc_user

kernel: devif_kernel usbif_kernel pwcmech_kernel pwc_kernel

devif_user:
	cd devif/Build
	cmake ../
	make

usbif_user:
	cp usbif/Makefile.user usbif/Makefile
	make -C usbif

pwcmech_user:
	cp drivers/pwc/pwcmech/Makefile.user drivers/pwc/pwcmech/Makefile
	make -C drivers/pwc/pwcmech

pwc_user:
	make -C drivers/pwc/user/pwc

devif_kernel:
	cp devif/Makefile.kernel devif/Makefile
	make -C devif

usbif_kernel:
	cp usbif/Makefile.kernel usbif/Makefile
	make -C usbif

pwcmech_kernel:
	cp drivers/pwc/pwcmech/Makefile.kernel drivers/pwc/pwcmech/Makefile
	make -C drivers/pwc/pwcmech

pwc_kernel:
	make -C drivers/pwc/kernel/pwc-3.1.0

