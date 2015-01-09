/******************************************************************************
 * Author: Arlen Strausman
 * 
 * This is a Pool simulation game written for my computer graphics course.
 * It is built upon a collision detection and animation program 
 * designed by Thomas Kelliher. 
 * 
 * Functions written: createAimer, rackBoard, shoot, rackCue, aim, raisePower,
 * lowerPower, elasiticityUp, elasticityDown, ballSizeUp, ballSizeDown, moveCueUp
 * moveCueDown, moveCueLeft, moveCueRight,
 * 
 * Functions modified: readFile, createCircle, createBoard, display, init, 
 * keyboard, mouse, idle, main
 * 
 * ****************************************************************************


Arlen Strausman's Pool Simulator Game:

===========================================================================================
-------------------------------------------------------------------------------------------
GOAL:
-------------------------------------------------------------------------------------------
- The goal of the game is simple; pocket all balls that are not the cue. 

-------------------------------------------------------------------------------------------
SHOOTING:
-------------------------------------------------------------------------------------------
- Click within the white "aiming circle" to choose the direction of your shot 
  (represented by a small pink "aiming ball". 
- To raise or lower the power of your shot, use the '+' and '-' keys.
- When you are satisfied with your direction and power, hit the space bar to shoot.

-------------------------------------------------------------------------------------------
RE-RACKING:
-------------------------------------------------------------------------------------------
- To rerack the cue, use the 'c' key. To rerack the board, use the 'r' key.

-------------------------------------------------------------------------------------------
BREAKING:
-------------------------------------------------------------------------------------------
- When either the cue ball or all of the balls have been recently racked, you may use 
  the 'w', 'a', 's' and 'd' keys to move the ball to the desired location within the legal 
  breaking area.

-------------------------------------------------------------------------------------------
COOKIES (EXTRA FEATURES):
-------------------------------------------------------------------------------------------
- To raise the size and mass of the ball, use the 'B' key. To lower them, 'b'.
- To raise and lower the elasticity of collision, use the 'E' and 'e' keys, respectively



Built upon skeleton Pool.cpp class provided by Thomas Kelliher (my professor)
