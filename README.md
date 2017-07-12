# About This Project
This Project is ESP32 porting version of AWS-IoT "shadow_console_echo" example.
Some example folder and files are added to search network discovery and other applications.
It does not include ESP-IDF but only having example project. So you need to setup ESP-IDF indivisually.
Have fun ^__^

# Amazon Web Services IoT Thing Shadow Example

This is an adaptation of the [AWS IoT C SDK](https://github.com/aws/aws-iot-device-sdk-embedded-C) "shadow_console_echo" example for ESP-IDF.

# Provisioning/Configuration

See the README.md in the parent directory for information about configuring the AWS IoT examples.

After following those steps, there is one additional step for this exmaple:

## Set Thing Name

For this example, you will need to set a Thing Name under `make menuconfig` -> `Example Configuration` -> `AWS IoT Thing Name`.

The Thing Name should match a Thing that you created while following the Getting Started guide (to check the Things you have registered, go t othe AWS IoT console web interface, click Registry and then click Things).


# Troubleshooting

If you're having problems with the AWS IoT connection itself, check the Troubleshooting section of the README in the parent directory.

* If your Thing is connecting and appears to be successfully updating, but you don't see any updates in the AWS IoT console, then check that the Thing Name in the Example Configuration under menuconfig matches exactly the thing name in AWS IoT console (including case).
