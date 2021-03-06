#include <FEHLCD.h>
#include <FEHServo.h>
#include <FEHIO.h>
#include <FEHUtility.h>
#include <FEHMotor.h>
#include <FEHRPS.h>
#include <FEHServo.h>
#include <FEHBuzzer.h>
#include <math.h>

#define PI 3.14159265

/*
* * * * Constants * * * *
*/
enum WheelID {LEFTWHEEL = 0, RIGHTWHEEL = 1};

enum DriveDirection {FORWARD = 1, BACKWARD = 0};
enum TurnDirection {RIGHT = 1, LEFT = 0};

enum FaceDirection {NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3};

enum LightColor {cNONE = 0, cRED = 1, cBLUE = 2};

//4.3654 encoder counts = 1 inch

int redSwitchDir;
int blueSwitchDir;
int whiteSwitchDir;

 float xRead, xCorrection, yRead, yCorrection, bumpheading;

/*
* * * * Variables * * * *
*/
/** RUNTIME DATA **/
int seconds = 0;
int RPSoffset = 0;

/** WHEELS **/
//FEHMotor var(FEHMotor::port, float voltage);
FEHMotor left_wheel(FEHMotor::Motor1, 7.2);   /** CHANGE THIS **/
int g_left_wheel_percent = 0;
FEHMotor right_wheel(FEHMotor::Motor0, 7.2);    /** CHANGE THIS **/
int g_right_wheel_percent = 0;

/** BUMP SWITCHES **/
DigitalInputPin bottom_left_bump(FEHIO::P0_6);   /** CHANGE THIS **/
DigitalInputPin bottom_right_bump(FEHIO::P2_0);    /** CHANGE THIS **/
DigitalInputPin top_left_bump(FEHIO::P2_4);   /** CHANGE THIS **/
DigitalInputPin top_right_bump(FEHIO::P2_6);   /** CHANGE THIS **/

/** CDS CELL **/
AnalogInputPin CDSCell(FEHIO::P3_0);   /** CHANGE THIS **/

/** SERVO ARM **/
FEHServo longarm(FEHServo::Servo0);   /** CHANGE THIS **/

/** SHAFT ENCODERS **/
DigitalEncoder right_encoder(FEHIO::P3_2);   /** CHANGE THIS **/
DigitalEncoder left_encoder(FEHIO::P3_0);   /** CHANGE THIS **/


/*
* * * * Function prototypes * * * *
*/
/** MOTORS **/
void setWheelPercent(WheelID wheel, float percent);
void stopAllWheels();
/** BASIC MOVEMENT **/
void driveStraight(DriveDirection direction, float speed, float seconds);
void driveStraight(DriveDirection direction, float speed);
void turn(TurnDirection direction, float speed, float seconds);
void turn(TurnDirection direction, float speed);
/** ADVANCED MOVEMENT **/
void adjustHeadingRPS(float heading, float motorPercent);
void adjustHeadingRPS2(float heading, float motorPercent, float tolerance);
void adjustXLocationRPS(float x_coordinate, float motorPercent, FaceDirection dirFacing);
void adjustYLocationRPS(float y_coordinate, float motorPercent, FaceDirection dirFacing);
void driveStraightRPS(DriveDirection direction, float speed, float distance); //NOT IMPLEMENTED
void turn90RPS(TurnDirection direction); //NOT IMPLEMENTED
void goUpRamp();
void driveStraightEnc(DriveDirection direction, float speed, float distance);
void turnEnc(TurnDirection direction, float speed, float distance);
void turn90Enc(TurnDirection direction, float speed);
/** DATA ACQUISTITION **/
int getHeading(); //NOT IMPLEMENTED
bool isFrontAgainstWall();
bool isBackAgainstWall();
LightColor getLightColor();
/** OTHER **/
void resetSceen();
int inchToCounts(float inches);
/** DEBUG **/
char startupTest(); //NOT IMPLEMENTED
void printDebug();
void checkPorts();


