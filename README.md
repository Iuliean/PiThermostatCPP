# Raspberry Pi Web Thermostat

## Dependencies
- [CrowCpp](https://github.com/CrowCpp/Crow)
- [WiringPi](https://github.com/WiringPi/WiringPi)
- [Boost](https://www.boost.org/)
- [Json](https://github.com/nlohmann/json)
- [SHA256](https://github.com/stbrumme/hash-library)

# Building

You first need to install `boost` with `sudo apt install build-essential libboost-all-dev`
after that add `dtoverlay=i2c-sensor,bmp280` to `/boot/config.txt` (this enables the system module for the BMP280 sensor) then `make`

If you wish to make it more permanent use `sudo make install` this will move the `build` folder into `/opt/RPIThermostat` and it will also move `libwiringPi.so` and `RPIThermostat.service` to `/usr/lib` and `/etc/systemd/system/`. Then you can use `systemctl` to start/stop/enable/disable the service

# Build Folder Structure

```
    build/
        static/
            css, javascript, etc
        templates/
            .html pages

        config.json
        parameters.json
        site

```

# Configuration Files

There are two files: 
- `config.json`
- `parameters.json`

## Config.json

This file contains all the parameters required to start the app. Default:

```json
{
    "site":{
        "port": 18080,
        "password": "password",
        "cookieLifetime": 20,
        "cleanInterval": 3600
    },

    "controller":{
        "numOfReads":1,
        "readDelay":7000,
        "calibration":-2,
        "driverFile":"sys/bus/iio/devices/iio:device0/in_temp_input",
        "relayPin":3,
        "saveInterval": 600,
        "display":{
            "digits":[12,16,20,21],
            "A":26,
            "B":13,
            "C":9,
            "D":5,
            "E":6,
            "F":19,
            "G":10,
            "DP":11
        }
    }
   
}
```

### Tags

- `site` settings related to the site
    - `port` is the port on which the webserver runs
    - `password` password for site access (this gets hashed)
    - `cookieLifetime` this determines the lifetime of the authentification tokens both server and client side
    - `cleanInterval` the amount of time at which the program cleans expired cookies from memory
- `controller` settings related to the phyisical part of the thermostat
    - `numOfReads` determines how many temperature readings the program makes and averages them
    - `readDelay` the delay between temperature readings
    - `calibration` offsets the read temperatures by the specified value
    - `driverFile` path to BMP280 input file 
    - `saveInterval` the amount of time at which the program saves the internal `threshold` and `range` to disk
    - `display`
        + `digits` pins responsible for each digit segment in the display in order
        + `A,B,C,D,E,F,G,DP` pins responsible for each segment of the 7 segment display

## Parameters.json

This file is used to store the operating range of the thermostat.

```json
{
    "minTemp": 29,
    "maxTemp":0.3
}
```
This file is updated every 10 minutes by the program with the values from memory.

# Site Endpoints

The main endpoints of the site are:
    
- `/` login page (if a login cookie is already present it redirects to `/dashboard`)
- `/auth` recieves form `POST`requests from `/` with the hashed password and if the hash is correct it returns a cookie with a lifetime specified in the `config.json` file 
- `/dashboard` the main page where the operating parameters can be set and information about the current temperature and current state can be seen. Also if there is no authentification cookie it redirects to `/`

- `/getParams` if a authentification cookie is present it returns a json object containing the current temperature, state, threshold and range
- `/setParams` recieves `POST` requests with json objects that contain the new threshold and range and it sets those parameters to the recieved ones
- `/shutdown` issues shutdown command to the system `sudo system -P now`
- `/temperature` get temperature related data from the database. Example: `/temperature/get/2021-01-01` returns all temperature from the specified date untill the present day
- `/state` same as with the temperature endpoint

By default in CrowCpp every file in `build/static/` can be accessed through `/static/file`

# Site aspect

You can replace the scripts, html and css files with your own and make it look good 