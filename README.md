# lunaEsp32Template

An example app for https://github.com/romanchom/libLunaEsp32

The actual examples currently on different branches due to the complexities of making a multiple executable ESP project.

## version/pwm
The version on the 'version/pwm' is meant for a single RGBW light source. It supports MQTT, IR and Persistency modules. Due to fast boot times (a few 100s of milliseconds depending on optimization level and logging verbosity) it can be used as a drop-in replacement for switch controlled LED, while also providing wireless and infrared controll options. 

## version/2rgb+2w+atx
The version on the 'version/2rgb+2w+atx' is meant for a home-cinema like setup with addressable LEDs and PWM controlled white stripes behind the screen. It show how to wire the various modules and how to use an ATX power supply with a dummy load on 12V rail for stability. It is primatily meant for realtime control via the desktop app - https://github.com/wchomik/LunaClient, but also allows MQTT control for easy integration with home automation.