/*
* * * * Functions * * * *
*/
/** MOTORS **/
void setWheelPercent(WheelID wheel, float percent) {
    switch (wheel) {
      case LEFTWHEEL:
        left_wheel.SetPercent(percent);
        g_left_wheel_percent = percent;
        break;
        //MANIPULATE WHEELS HERE
      case RIGHTWHEEL:
        right_wheel.SetPercent(percent*1.01);
        g_right_wheel_percent = percent;
        break;
      default:
        LCD.WriteLine("setWheelPercent ERROR");
    }
}
void stopAllWheels() {
    left_wheel.SetPercent(0);
    g_left_wheel_percent = 0;
    right_wheel.SetPercent(0);
    g_right_wheel_percent = 0;
}


/** BASIC MOVEMENT **/
void driveStraight(DriveDirection direction, float speed, float seconds) {
    if (direction == FORWARD) {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, speed);
    } else {
        setWheelPercent(RIGHTWHEEL, -speed);
        setWheelPercent(LEFTWHEEL, -speed);
    }

    Sleep(seconds);

    stopAllWheels();
}
void driveStraight(DriveDirection direction, float speed) {
    if (direction == FORWARD) {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, speed);
    } else {
        setWheelPercent(RIGHTWHEEL, -speed);
        setWheelPercent(LEFTWHEEL, -speed);
    }
}

void turn(TurnDirection direction, float speed, float seconds) {
    if (direction == RIGHT) {
        setWheelPercent(RIGHTWHEEL, -speed);
        setWheelPercent(LEFTWHEEL, speed);
    } else {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, -speed);
    }

    Sleep(seconds);

    stopAllWheels();
}
void turn(TurnDirection direction, float speed) {
    if (direction == RIGHT) {
        setWheelPercent(RIGHTWHEEL, -speed);
        setWheelPercent(LEFTWHEEL, speed);
    } else {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, -speed);
    }
}

/** ADVANCED MOVEMENT **/
void adjustHeadingRPS(float heading, float motorPercent, float tolerance) {
    //Turn speed failsafe
    //float encoderCounts = 3;
    int loopCount = 0;

    //See how far we are away from desired heading
    float difference = heading - RPS.Heading();
    if (difference < 0) difference *= -1; //Absolute value of difference
    while (difference > tolerance) {
        printDebug();

        //Find what direction we should turn to
        bool turnRight;
        if (RPS.Heading() < heading) {
            //Less than desired -> Increase degrees
            turnRight = false;
        } else {
            //Greater than desired -> Decrease degrees
            turnRight = true;
        }
        if (difference > 180) {
            turnRight = !turnRight;
        }

        //Turn
        if (turnRight) {
            turn(RIGHT, motorPercent, .1);
        } else {
            turn(LEFT, motorPercent, .1);
        }

        Sleep(50);

        //Recalculate difference
        difference = heading - RPS.Heading();
        if (difference < 0) difference *= -1;

        //Failsafe
        loopCount++;
        if (loopCount > 400) {
            motorPercent *= .8;
            //encoderCounts *= .8;
            loopCount = 0;
        }
        if (motorPercent <= 4) {
            //Things have gone wrong
            break;
        }

    }
    stopAllWheels();
    printDebug();
}

void adjustHeadingRPS2(float heading, float motorPercent, float tolerance) {
    //Timeout
    int timeoutCount = 0;
    int widdleCount = 0;
    bool didTimeout = false;
    bool didError = false;
    float minSpd = 15;
    //float lastHeading = RPS.Heading();

    //Make sure we arnt going with too low of a motor power
    if (motorPercent <= minSpd) {
        motorPercent = minSpd;
    }

    //See how far we are away from desired heading
    float difference = heading - RPS.Heading();
    if (difference < 0) difference *= -1; //Absolute value of difference
    while (difference > tolerance && !didTimeout && !didError) {
        printDebug();

        //Check if we are inside the RPS zone
        if (RPS.Heading() < 0) {
            didError = true;
            break;
        }

        //Find what direction we should turn to
        bool turnRight;
        if (RPS.Heading() < heading) {
            //Less than desired -> Increase degrees
            turnRight = false;
        } else {
            //Greater than desired -> Decrease degrees
            turnRight = true;
        }
        if (difference > 180) {
            turnRight = !turnRight;
        }

        //Turn
        if (turnRight) {
            turn(RIGHT, motorPercent, .1);
        } else {
            turn(LEFT, motorPercent, .1);
        }

        Sleep(50);

        //Recalculate difference
        difference = heading - RPS.Heading();
        if (difference < 0) difference *= -1;

        //Timeout
        if (timeoutCount > 50) {
            didTimeout = true;
        } else {
            timeoutCount++;
        }

        if (difference < 30) {
            widdleCount++;
            if (widdleCount > 5) {
                didTimeout = true;
            }
        }
    }

    //Stop all the wheels and wait a few seconds to give them time to stop
    stopAllWheels();
    printDebug();
    Sleep(100);

    //Recalculate difference and recall method if we are not close enough
    if (!didError && motorPercent != minSpd) {
        difference = heading - RPS.Heading();
        if (difference < 0) difference *= -1;
        if (difference > tolerance) {
            adjustHeadingRPS2(heading, minSpd, tolerance);
        }
    }
}

