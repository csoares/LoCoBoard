/*
 * File:   Algorithms.h
 * Author: Christophe Soares
 *
 * Created on 6 de Outubro de 2009, 19:46
 * Copyright 2009 . All rights reserved.
 */

#ifndef INCLUDED_LOCOBOARD_H
#define INCLUDED_LOCOBOARD_H

#ifdef __APPLE__
#include <OpenCV/cv.h>
#include <OpenCV/cvaux.h>
#include <OpenCV/highgui.h>
#include <ApplicationServices/ApplicationServices.h>
// Threads
#define THREAD_RETURN_TYPE void*
#define SLEEP(x) usleep(x*1000)
#endif

#if defined(WIN32) || defined(WIN64)
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <windows.h>
#include <process.h>
// Threads
#define THREAD_RETURN_TYPE void
#define SLEEP(x) Sleep(x)
#endif

#ifdef LINUX
#include <QtGui/qcursor.h>
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <SDL.h>
#include <X11/extensions/XTest.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>
#include <sys/time.h>
#include <math.h>

// If true use webcam, else use a AVI File
#define cam 1
// If true multipoint system active
#define multiPoint 0
// If true debug mode is active
#define debug 0
// Depth of the Spiral
#define DEEP 5
// Define the Number of active Webcam - for now you could only use one !
#define camQUANTITY 1
// Background Model Gauss 1 - FGD 2
#define bgdMODEL 1
// Projection 1 - Front-side 0
#define PROJECTION 0
// using OSC Tbeta or not
//#define USING_OSC_TBETA

#ifdef USING_OSC_TBETA
#include "osc.h"
#endif /* USING_OSC_TBETA */

using namespace std;

///////////////////////////////////////////////////////////////////////
// Declaration of the variable and methods used by LoCoBoard         //
///////////////////////////////////////////////////////////////////////

// this struct will be use to store the coordinates of the Point of Interest (PI)
struct coordinate {
	int x, y;
	float sizeRect;
	int intensity;
};

// this struct will be use with the Algorithm of Erik van Kempen, this allow to store a line collection of PI on the first step of the algorithm detection
struct lineBlob {
	int min, max;
	unsigned int blobId;
	bool attached;
};

// this struct will be use with the Algorithm of Erik van Kempen, this allow to store a PI
struct blob {
	//unsigned int blobId;
	coordinate min, max;
	coordinate center;
};

// This is the main class of our application
class Algorithms {

	/////////////////////////////////////////////////////////////////////////
	////              METHODS                                            ////
	/////////////////////////////////////////////////////////////////////////

public:
	// Constructor
	Algorithms();
	// Destructor
	~Algorithms();
	// Constructor with initialization of threshold value
	Algorithms(int thresholdValue);
	// Initializtion of the lookup table for the spiral process, max is the maximum value to the spiral deep
	void distance(int max);
	// this method will print a circle in frame at the coordinate from center
	void printCircle(coordinate center, IplImage* frame);
	// this method will calculate the center from a collection of known values of PI
	coordinate processCenter();
	// this method will calculate the center from a set of PI on a frame, temp is one of this set of PI
	coordinate findCenter(coordinate temp, IplImage* frame);
	// this method will clear all the values that have been collected in the process
	void cleanVector();
	// this method will move the cursor of the OS to the coordinates x,y
	void mouseMove(int x, int y);
	// this method will check what is the current resolution on your PC - NOT IMPLEMENTED YET
	void screenSize();
	// this method will be used to to convert a PI from a resolution to an other (INT)
	int convertToResolution(int p, int escalaOrigem, int escalaDestino);
	// this method will be used to to convert a PI from a resolution to an other (FLOAT)
	float convertToResolutionF(float p, float escalaOrigem, float escalaDestino);
	// this algorithm have been adapted from the Erik van Kempen one
	vector<coordinate> detectBlobs(IplImage* frame, IplImage* finalFrame);
	// this is algorithm A1 - linear search on all the frame
	coordinate singlePointProcess(IplImage* workFrame);
	// this is algorithm A2 - linear search on all the frame with a stepping in columns
	coordinate singlePointProcess(IplImage* workFrame, int step);
	// this is algorithm A1 - second version - use pointer increment instead of two for cicle
	coordinate singlePointProcessPTR(IplImage* workFrame);
	// this is algorithm A3 - it use a linear seach, this will be interupted each time it finds a PI. After it will use findCenter to process the center.
	coordinate singlePointProcessPTR(IplImage* workFrame, int step);
	// part of the Algorithm A4 - first version - process jump spiral value in real time
	coordinate spiral(IplImage* workFrame, int x, int y);
	// part of the Algorithm A4 - second version - use a lookup table for the jump spiral value
	coordinate spiral2(IplImage* workFrame, int x, int y);
	// this is algorithm A4 - use some prediction between concurrent frames
	coordinate smartSinglePointProcess(IplImage* workFrame);
	// this method allow to report on the console line the values of PI
	void reportCoordinate(coordinate temp);
	// use this method to initialize the Classe Algorithms
	void initialize(int argc, char *argv[]);
	// this method will clean the vectors that have been use to collect the PI values - ie blob with a intesity superior to the threshold value
	void cleanVectorResults();
	// use this method after initialize to start the threads Input/Output and Capture Video
	void startProcessingMain();
	// this method will be the main loop of collecting images and process them with the chosen algorithm
	coordinate threadVideoProcessing_MainLoop();
	// this method will manage the press keyboard characters to take some action
	void threadGUI_MainLoop();
	// this method will emulate the mouse function based on the detected PI
	void interpreter();
	// this method turn on the vertical FLIP on frame
	void verticalFlipON();
	// this method turn off the vertical FLIP on frame
	void verticalFlipOFF();
	// this method turn on the BLURR on frame
	void blurON();
	// this method turn off the BLURR on frame
	void blurOFF();
	// this method allow us to set some capture parameter to the camera - FPS / Width / Height
	void setCapture(int captureFPS, int captureWidth, int captureHeight);
	// this method will print on the console line the properties of the capture
	void showCaptureProperties(CvCapture* cap);
	// this method will allow to define the value of the stepping - ie the number of pixel to be ignored between each reading
	void setStep(int value);
	// this method wiil allow to define the value of the stepping in the spiral movement
	void setJumpSpiral(int value);
	// this method will print the lookup table of the spiral on the comand line
	void printSpiralDistance();
	// this method will allow us to define the deep of the spiral
	void setSpiralDeep(int value);
	// this method will print on the console line the current time of the system
	void printTime();
	// this method will start the calibration process to calculate the resolution of the projection
	void startCalibrate();
	// this method based on the calibration process will calculate a matrix to process the conversion
	void setScale();
	// this method based on the matrix will allow us to convert a PI from the projection scale to the scale of our laptop, ie resolution
	coordinate convertToScale(coordinate temp);
	// this method will allow us to define the algorithm to be used - it could be change in real time
	void setAlgorithm(int value);
	//void windowIO(); NOT IMPLEMENTED
	// this method will allow to process the collection of value that will be used to calculate the MetricValue - this value will be use to set a reset of the background model
	void setTestCoordinates(int height,int width);
	// this method will calculate a metric value based on the coordinates
	float returnMetricValue(IplImage * frame);

