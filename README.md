# Raspberry Pi Web Thermostat

## Dependencies
- [CrowCpp](https://github.com/CrowCpp/Crow)
- [WiringPi](https://github.com/WiringPi/WiringPi)
- [Boost](https://www.boost.org/)
- [Json](https://github.com/nlohmann/json)
- [SHA256](https://github.com/stbrumme/hash-library)

# Building

A simple ` make ` should do the trick. The output it's in `build/`

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
        "cookieLifetime": 20
    },

    "controller":{
        "tempReads":1,
        "readDelay":0,
        "tempPin":22,
        "relayPin":3,
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
- `controller` settings related to the phyisical part of the thermostat
    - `tempReads` determines how many temperature readings the program makes and averages them
    - `readDelay` the delay between temperature readings
    - `tempPin` pin on which the temperature sensor is installed on
    - `realyPin` pin on which the relay is installed on
    - `display`
        + `digits` pins responsible for each digit segment in the display in order
        + `A,B,C,D,E,F,G,DP` pins responsible for each segment of the 7 segment display

## Parameters.json

This file is used to store the operating range of the thermostat.

```json
{
    "threshold": 29,
    "range":0.3
}
```

The above settings mean that the thermostat will turn on if the temperature is below 28.7 and will turn off the temperature is above 29.3 .

This file is updated every 10 minutes by the program with the values from memory.

# Site Endpoints

The main endpoints of the site are:
    
- `/` login page (if a login cookie is already present it redirects to `/dashboard`)
- `/auth` recieves form `POST`requests from `/` with the hashed password and if the hash is correct it returns a cookie with a lifetime specified in the `config.json` file 
- `/dashboard` the main page where the operating parameters can be set and information about the current temperature and current state can be seen. Also if there is no authentification cookie it redirects to `/`

- `/getParams` if a authentification cookie is present it returns a json object containing the current temperature, state, threshold and range
- `/setParams` recieves `POST` requests with json objects that contain the new threshold and range and it sets those parameters to the recieved ones

By default in CrowCpp every file in `build/static/` can be accessed through `/static/file`