void adjustXLocationRPS(float x_coordinate, float motorPercent, FaceDirection dirFacing, float tolerance) {
    //Facing east
    if (dirFacing == EAST) {
        while(RPS.X() < x_coordinate - tolerance || RPS.X() > x_coordinate + tolerance)
        {
            printDebug();

            //We are in front facing away -> move backwards
            if(RPS.X() > x_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(BACKWARD, motorPercent, 0.1);
            }
            //We are behind facing towards -> move forwards
            else if(RPS.X() < x_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(FORWARD, motorPercent, 0.1);
            }
        }
    }

    //Facing west
    if (dirFacing == WEST) {
        while(RPS.X() < x_coordinate - tolerance || RPS.X() > x_coordinate + tolerance)
        {
            printDebug();

            //We are in front facing towards -> move backwards
            if(RPS.X() > x_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(FORWARD, motorPercent, 0.1);
            }
            //We are behind facing away -> move forwards
            else if(RPS.X() < x_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(BACKWARD, motorPercent, 0.1);
            }
        }
    }
}

void adjustYLocationRPS(float y_coordinate, float motorPercent, FaceDirection dirFacing, float tolerance) {
    //Facing north
    if (dirFacing == NORTH) {
        while(RPS.Y() < y_coordinate - tolerance || RPS.Y() > y_coordinate + tolerance)
        {
            printDebug();

            //We are in front facing away -> move backwards
            if(RPS.Y() > y_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(BACKWARD, motorPercent, 0.1);
            }
            //We are in behind facing towards -> move forwards
            else if(RPS.Y() < y_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(FORWARD, motorPercent, 0.1);
            }

            Sleep(50);
        }
    }

    //Facing south
    if (dirFacing == SOUTH) {
        while(RPS.Y() < y_coordinate - tolerance || RPS.Y() > y_coordinate + tolerance)
        {
            printDebug();

            //We are in front facing towards -> move forwards
            if(RPS.Y() > y_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(FORWARD, motorPercent, 0.1);
            }
            //We are in behind facing away -> move backwards
            else if(RPS.Y() < y_coordinate)
            {
                //pulse the motors for a short duration in the correct direction
                driveStraight(BACKWARD, motorPercent, 0.1);
            }

            Sleep(50);
        }
    }
}

void goUpRamp() {
    //Drive to right wall
    driveStraight(FORWARD, 30);
    while (!isFrontAgainstWall());
    stopAllWheels();
    Sleep(100);

    //Pivot
    driveStraight(BACKWARD, 30, 1.5);
    Sleep(.5);
    setWheelPercent(RIGHTWHEEL, 60);
    Sleep (1.1);
    stopAllWheels();
    Sleep(.5);
    driveStraight(BACKWARD, 30);
    while (bottom_left_bump.Value());
    stopAllWheels();
    Sleep(0.5);
    setWheelPercent(RIGHTWHEEL, -20);
    while (bottom_right_bump.Value());
    stopAllWheels();
    Sleep(1.0);

    //Drive up ramp
    driveStraight(FORWARD, 45);
    while (!isFrontAgainstWall());
    stopAllWheels();
    Sleep(0.2);


    driveStraight(BACKWARD, 30, 1.5);
    Sleep(.5);
    setWheelPercent(RIGHTWHEEL, 60);
    Sleep (1.1);
    stopAllWheels();
    Sleep(.5);
    driveStraight(BACKWARD, 30);
    while (!isBackAgainstWall());
    stopAllWheels();
    Sleep(1.0);

    driveStraight(FORWARD, 30, 2.5);
    stopAllWheels();
}