	// Threads  to control the video capture & Algorithms, to capture the input keyboard
	static THREAD_RETURN_TYPE _threadGUI_Function(void*);
	//static THREAD_RETURN_TYPE _windowIO(void * obj); NOT IMPLEMENTED


	// MOUSE EVENTS CLIC for APPLE, LINUX AND WIN
#ifdef __APPLE__
	void postMouseEventApple(int x, int y, int width, int height, int click);
	void postMouseEventApple(int x, int y, int click);
#endif

#ifdef LINUX
	void postMouseEventLinux(int x, int y, int click);
#endif
#if defined(WIN32) || defined(WIN64)
	void postMouseEventWindows(int x, int y, int click);
#endif

	/////////////////////////////////////////////////////////////////////////
	////              ATTRIBUTES                                         ////
	/////////////////////////////////////////////////////////////////////////


	// These values are used to store some data on the algorithms process
	CvCapture *capture, *capture2;
	int xCenter, yCenter, xDelta, yDelta, step, scaleX, scaleY;
	vector<int> xValues;
	vector<int> yValues;
	vector<coordinate> vDistance;
	int threshold, choice;
	vector<coordinate> results;
	coordinate lastPoint, presentPoint, tryPoint;
	bool wantToClic, keyPress;

	int imgWidth, imgHeight, aproximation;
	bool flipVertical, Blur;
	CvBGStatModel* bg_model;
	int captureFPS, captureHeight, captureWidth;
	int jumpValue;
	int deep;
	int spiralTentatives, spiralSuceeded;
	coordinate references[4], projection[4];

	int countRightClic;
	bool rightClic;

	// Screen Resolution of your laptop
	int screenWidth;
	int screenHeight;

	// Input-Output
	bool circle, report, move, calibrate, difer, haveCalibrate, restartBackground, Quit, haveCalibrateScreen,adpatativeBackground;
	int keyPressValue;
	// Scale
	CvPoint2D32f srcQuad[4], dstQuad[4];
	CvMat * mat_trf;

	// Test Value to restart Background
	float metricValue,tempMetricValue;
	vector<float> vectorMetricValue;
	// Coordinates to test
	vector<long> testingCoordinates;

	//TUIO ID BLOB
#ifdef USING_OSC_TBETA
	OSCApp app;
	int blobID;
#endif /* USING_OSC_TBETA */

	//Frame for metric
	IplImage* workingMetricFrame;

	// Thread values
#if defined(WIN32) || defined(WIN64)
	HANDLE hThread;
	HANDLE ioThread;
#else
	pthread_t hThread;
	pthread_t ioThread;

#endif

};

#endif /* INCLUDED_LOCOBOARD_H */
