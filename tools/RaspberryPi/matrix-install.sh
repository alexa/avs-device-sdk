#!/bin/bash

echo "################################################################################"
echo "################################################################################"
echo ""
echo "MATRIX Labs software installation"
echo ""
echo "################################################################################"
echo "################################################################################"

# Add repo and key
curl https://apt.matrix.one/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://apt.matrix.one/raspbian $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/matrixlabs.list
# Update packages and install
sudo apt-get update
sudo apt-get upgrade
# Installation MATRIX Pacakages
sudo apt install matrixio-creator-init
# Installation Kernel Packages
sudo apt-get -y install raspberrypi-kernel-headers raspberrypi-kernel git 

# Reboot
echo "Rebooting required. Starting in 5sec."
sleep 5
echo "Connect again after 30 sec."
sudo reboot