void driveStraightEnc(DriveDirection direction, float speed, float distance) {
    //Convert distance to counts
    int counts = inchToCounts(distance);

    //Reset encoder counts
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    //Set both motors to desired percent
    if (direction == FORWARD) {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, speed-1);
    } else {
        setWheelPercent(RIGHTWHEEL, -speed-1);
        setWheelPercent(LEFTWHEEL, -speed);
    }

    //While the average of the left and right encoder are less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);

    //Turn off motors
    stopAllWheels();
}

void turnEnc(TurnDirection direction, float speed, float distance) {
    //Convert distance to counts
    int counts = inchToCounts(distance);

    //reset encoder counts
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    //set both motors to desired percent
    if (direction == RIGHT) {
        setWheelPercent(RIGHTWHEEL, -speed);
        setWheelPercent(LEFTWHEEL, speed);
    } else {
        setWheelPercent(RIGHTWHEEL, speed);
        setWheelPercent(LEFTWHEEL, -speed);
    }

    //while the average of the left and right encoder are less than counts, keep running motor
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts) {
        LCD.WriteRC("Encoder",0,0);
        LCD.WriteRC(right_encoder.Counts(),1,0);
        LCD.WriteRC(left_encoder.Counts(),2,0);
    }

    //turn off motors
    stopAllWheels();
}

void turn90Enc(TurnDirection direction, float speed) {
    //5.89 inches for 90 degrees (for each wheel)
    turnEnc(direction, speed, 5.89);
}

/** DATA ACQUISTITION **/
bool isFrontAgainstWall() {
    return top_left_bump.Value()==false && top_right_bump.Value()==false;
}
bool isBackAgainstWall() {
    return bottom_left_bump.Value()==false && bottom_right_bump.Value()==false;
}
LightColor getLightColor() {
    if (CDSCell.Value() <= 1.1) {
        return cRED;
    } else if (CDSCell.Value() > 1.1 && CDSCell.Value() <= 2.4) {
        return cBLUE;
    } else {
        return cNONE;
    }
}

/** OTHER **/
void resetScreen() {
    LCD.Clear( FEHLCD::Black );
}
int inchToCounts(float inches) {
    //4.3654 encoder counts = 1 inch
    return 4.3654 * inches;
}

/** DEBUG **/
char startupTest() {
    //Test to make sure everything is reading normal values
    //Return 0 if everything is okay
    return 0;
}

void printDebug() {
    //Clear screen
    //resetScreen();

    //Print RPS data
    LCD.WriteRC("RPS DATA",0,0);
    LCD.WriteRC(RPS.X(),1,0); //update the x coordinate
    LCD.WriteRC(RPS.Y(),2,0); //update the y coordinate
    LCD.WriteRC(RPS.Heading(),3,0); //update the heading

    //Print error list
}

void checkPorts() {
    setWheelPercent(RIGHTWHEEL, 15);
    Sleep(100);
    stopAllWheels();

    Sleep(1.0);

    setWheelPercent(LEFTWHEEL, 15);
    Sleep(100);
    stopAllWheels();

    Sleep(1.0);

    while (true) {
        LCD.WriteRC(top_left_bump.Value(),0,0);
        LCD.WriteRC(top_right_bump.Value(),1,0);
        LCD.WriteRC(bottom_left_bump.Value(),2,0);
        LCD.WriteRC(bottom_right_bump.Value(),3,0);
    }
}


void encoderTest() {
    //void driveStraightEnc(DriveDirection direction, float speed, float distance);
    //void turnEnc(TurnDirection direction, float speed, float distance);
    //void turn90Enc(TurnDirection direction, float speed);

    resetScreen();

    //Reset encoder counts
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    turn90Enc(RIGHT, 20);

    while (true) {
        LCD.WriteRC("Encoder",0,0);
        LCD.WriteRC(right_encoder.Counts(),1,0);
        LCD.WriteRC(left_encoder.Counts(),2,0);
    }
}


