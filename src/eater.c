#include <robotdriver/motioncontroller.h>
#include <robotdriver/speedcontroller.h>
#include <robotdriver/headingcontroller.h>
#include <robotdriver/motordriver.h>
#include <robotdriver/toolboxdriver.h>
#include <pathfollower/pathfollower.h>
#include <stdlib.h>
#include <stdio.h>
#include "robot.h"
#include "eater.h"

// Ax-12 that controls the brushes
// middle : rayon 235.5mm
#define AXBRUSH 133
#define BRUSHSPEED 500
#define BRUSHON 812
#define BRUSHOFF 280
#define BRUSHMIDDLE 720

#define AXDOOR 141
#define DOORSPEED 400
#define AXDOORCLOSED 670
#define AXDOOROPEN 248

// Ax-12 that controls the conveyor belt
//TODO Check rotation direction

#define AXCONVEYOR    135
#define CONVEYORSPEED (-1023)

// Ax-12 that control the brushes
// TODO Check rotation direction

#define AXLEFTBRUSH  143
#define AXRIGHTBRUSH 142
#define AXLEFTSPEED  1023
#define AXRIGHTSPEED (-1023)

static void (*setBrushCallback)(void) = NULL;
static void (*unsetBrushCallback)(void) = NULL;
static void (*brushMiddleCallback)(void) = NULL;

void initBrush() {
	axSetTorqueSpeed(AXBRUSH, -1, BRUSHSPEED, 0);
	axSetTorqueSpeed(AXDOOR, -1, DOORSPEED, 0);
}

void setBrush() {
	axMove(AXBRUSH, BRUSHON, setBrushCallback);
}
void setBrushMiddle() {
	axMove(AXBRUSH, BRUSHMIDDLE, brushMiddleCallback);
}
void unsetBrush() {
	axMove(AXBRUSH, BRUSHOFF, unsetBrushCallback);
}

void setSetBrushCallback(void (*callback)(void)){
	setBrushCallback = callback;
}
void setBrushMiddleCallback(void (*callback)(void)){
	brushMiddleCallback = callback;
}
void setUnsetBrushCallback(void (*callback)(void)){
	unsetBrushCallback = callback;
}

void openDoor() {
	axMove(AXDOOR, AXDOOROPEN, NULL);
}
void closeDoor() {
	axMove(AXDOOR, AXDOORCLOSED, NULL);
}
void startEater(){
	axSetTorqueSpeed(AXCONVEYOR,   -1, CONVEYORSPEED, 1);
	axSetTorqueSpeed(AXLEFTBRUSH,  -1, AXLEFTSPEED,   1);
	axSetTorqueSpeed(AXRIGHTBRUSH, -1, AXRIGHTSPEED,  1);
}

void stopEater(){
	axSetTorqueSpeed(AXCONVEYOR,   -1, 0, 1);
	axSetTorqueSpeed(AXLEFTBRUSH,  -1, 0, 1);
	axSetTorqueSpeed(AXRIGHTBRUSH, -1, 0, 1);
}

// ############## action moves : ##############
static int actionState = 0;

static void robotEmpty() {
	actionState = 10;
	stopEater();
	closeDoor();
	setCurrentLocation(1162, 1386);
	setActiveDetectors(all);
	actionState = 11;
}
static void unloadCubes() {
	fastSpeedChange(0);
	setBlockingCallback(NULL);
	enableHeadingControl(1);
	startEater();
	openDoor();
	scheduleIn(5000, robotEmpty);
}
static void turnEnd5() {
	setActiveDetectors(none);
	queueSpeedChange(-0.1, NULL);
	enableHeadingControl(0);
	setBlockingCallback(unloadCubes);
}
static void nearZone(struct motionElement * a) {
	actionState = 9;
	if(a) {}
	printf("finished eating\n");
	setTargetHeading(90,turnEnd5);
}
static void stopEating() {
	actionState = 8;
	setActiveDetectors(rear);
	stopEater();
	queueSpeedChange(-0.2, NULL);
	queueStopAt(-230, nearZone);
}

static void stopAndEat();
static void moveAndEat() {
	fastSpeedChange(0.01);
	setSideBlockingCallback(stopAndEat);
}
static void stopAndEat() {
	fastSpeedChange(-0.01);
	setSideBlockingCallback(NULL);
	scheduleIn(800, moveAndEat);
}
static void turnEnd4() {
	setActiveDetectors(none);
	queueSpeedChange(0.1, NULL);
	queueSpeedChangeAt(40, 0.01, NULL);
	setRobotDistance(0);
	setSideBlockingCallback(stopAndEat);
}
static void backwardFinished(struct motionElement * a) {
	actionState = 7;
	if(a) {}
	setTargetHeading(115, turnEnd4);
}
static void turnEnd3() {
	actionState = 6;
	setRobotDistance(0);
	setBrush();
	queueSpeedChange(-0.1, NULL);
	queueStopAt(-120, backwardFinished);
}

static void backFromCubes(struct motionElement * a) {
	actionState = 5;
	if(a) {}
	setTargetHeading(200, turnEnd3);
}
static void turnEnd2() {
	actionState = 4;
	setRobotDistance(0);
	fastSpeedChange(0);
	queueSpeedChange(-0.2, NULL);
	queueStopAt(-60, backFromCubes);
	setSideBlockingCallback(NULL);
}
static void turnBack() {
	actionState = 3;
	//startEater();
	fastSpeedChange(0);
	setTargetHeading(110, turnEnd2);
	setSideBlockingCallback(turnEnd2);
}
static void turnEnd() {
	actionState = 2;
	setBrushMiddle();
	queueSpeedChange(0.1, NULL);
	setSideBlockingCallback(turnBack);
}
// start collecting cubes : first destroy cube stack
void startEaterAction() {
	actionState = 1;
	scheduleIn(20000, stopEating);
	setTargetHeading(150, turnEnd);
	initBrush();
	setActiveDetectors(rear);
}

void pauseEaterAction() {
	// stop turning
    setTargetHeading(getRobotHeading(), NULL);
    // stop moving
    clearMotionQueue();
    queueSpeedChange(0, NULL);
}
void resumeEaterAction() {
	switch (actionState) {
		case 1:
			setTargetHeading(150, turnEnd);
			break;
		case 2:
			turnEnd();
			break;
		case 3:
			turnBack();
			break;
		case 4:
			queueSpeedChange(-0.2, NULL);
			queueStopAt(-60, backFromCubes);
			break;
		case 5:
			backFromCubes(NULL);
			break;
		case 6:
			queueSpeedChange(-0.1, NULL);
			queueStopAt(-120, backwardFinished);
			break;
		case 7:
			backwardFinished(NULL);
			break;
		case 8:
			stopEating();
			break;
		case 9:
			nearZone(NULL);
			break;
	}
}
int isEaterActionFinished() {
	return actionState == 11;
}
