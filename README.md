TO COMPILE THE PROGRAM:
    
    gbacc MazeRunner.c checkPlayerWin.s getFrameDigit.s

TO PLAY:
    Use an emulator to play the .gba file


GOAL OF GAME:
    Try to escape the maze. To open gates, the player must first collect the associated key. Make it through the final gate to escape the maze!


TIPS FOR IAN:
    
    A file named MazeTemplate.png is included in the github. This file has the correct path to win the game colored in green. All other paths colored in red should not be taken and are just distractions.

    There is a WIN and LOSE screen for each outcome. When the time runs out, the LOSE screen will appear. If you escape the maze by following the path, the WIN screen appears. 

    Once the game ends (either when the player wins or loses) then the game must be restarted through the GBA emulator.

    There is also a visual bug for the 3rd key and 3rd gate. I believe this is emulator based as they are implemented the same as the other keys and gates in the code that function properly.

    Hope you have fun!!