/** Competition steps **/
void doBottomSwitches() {
    //**** BOTTOM SWITCHES ****//

    //Turn to face switches
    setWheelPercent(RIGHTWHEEL, 40);
    Sleep(1.5);
    stopAllWheels();
    driveStraight(FORWARD, 30);
    while (RPS.X() > 8);
    stopAllWheels();
    adjustXLocationRPS(8, 30, NORTH, 1.0);
    adjustHeadingRPS2(90, 20, 1.0);

    //Drive till bump against wall
    driveStraight(FORWARD, 50);
    while (!isFrontAgainstWall());
    stopAllWheels();

    bumpheading = RPS.Heading();
    yRead = RPS.Y();
    yCorrection = 25.5 - yRead;

    //Backup to a certain spot
    driveStraight(BACKWARD, 20, 1.6);
    adjustYLocationRPS(22, 20, NORTH, 0.8);

    //Decide which switch to press
    //red - left, white - middle, blue - right
    //1 - forward, 2 - backwards
    redSwitchDir = 2; //RPS.RedSwitchDirection();
    blueSwitchDir = 2; //RPS.BlueSwitchDirection();
    whiteSwitchDir = 2; //RPS.WhiteSwitchDirection();


    if (whiteSwitchDir == 2) {
        //Set arm height
        longarm.SetDegree(54);

        //Drive forward
        driveStraight(FORWARD, 30, 1.3);

        //Backup
        driveStraight(BACKWARD, 20, 1.4);
        adjustYLocationRPS(22, 20, NORTH, 0.8);
    }

    if (redSwitchDir == 2) {
        //Turn left
        turn(LEFT, 30, 0.5);
        adjustHeadingRPS2(180, 25, 1.0);

        //Drive
        float originX = RPS.X();
        adjustXLocationRPS(originX - 2, 20, WEST, 1.0);

        //Turn straight
        adjustHeadingRPS2(96, 25, 1.0);

        //Set arm height
        longarm.SetDegree(54);

        //Drive forward
        driveStraight(FORWARD, 30, 1.2);

        //Backup
        longarm.SetDegree(55);
        driveStraight(BACKWARD, 30, 1.0);
        adjustHeadingRPS2(0, 25, 1.0);

        //Go back to original position
        adjustXLocationRPS(originX-1.5, 20, EAST, 1.0);
    }

    if (blueSwitchDir == 2) {
        //Turn left
        //turn(LEFT, 30, 0.5);
       // adjustHeadingRPS2(0, 25, 1.0);

        //Drive
        //int originX = RPS.X();
//        adjustXLocationRPS(originX + 2, 20, EAST, 1.0);

        //Turn straight
        longarm.SetDegree(100);
        adjustHeadingRPS2(90, 25, 1.0);

        //Set arm height
        longarm.SetDegree(54);

        //Drive forward
        driveStraight(FORWARD, 30, 1.2);

        //Backup
        longarm.SetDegree(55);
        driveStraight(BACKWARD, 30, 1.0);
        //adjustHeadingRPS2(180, 25, 1.0);

        //Go back to original position
        //adjustXLocationRPS(originX - 2, 20, WEST, 1.0);
    }
}

void doDumbbell() {
    //**** DUMBBELL ****//

    //Turn (north -> southeast) to face towards dumbbell
    turn(RIGHT, 40, 1.5);
    adjustHeadingRPS2(325, 20, 1.0);

    //Drive for a little bit
    driveStraight(FORWARD, 40);
    while (RPS.Y() > 16.5);
    stopAllWheels();

    //Turn to the LEFT to face right wall
    turn(LEFT, 30, 0.7);
    adjustHeadingRPS2(0, 20, 1.3);

    //Drive until bump against right wall in front of dumbbell
    driveStraight(FORWARD, 40);
    while (!isFrontAgainstWall());
    //LCD.WriteRC("X we want", 6, 0);
    //LCD.WriteRC(RPS.X(), 7, 0);
    stopAllWheels();


    //Backup slighty to get apart from wall then turn (east -> south) to face dumbbell
    driveStraight(BACKWARD, 30, 1.0);
    Sleep(100);
    turn(RIGHT, 30.0, 1.25);
    adjustHeadingRPS2(270, 15, .8);
    Sleep(100);

    //Backup
    adjustYLocationRPS(18.75-yCorrection, 20, SOUTH, 0.8);
    Sleep(100);

    //Lower longarm to dumbbell height
    longarm.SetDegree(15);
    Sleep(0.5);

    //Adjust heading to face dumbbell
    adjustHeadingRPS2(270, 15, 0.8);
    Sleep(250);

    //Drive forward until arm is under dumbbell
    adjustYLocationRPS(15, 20, SOUTH, 0.8);
    Sleep(250);

    //Raise arm to pickup dumbbell
    longarm.SetDegree(110);
    Sleep(0.5);
}

