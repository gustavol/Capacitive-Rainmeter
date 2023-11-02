# Capacitive-Rainmeter
Measure rain intensity in Homeassistant with a capacitive sensor 

The code uses mqtt to send data and also self register the sensor in Homeassistant. 

The sensor heater runs on 12V so that is the voltage you need to supply. Any ESP32 will really do but since you need to run this on 12V you need a DC-DC buck converter to power the ESP32 at 5V.  I instead chose my own RejsaCAN board since it has an on board DC-DC buck converter that can run the board on 4-24V. Pretty neat one board solution with it, I just added a few components to interface the ESP32-S3 with the rain sensor board. It felt a bit backwards to have an unused CAN bus on the board that I used, but its built in DC-DC buck converter made the choice easy since I had some extra of these boards lying around.

The code as it is now keeps the sensor at around 35 degrees Celsius. I don't know if that's the optimal temperature to for example keep snow melting and drying of rain not too fast and not too slow. I have a sensor installed and log it in Homeassistant and I might tweak the code as time goes by.

Why a capacitive sensor? I've used these common resistive sensors that have copper traces that the rain falls on. The problem is these boards tend to wear down after a while due to oxidization. There are tricks to minimize the problems, reducing the current and the time current is flowing, but in the end you still have metal and electrical current in a harsh environment. But a capacitive sensor is totally electrically isolated from the rain and dampness. No external parts of metal, removing any wear or tear in an outdoor environment.   


work in progress....


The sensor:
https://radiocontrolli.eu/Capacitive-Rain-Sensor-RC-SPC1K-p242943346

![sensor](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/523c88bf-ae12-401c-bdff-ebb91b518667)

![sensor back](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/43c9af0f-6f9f-44c0-93ee-2d97a060bb8d)

![pcb top](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/10883a02-48e6-4aa7-8237-53d67ebee8c7)

![pcb bottom](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/f8bb0217-bba8-4e76-8d42-76a1d8737b4e)

![arm closed](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/fd34311f-c391-4b5f-9df3-93f9d417c394)

![arm open](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/7e2d9be2-ed48-42fb-946f-4e2924a92be1)

![schema](https://github.com/MagnusThome/Capacitive-Rainmeter/assets/32169384/2cf65521-ba4e-4b0e-887e-84a0da4d858c)
