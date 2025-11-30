# PusheenGuardian
A LED alert for Wi-Fi attacks for the Pusheen Cat Lamp 2025

Created for the "Purrsheen" cat lamp build at Retia workshops, this release creates a flicker-flame yellow lamp that hides a packet filter looking for Wi-Fi attacks. 
The lamp will detect jamming attacks and respond by turning RED for deauthentication attacks, BLUE for dissisociation attacks, and PURPLE for a combination of both. 
While there may be false positives (legitimate uses do exist for both types of packets), a flood of either (or both) types of packets usually represents a Wi-Fi jamming attack.


Frequently, these attacks are used to target and disable IoT devices like cameras by police and criminals alike, but they are also a low-cost, easy nuscance attack.
You can flash the pre-compiled binary at nugget.dev. To change this code, download and open the source in Arduino IDE, add the Neopixel library, and compile for the Wemos D1/R2 Mini.

This code is based on the Deauth Detector, with modifications and upgrades to support neopixels and the current Purrsheen cat lamp build. 