void doMoveToTop() {
    //**** MOVE TO TOP ****//

    //Get close to ramp
    driveStraight(BACKWARD, 40, 1.5);
    adjustYLocationRPS(26, 20, SOUTH, 0.8);
    Sleep(250);

    //Adjust heading (north -> east)
    adjustHeadingRPS2(269, 20, 0.8);
    Sleep(100);

    //Lift up dumbbell
    longarm.SetDegree(125);
    Sleep(0.5);

    //Drive up ramp
    driveStraight(BACKWARD, 70, 2.5);
    Sleep(1.0);

    //Lower longarm and turn south
    longarm.SetDegree(110);
    adjustHeadingRPS2(270, 30, 1.0);
    Sleep(1.0);
}

void doButtons() {
    //**** BUTTONS ****//

    if (!(RPS.X() > 25.5 && RPS.X() < 26.5)) {
        //Adjust heading (? -> east)
        turn(LEFT, 40, 1.0);
        adjustHeadingRPS2(0, 20, 1.5);
        Sleep(200);

        //Adjust x position to be in line with buttons
        adjustXLocationRPS(26.0, 25, EAST, .6);
        Sleep(200);
    }
    else {
        turn(LEFT, 30, 1.0);
    }

    //Turn to face buttons
    turn(LEFT, 40, 1.0);
    adjustHeadingRPS2(bumpheading,15,.6);
    Sleep(200);

    //Drive forward to get close to buttons
    driveStraight(FORWARD, 45, 1.7);
    Sleep(200);
    adjustHeadingRPS2(bumpheading,15,.6);
    driveStraight(FORWARD, 45, .9);
    adjustYLocationRPS(61, 30, NORTH, 1.0);
    Sleep(200);
    //adjustHeadingRPS2(90, 20, 1.0);
    Sleep(200);

    //Check button color
    LightColor buttonColor;
    buttonColor = getLightColor();

    if (buttonColor == cRED) {
        //LCD.WriteRC("Red",5,0);

        //Adjust arm to be in line with button
        longarm.SetDegree(130);
        Sleep(1.0);

        //Adjust y position to press button and wait 5 seconds
        driveStraight(FORWARD, 35, 2);
        Sleep(5.5);
    }

    if (buttonColor == cBLUE || buttonColor == cNONE) {
        //LCD.WriteRC("Blue",5,0);

        //Adjust y position to press button and wait 5 seconds
        driveStraight(FORWARD, 40, 2);
        Sleep(5.5);

    }

    //Backup for a sec
    driveStraight(BACKWARD, 30, 1.5);

    longarm.SetDegree(95);


}

void doDumbbellDrop() {
    //Backup to get in line with wall adjacent to dumbbell drop
    driveStraight(BACKWARD, 50, 1.8);
    adjustYLocationRPS(47.5, 20, NORTH, 0.8);
    Sleep(200);

    //Turn (north -> west)
    turn(LEFT, 30, 1.0);
    adjustHeadingRPS2(178, 20, 0.8);
    Sleep(200);

    //Drive straight till bump against wall
    driveStraight(FORWARD, 80);
    while (!isFrontAgainstWall());
    stopAllWheels();
    Sleep(200);

    //Backup
    driveStraight(BACKWARD, 30, 1.1);
    Sleep(200);

    //Turn to face the dropoff
    turn(RIGHT, 30, 1.5);
    Sleep(200);

    //Bump against dropoff
    driveStraight(FORWARD, 40);
    while (!(top_left_bump.Value() == 0 || top_right_bump.Value() == 0));
    Sleep(0.5);
    stopAllWheels();
    Sleep(200);

    //Backup a little bit
    driveStraight(BACKWARD, 30, 1.1);

    //Drop off dumbbell
    longarm.SetDegree(20);
    Sleep(0.5);

    //Drive backwards to drop off dumbbell
    driveStraight(BACKWARD, 30, 1.0);
    Sleep(200);

    //Raise longarm back up
    longarm.SetDegree(110);
    Sleep(200);
}

