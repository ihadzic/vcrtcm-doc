UDEV_RULES_PATH=/etc/udev/rules.d

all:
	@echo "Type make install to install the rules in $(UDEV_RULES_PATH)"

install:
	@echo "Installing udev rules to $(UDEV_RULES_PATH)..."
	sudo rm -f $(UDEV_RULES_PATH)/*pimmgr.rules
	sudo cp -v *.rules $(UDEV_RULES_PATH)

