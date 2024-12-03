# DroneCommand
DroneCommand is an interactive control station for managing unmanned aerial vehicles (UAVs), featuring real-time map visualization, flight control, battery monitoring, no-fly zone management, and collision detection for seamless and safe drone operations.

## Features and Components
1. LED Indicators
Purpose: Display the activity status of two UAVs.
Behavior:
Dark color: UAV is inactive.
Bright color: UAV is active.

2. Topographic Map
Display: A textured representation of Mount Majevica and surrounding municipalities.
Design: Rendered with a greenish glass overlay for a technological aesthetic.

#### Start screen
![Start screen](https://github.com/user-attachments/assets/511b618c-7bf8-4b9e-a79d-aa400029b190)

4. No-Fly Zone
Location: Positioned over the municipality of Lopare.
Appearance: Semi-transparent circular overlay.
Interactivity:
Move:
Drag with the left mouse button to reposition the zone.
Release the button to set the new position. UAVs encountered during dragging are destroyed.

#### Moved no fly zone
![Moved no fly zone](https://github.com/user-attachments/assets/8cf19066-d257-48d4-aaba-694b9a0f47ec)

Resize:
Hold the right mouse button to increase the size of the zone.
Release the button to finalize the size. UAVs encountered during resizing are destroyed.

#### Resizing no fly zone
![Resizing no flz zone](https://github.com/user-attachments/assets/ecd0389b-b2f7-4096-a934-0e4f9e61a76c)

Reset: Press the R key to reset the position and size of the zone.

6. Battery Levels
Display: Two progress bars show the battery percentage of each UAV.
Details:
The percentage value is displayed in the center of each bar.
XY coordinates of each UAV are displayed below their respective bars.
Behavior:
Batteries deplete gradually while UAVs are active.
UAVs with empty batteries are destroyed and deactivated.


UAV Control Station Interface README
Overview
This document provides a detailed explanation of the functionalities and elements of the UAV (Unmanned Aerial Vehicle) Control Station interface. The interface includes various interactive components to monitor, control, and simulate the behavior of UAVs.

Features and Components
1. LED Indicators
Purpose: Display the activity status of two UAVs.
Behavior:
Dark color: UAV is inactive.
Bright color: UAV is active.
2. Topographic Map
Display: A textured representation of Mount Majevica and surrounding municipalities.
Design: Rendered with a greenish glass overlay for a technological aesthetic.
3. No-Fly Zone
Location: Positioned over the municipality of Lopare.
Appearance: Semi-transparent circular overlay.
Interactivity:
Move:
Drag with the left mouse button to reposition the zone.
Release the button to set the new position. UAVs encountered during dragging are destroyed.
Resize:
Hold the right mouse button to increase the size of the zone.
Release the button to finalize the size. UAVs encountered during resizing are destroyed.
Reset: Press the R key to reset the position and size of the zone.
4. Battery Levels
Display: Two progress bars show the battery percentage of each UAV.
Details:
The percentage value is displayed in the center of each bar.
XY coordinates of each UAV are displayed below their respective bars.
Behavior:
Batteries deplete gradually while UAVs are active.
UAVs with empty batteries are destroyed and deactivated.
5. UAV Representation
Elements: Two circles represent the UAVs.
Behavior:
Initial State: Both UAVs are grounded (inactive), with full batteries.
Activation:
1 + U: Activates the first UAV.
2 + U: Activates the second UAV.
Deactivation:
1 + I: Deactivates the first UAV.
2 + I: Deactivates the second UAV.
Movement:
W, S, A, D: Controls the first UAV when active.
Arrow keys: Controls the second UAV when active.

#### Moving the drone
![Moving the drone](https://github.com/user-attachments/assets/c2f2f0f0-1fce-42a4-96c7-11f6bd7f8ae0)

Collision Handling:
If UAVs collide, move out of the map, enter the no-fly zone, or run out of battery, they are destroyed (removed from the map and permanently deactivated).

#### Destroyed drone
![Destroyed drone](https://github.com/user-attachments/assets/6ca5a6fa-882e-41ea-afb4-9c5014780427)

## User Interactions and Controls
| Action                   | Key/Control       | Description                                                      |
|--------------------------|-------------------|------------------------------------------------------------------|
| Activate first UAV       | `1 + U`          | Turns on the first UAV.                                          |
| Activate second UAV      | `2 + U`          | Turns on the second UAV.                                         |
| Deactivate first UAV     | `1 + I`          | Turns off the first UAV.                                         |
| Deactivate second UAV    | `2 + I`          | Turns off the second UAV.                                        |
| Move first UAV           | `W, S, A, D`     | Moves the first UAV in the specified directions.                 |
| Move second UAV          | Arrow keys       | Moves the second UAV in the specified directions.                |
| Drag no-fly zone         | Left mouse button| Moves the no-fly zone.                                           |
| Resize no-fly zone       | Right mouse button| Expands the no-fly zone while held.                              |
| Reset no-fly zone        | `R`              | Resets the no-fly zone to its original size and position.         |

## Author
Mila Milović SV22-2021