void doTopSwitches() {
    //Bump back against wall
    setWheelPercent(LEFTWHEEL, -40);
    Sleep(0.5);
    stopAllWheels();
    setWheelPercent(RIGHTWHEEL, -40);
    while (bottom_left_bump.Value() == 1);
    stopAllWheels();
    setWheelPercent(RIGHTWHEEL, -40);
    //driveStraight(BACKWARD, 40);
    while (!isBackAgainstWall());
    stopAllWheels();

    //Drive straight a little bit
    driveStraight(FORWARD, 40, 0.8);

    //Turn 180 to be facing toward switches
    turn(RIGHT, 40, 1.8);
    Sleep(200);

    //Drive straight till bump against wall
    driveStraight(FORWARD, 30);
    while (top_right_bump.Value() == 1);
    setWheelPercent(LEFTWHEEL, 40);
    while (!isFrontAgainstWall());
    stopAllWheels();
    Sleep(200);

    //Backup a little bit TO LOCATION BEHIND WHITE
    driveStraight(BACKWARD, 30, 1.8);
    Sleep(200);


   //TOP SWITCH LOGIC ADDED 3/30 BY KEV AND JAKE
    /*
    if (whiteSwitchDir == 1) {
        //Set arm height
        longarm.SetDegree(35);

        //Drive forward
        driveStraight(FORWARD, 30, 1.3);

        //Backup
         driveStraight(BACKWARD, 30, 1.3);
                //adjustYLocationRPS(20, 20, NORTH, 0.8);

         if (blueSwitchDir == 2 && redSwitchDir == 2 ){
             //Turn (south -> east)
             turn(LEFT, 40, .75);
         }
    }

    if (blueSwitchDir == 1) {
        //Turn right
        turn(LEFT, 30, 0.3);
                //adjustHeadingRPS2(68, 20, 1.2);
        Sleep(200);
        //Set arm height
        longarm.SetDegree(35);
        Sleep(200);
        //Drive forward
        driveStraight(FORWARD, 30, 1.8);

        //Backup
         driveStraight(BACKWARD, 30, 1.8);
                //adjustYLocationRPS(20, 20, NORTH, 0.8);
        Sleep(200);


         if (redSwitchDir == 2 ){
             //Turn (south -> east)
             turn(LEFT, 40, .6);
         }
         else{
            turn(RIGHT, 30, 0.3);
         }
    }

    if (redSwitchDir == 1) {
        //Turn left
        turn(RIGHT, 30, 0.4);
                //adjustHeadingRPS2(114, 20, 1.4);

        //Set arm height
        longarm.SetDegree(35);

        //Drive forward
        driveStraight(FORWARD, 30, 1.8);

        //Backup
        driveStraight(BACKWARD, 30, 1.8);
                //adjustYLocationRPS(20, 20, NORTH, 0.8);

        //Turn (south -> east)
        turn(LEFT, 40, 1.0);
    }
    */

    //Turn (south -> east)
    turn(LEFT, 40, 1.0);

    driveStraight(FORWARD, 60);
    while (RPS.X() < 24);
    stopAllWheels();

}

void doMoveToBottomAndEnd() {
    //Turn towards ramp
    adjustHeadingRPS2(90, 20, 1.0);
    Sleep(200);

    //Drive down
    driveStraight(BACKWARD, 50, 2.0);

    //Drive to finish button
    while (RPS.Y() > 22 || RPS.Y() <= 0);
    stopAllWheels();
    adjustYLocationRPS(22, 30, NORTH, 1.0);

    //Turn to face end
    adjustHeadingRPS2(208, 20, 1.0);
    driveStraight(FORWARD, 40);
}

