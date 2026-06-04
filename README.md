# MakingWaves_Project
"MakingWaves" is a wearable bracelet paired with a mobile app, designed to help international STEM students at UW turn everyday moments into meaningful friendships, starting with a simple wave of the hand.

# BOM
- OLED Screen
- MPU-6050
- XIAO Seeed Studio
- Coin Motor
- Button

# Algorithm Explanation 

It is a social wearable device in which an animal character on the bracelet’s OLED screen reacts when two people shake hands. The core idea is to transform the physical and social action of a handshake into a signal for digital connection.

The bracelet’s algorithm consists of three main parts. The first is the handshake detection algorithm. The MPU-6050 accelerator sensor reads the movement of the wrist approximately every 35 milliseconds. Instead of using the absolute sensor values, the algorithm calculates the difference, or delta, between the previous measurement and the current measurement. The reason for using the change in movement rather than the absolute value is that is allows the bracelet to detect handshake motions in  any direction, regardless of gravity direction or the angle at which the bracelet is worn. If this change exceeds the threshold value of 5000, it is recognized as a shake and the count increases. If shaking is detected five or more times with in 1.5 seconds, the movement is finally recognized as a handshake. This repeated pattern is required in order to distinguish an actual handshake from ordinary wrist movements.

The second part is the state machine. The bracelet is always in one specific state and has a total of ten states, including IDLE, CONNECTED, HAPPY, EATING, SINGING, HEART, and SLEEPING. When a handshake is detected or a button is pressed, the bracelet switches to corresponding state. Once the animation is completed, it returns to the IDLE state. If there is no input for 18 seconds, it automatically switches to the SLEEPING state. 

The third part is interface control. When the button is pressed once briefly, the interactions cycle through happy, eating, singing, shy, and playing animations. When the button is pressed twice quickly, the heart animation is triggered. When the button is pressed and held, the CONNECTED state is activated. During each state transition, the vibration motor operates according to a specific pattern, providing tactile feedback along with visual feedback. In the upper right corner of the screen, a real-time clock is also displayed by calculating the elapsed time using the millis() function.  
