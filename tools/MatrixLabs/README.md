# Alexa Demo for MATRIX Creator/Voice board.

Guide on how to setup the Alexa SDK SampleApp in a Raspberry Pi with a MATRIX Creator/Voice.

### Required Hardware

Before getting started, let's review what you'll need.
* Raspberry Pi model 3 B and B+ (Recommended) or Pi 2 Model B (Supported).
* MATRIX Voice or MATRIX Creator - Raspberry Pi does not have a built-in microphone, the MATRIX Voice / Creator have an 8 mic array - Buy MATRIX Voice / MATRIX Creator. 
* Micro-USB power adapter for Raspberry Pi.
* Micro SD Card (Minimum 8 GB) - An operating system is required to get started. You can download Raspbian Stretch and use the guides for Mac OS, Linux and Windows, on the Raspberry Pi website.
* External Speaker with 3.5 mm audio cable.
* A USB Keyboard & Mouse, and an external HDMI Monitor - we also recommend having a USB keyboard and mouse as well as an HDMI monitor handy. You can also use the Raspberry Pi remotely, see this guide from Google.
* Internet connection (Ethernet or WiFi)
(Optional) WiFi Wireless Adapter for Pi 2. Note: Pi 3 has built-in WiFi.

### Let's get started

Once you have the Raspberry Pi with the SD Card inserted, the MATRIX board connected to it, power up the Raspberry Pi and open a terminal either in the Raspbian Desktop or just by ssh into it ( if you are you are using the Raspbian Lite version, in this case, see guide on how to ssh [here](https://www.raspberrypi.org/documentation/remote-access/ssh/) ).

### 1. Installing MATRIX software

In order to allow the Alexa SDK to have access to MATRIX Voice microphones you need to first install:

```bash
# Add repo and key
curl https://apt.matrix.one/doc/apt-key.gpg | sudo apt-key add -
echo "deb https://apt.matrix.one/raspbian $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/matrixlabs.list

# Update packages and install
sudo apt-get update
sudo apt-get upgrade

# Reboot in case of Kernel Updates
sudo reboot

```

Connect after reboot and continue with:

```bash
# Update again is needed in case Kernel Updates
sudo apt-get update
# Installation MATRIX Pacakages
sudo apt install matrixio-creator-init
# Install kernel modules package
sudo apt install matrixio-kernel-modules

# Reboot
sudo reboot
```

Wait a bit and try to reconnect again.

### 2. Register a Product in Amazon Developer 

You'll need to register a device and create a security profile at  Amazon developer website. If you have already registered a product and even created Security Profile we still recommend you create a new one for this demo. Click [here](https://github.com/alexa/avs-device-sdk/wiki/Create-Security-Profile) for step-by-step instructions.

***
**IMPORTANT**: As you will be told in the steps, you need to save the **Client ID** and  **Product ID**, in the case of the **Client ID** you specifically need the one generated in *Platform Information > Other devices and platforms* tab.

### 3. Installation

Download the install script. We recommend running these commands from the home directory (`~/` or `/home/pi/`); however, you can run the script anywhere.

```bash
wget https://raw.githubusercontent.com/matrix-io/avs-device-sdk/master/tools/MatrixLabs/setup.sh
wget https://raw.githubusercontent.com/matrix-io/avs-device-sdk/master/tools/MatrixLabs/config.txt
```

Open the `config.txt` file in an editor and use the `Client ID`, `Product ID` from the registration steps to fill the file `config.txt`. Check [here](https://www.raspberrypi.org/magpi/edit-text/) If you need help editing the file.

Run the setup script with your configuration as an argument:
```
bash ./setup.sh ./config.txt
```
This installation can take some time, so go find something useful to do while waiting :)

### 4. Run Alexa!

Let's run the Sample App

```bash
bash ./startsample.sh
```

The first time the app will display a message like this:

```bash
######################################################
#       > > > > > NOT YET AUTHORIZED < < < < <       #
######################################################

############################################################################################
#     To authorize, browse to: 'https://amazon.com/us/code' and enter the code: {XXXX}     #
############################################################################################
```

So go to a browser (could be on another device, PC, tablet, phone) and go to https://amazon.com/us/code and used the provided code XXXX inside the curly brackets. Then select "Allow" and wait for it to get the token (it may take as long as 30 seconds). This authentication is just required this first time running the app.

After authentication is done you should see this message:

```bash
########################################
#       Alexa is currently idle!       #
########################################
```

Now you can talk to Alexa!