void doMoveToBottomAndEnd2() {
    //Turn towards ramp
    //turn(LEFT, 40, 1.0);
    adjustHeadingRPS2(90, 20, 1.0);

    //Get close to down ramp
    driveStraight(BACKWARD, 40, 2.3);
    //adjustYLocationRPS(17, 30, NORTH, 1.0);

    //Adjust heading (? -> east)
    turn(RIGHT, 40, 0.8);
    adjustHeadingRPS2(0, 20, 1.5);
    Sleep(200);

    //Adjust x position to be in line with buttons
    adjustXLocationRPS(26.0, 25, EAST, .6);
    Sleep(200);

    //Turn towards ramp
    adjustHeadingRPS2(90, 20, 1.0);
    Sleep(200);

    //Drive down
    driveStraight(BACKWARD, 50, 2.5);

    //Drive to finish button
    while (RPS.Y() > 22 || RPS.Y() <= 0);
    stopAllWheels();
    adjustYLocationRPS(22, 30, NORTH, 1.0);

    //Turn to face end
    turn(LEFT, 40, 1.0);
    adjustHeadingRPS2(215, 20, 1.0);
    longarm.SetDegree(125);
    driveStraight(FORWARD, 40);

}


/*
* * * ****************************************************************** Main ************************************************************************* * * *
*/
int main(void)
{



    /*
        Individual competition
    */
    /* FUNCTIONS TO USE:
    void driveStraight(FORWARD or BACKWARD, speed, seconds);  //drive for a number of seconds
    void driveStraight(FORWARD or BACKWARD, speed);  //turn until stopAllWheels();
    void stopAllWheels();
    void turn(LEFT or RIGHT, speed, seconds);    //turn for a number of seconds (turns both wheels)
    void turn(LEFT or RIGHT, speed);    //turn until stopAllWheels(); (turns both wheels)
    bool isFrontAgainstWall();   //returns true or false
    bool isBackAgainstWall();    //returns true or false
    void setWheelPercent(LEFTWHEEL or RIGHTWHEEL, percent);   //move an individual wheel
    void adjustHeadingRPS(float heading, float motorPercent);
    void adjustYLocationRPS(float y_coordinate, float motorPercent, NORTH or SOUTH or EAST or WEST);
    void adjustXLocationRPS(float x_coordinate, float motorPercent, NORTH or SOUTH or EAST or WEST);
    void driveStraightEnc(DriveDirection direction, float speed, float distance);
    void turnEnc(TurnDirection direction, float speed, float distance);
    void turn90Enc(TurnDirection direction, float speed);
    */

    //**** STARTUP SEQUENCE ****//

    //Start off by clearing screen
    LCD.Clear( FEHLCD::Black );
    LCD.SetFontColor( FEHLCD::White );

    //Check ports
    //checkPorts();

    //Tests
    //encoderTest();

    //Initialize RPS
    RPS.InitializeTouchMenu();

    //Call startup test to make sure everything is okay
    if (startupTest() != 0) {
        while (true) {
            LCD.WriteLine("Startup Test Failure.");
            Sleep(50);
        }
    }

    //Setmin and Setmax for Servo
    longarm.SetMin(550);
    longarm.SetMax(2438);

    //Wait to start
    Sleep(2.0);
    float touch_x, touch_y;
    LCD.WriteLine("BUILD ID: 55");
    LCD.WriteLine("Touch to start");
    while (!LCD.Touch(&touch_x, &touch_y));

    resetScreen();
/*
    while (true){
    printDebug();
    Sleep (200);
    }
*/
    //Set initial position for longarm
    longarm.SetDegree(115);

    //Start from cds cell
    int timeout = 0;
    while(CDSCell.Value() > 1.1 ) {
        LCD.WriteLine (CDSCell.Value());
        Sleep(100);
        timeout++;
        if (timeout >= 300) break;
    }

    //----**** BEGIN RUN ****----//

    //**** BOTTOM SWITCHES ****//
    doBottomSwitches();

    //**** DUMBBELL ****//
    doDumbbell();

    //**** MOVE TO TOP ****//
    doMoveToTop();

    //**** BUTTONS ****//
    doButtons();

    //**** DUMBBELL DROP ****//
    //doDumbbellDrop();

    //**** TOP BUTTONS  ****//
    //doTopSwitches();

    //**** MOVE TO BOTTOM ****//
    doMoveToBottomAndEnd2();

}
