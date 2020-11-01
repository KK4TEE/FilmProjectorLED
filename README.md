# Film_Projector_LED
![Film Projector](doc/img/readme_banner.jpg?raw=true "Picture of the running projector with protective cover removed")

Upgrading a 16mm Projector with a Teensy controlled LED with my friend Vangelis Lympouridis. He wanted to give his projector a few improvements, including replacing the traditional incandescent light bulb with an LED and using a micro-controller to replace the mechanical shutter. Little did we know... flashing a high power LED so rapidly is non-trivial.

More information can be found in the [YouTube video](https://www.youtube.com/watch?v=0nMBs8RFyTc) going over the project.

## Project Overview
 - Hardware: Vangelis Lympouridis, PhD
 - Firmware: Seth Persigehl, Factorio Player

 - Goal: Replace the mechanical shutter system with an electronic equivilent (using an LED). Timings, duty cycle, and number of pulses are adjustable.
 - Serial output provides on-going status and timing information
 - The hardware wheel has 12 cogs
 - DC60S3 Solid state relay, switching 24VDC
 - Relay was [programed](https://github.com/KK4TEE/FilmProjectorLED/blob/47bee6803c6bd2781ce234f0b7c16d7a37f5f261/FilmProjectorLED.ino#L51) to pulse the LED in a `{0,1,0,0,1,0,0,0,0,0}` pattern to provide crisp visuals
 
 The Teensy constantly monitors the speed of the driving motor to adjust the LED pulses to stay in sync with where the mechanical shutter would have been based on the 12 cog wheel. An optical sensor is placed at the top center position of the cog. See the [main project file](FilmProjectorLED.ino) for details.
 ```
 void loop()
	{
	  DetectFrameStartAndEnd();
	  NewWaveDetectedInstantResponse();
	  RecordFrameTimings();
	  CheckAndRunTimedEvents();
	  CommunicateWithUser();
	}
```

## Known issues
 - After around 50 days of uptime, there will (probably) be a stutter in the LED due to a rollover in `actionsTimingsArray`
