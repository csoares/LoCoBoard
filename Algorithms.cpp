/*
 * File:   Algorithms.cpp
 * Author: Christophe Soares
 *
 * Created on 6 de Outubro de 2009, 19:46
 * Copyright 2009 . All rights reserved.
 */

#include "Algorithms.h"

///////////////////////////////////////////////////////////////////////
// Implementation of the LoCoBoad Methods                            //
///////////////////////////////////////////////////////////////////////

// Constructor
Algorithms::Algorithms() {
	Algorithms::xCenter = -1;
	Algorithms::yCenter = -1;
	Algorithms::threshold = 50;
	Algorithms::lastPoint.x = -1;
	Algorithms::lastPoint.y = -1;
	Algorithms::presentPoint.x = -1;
	Algorithms::presentPoint.y = -1;
	Algorithms::tryPoint.x = -1;
	Algorithms::tryPoint.y = -1;
	Algorithms::captureFPS = 1;
	Algorithms::captureHeight = 480;
	Algorithms::captureWidth = 640;
	Algorithms::wantToClic = false;
	Algorithms::keyPress = false;
	Algorithms::flipVertical = false;
	Algorithms::Blur = false;
	Algorithms::step = 1;
	Algorithms::deep = 0;

	Algorithms::choice = 3; // selects algorithm

	Algorithms::circle = true; // shape of POI draw
	Algorithms::report = false; // tuio communication
	Algorithms::move = true; //mouse control on/off
	Algorithms::calibrate = true;
	Algorithms::difer = true; // background subtraction enabled/disabled
	Algorithms::haveCalibrate = false;
	Algorithms::restartBackground = false;
	Algorithms::Quit = false;
	Algorithms::haveCalibrateScreen = false;
	Algorithms::adpatativeBackground = false;
}
// Constructor
Algorithms::Algorithms(int thresholdValue) {
	Algorithms();

	Algorithms::xCenter = -1;
	Algorithms::yCenter = -1;
	Algorithms::threshold = thresholdValue;

}

// Destructor
Algorithms::~Algorithms() {

	// Threads
#ifdef WIN32
	if(hThread)
	TerminateThread(hThread, 0);
#else
	if (hThread) {
		pthread_kill(hThread, 15);
		hThread = 0;
	}
#endif

	cvReleaseCapture(&capture);
	cvDestroyWindow("Whiteboard Process");

	if (debug) {
		cvDestroyWindow("View");
	}

}

// this method will allow us to define the algorithm to be used - it could be change in real time
void Algorithms::setAlgorithm(int value) {
	Algorithms::choice = value;
}

// Initializtion of the lookup table for the spiral process, max is the maximum value to the spiral deep
void Algorithms::distance(int max) {
	int seX = -1, seY = -1;
	int idX = 1, idY = 1;
	int i;
	int circles = 1;
	coordinate c = { 0, 0 };
	vDistance.push_back(c);

	//if(jump==0)
	//	jump=1;

	//printf("\n\nJUMP %d",jumpValue);

	do {
		//printf("\n\nCiclo numero: %d\n",circles);


		for (i = seX; i <= idX; i += jumpValue) {
			c.x = i;
			c.y = seY;
			vDistance.push_back(c);
		}

		for (i = seY + 1; i <= idY; i += jumpValue) {
			c.x = idX;
			c.y = i;
			vDistance.push_back(c);
		}

		for (i = idX - 1; i >= seX; i -= jumpValue) {
			c.x = i;
			c.y = idY;
			vDistance.push_back(c);
		}

		for (i = idY - 1; i > seY; i -= jumpValue) {
			c.x = seX;
			c.y = i;
			vDistance.push_back(c);
		}

		seX--;
		seY--;

		idX++;
		idY++;

		circles++;

	} while (circles <= max);
	//printf("\nSize %d\n", (int) vDistance.size());
}

// this method will allow to process the collection of value that will be used to calculate the MetricValue - this value will be use to set a reset of the background model
void Algorithms::setTestCoordinates(int height, int width) {
	// clean vector
	testingCoordinates.erase(testingCoordinates.begin(),
			testingCoordinates.end());

	int stepI = height / 11;
	int stepJ = width / 11;
	for (int i = stepI; i <= height - stepI; i = i + stepI) {
		for (int j = stepJ; j <= width - stepJ; j = j + stepJ) {
			long v = (j * captureHeight) + (i);
			testingCoordinates.push_back(v);
		}
	}

	//printf("\nSize testingCoordinates = %d", testingCoordinates.size());

}

// this method will print a circle in frame at the coordinate from center
void Algorithms::printCircle(coordinate center, IplImage* frame) {
	// para mostrar a zona onde foi encontrado
	if (center.x >= 0 && center.y >= 0) {
		CvPoint c;
		c.x = center.x;
		c.y = center.y;

		cvCircle(frame, c, 10, CV_RGB(255, 0, 0), CV_FILLED, CV_AA, 0);
	}
}

// this method will calculate the center from a collection of known values of PI
coordinate Algorithms::processCenter() {
	int xTemp = 0, yTemp = 0;
	coordinate center = { -1, -1, 0.0, 0 };
	if (xValues.size() == 0)
		return center;
	//puts("ProcessCenter");
	for (int i = 0; i < (int) xValues.size(); i++) {
		//printf("%d",i);
		xTemp = xTemp + (int) xValues.at(i);
		yTemp = yTemp + (int) yValues.at(i);
	}

	center.x = (int) (xTemp / xValues.size());
	//printf("%d",xCenter);
	center.y = (int) (yTemp / yValues.size());
	//printf("%d",yCenter);

	center.sizeRect = abs(((int) xValues.at(xValues.size() - 1)
			- (int) xValues.at(0)) * ((int) yValues.at(yValues.size() - 1)
			- (int) yValues.at(0)));

	return center;
}

// this method will calculate the center from a set of PI on a frame, temp is one of this set of PI
coordinate Algorithms::findCenter(coordinate temp, IplImage* frame) {
	coordinate center = { -1, -1, 0.0, 0 };

	int xMax = 0, xMin = 0, yMax = 0, yMin = 0;

	int pointer = (temp.y * frame->width + temp.x);

	for (int p = pointer; p < ((temp.y + 1) * frame->width - 1)
			&& ((unsigned char) frame->imageData[p]) >= threshold; xMax++, p++)
		;
	for (int p = pointer; p > ((temp.y) * frame->width)
			&& ((unsigned char) frame->imageData[p]) >= threshold; xMin--, p--)
		;

	int distance = xMax - xMin;
	center.x = temp.x + int(distance / 2);

	//adjust pointer
	pointer = (temp.y * frame->width + center.x);

	for (int p = pointer; p < (frame->height + center.x - 1)
			&& ((unsigned char) frame->imageData[p]) >= threshold; yMax++, p
			+= frame->width)
		;
	for (int p = pointer; p > (center.x)
			&& ((unsigned char) frame->imageData[p]) >= threshold; yMin--, p
			-= frame->width)
		;

	distance = yMax - yMin;
	center.y = temp.y + int(distance / 2);

	center.sizeRect = abs((xMax - xMin) * (yMax - yMin));

	return center;

}

// this method will clear all the values that have been collected in the process
void Algorithms::cleanVector() {
	//puts("CleanVector");
	if (xValues.size() != 0) {
		xValues.erase(xValues.begin(), xValues.end());
		yValues.erase(yValues.begin(), yValues.end());
	}

	//printf("\nClean vector xValues (%d) and yValues (%d)",(int) xValues.size(),(int) yValues.size());
}

// this method will clean the vectors that have been use to collect the PI values - ie blob with a intesity superior to the threshold value
void Algorithms::cleanVectorResults() {
	//puts("CleanVector");
	if (results.size() != 0) {
		results.erase(results.begin(), results.end());
	}
	//printf("\n\nClean VECTOR RESULTS (SIZE: %d)\n\n",results.size());
}

// this method will move the cursor of the OS to the coordinates x,y
void Algorithms::mouseMove(int x, int y) {
    if (x >= 0 && y >= 0) {
            #if defined(WIN32)
            SetCursorPos(x, y);
            #endif
            #ifdef LINUX
            QCursor::setPos(x, y);  // usse QT in Linux
            #endif
    }
}

// this method will check what is the current resolution on your PC - NOT IMPLEMENTED YET
void Algorithms::screenSize() {

	//int w = QApplication::desktop()->width();
	//int h = QApplication::desktop()->height();

	//printf("Screen Resolution x= %d y=%d", w, h);
}

// this method will be used to to convert a PI from a resolution to an other (INT)
int Algorithms::convertToResolution(int p, int escalaOrigem, int escalaDestino) {
	if (p == -1)
		return p;
	return (p * escalaDestino) / escalaOrigem;
}

// this method will be used to to convert a PI from a resolution to an other (FLOAT)
float Algorithms::convertToResolutionF(float p, float escalaOrigem,
		float escalaDestino) {
	if (p == -1)
		return p;
	return (p * escalaDestino) / escalaOrigem;
}

// this algorithm have been adapted from the Erik van Kempen one
vector<coordinate> Algorithms::detectBlobs(IplImage* frame,
		IplImage* finalFrame) {
	// http://geekblog.nl/entry/24
	// 28 outubro 2008
	int blobCounter = 0;
	map<unsigned int, blob> blobs;

	vector<vector<lineBlob> > imgData(frame->width);

	cleanVectorResults();

	for (int row = 0; row < frame->height; ++row) {

		for (int column = 0; column < frame->width; ++column) {

			//unsigned char byte = (unsigned char) imgStream.get();
			unsigned char byte = (unsigned char) frame->imageData[(row
					* frame->width) + column];

			if (byte >= threshold) {
				int start = column;

				for (; byte >= threshold; byte
						= (unsigned char) frame->imageData[(row * frame->width)
								+ column], ++column)
					;

				int stop = column - 1;
				lineBlob lineBlobData = { start, stop, blobCounter, false };

				imgData[row].push_back(lineBlobData);
				blobCounter++;
			}
		}
	}

	/* Check lineBlobs for a touching lineblob on the next row */

	for (int row = 0; row < (int) imgData.size(); ++row) {

		for (int entryLine1 = 0; entryLine1 < (int) imgData[row].size(); ++entryLine1) {

			for (int entryLine2 = 0; entryLine2 < (int) imgData[row + 1].size(); ++entryLine2) {

				if (!((imgData[row][entryLine1].max
						< imgData[row + 1][entryLine2].min)
						|| (imgData[row][entryLine1].min
								> imgData[row + 1][entryLine2].max))) {

					if (imgData[row + 1][entryLine2].attached == false) {

						imgData[row + 1][entryLine2].blobId
								= imgData[row][entryLine1].blobId;

						imgData[row + 1][entryLine2].attached = true;
					} else

					{
						imgData[row][entryLine1].blobId
								= imgData[row + 1][entryLine2].blobId;

						imgData[row][entryLine1].attached = true;
					}
				}
			}
		}
	}

	// Sort and group blobs

	for (int row = 0; row < (int) imgData.size(); ++row) {

		for (int entry = 0; entry < (int) imgData[row].size(); ++entry) {

			if (blobs.find(imgData[row][entry].blobId) == blobs.end()) // Blob does not exist yet

			{
				coordinate min = { imgData[row][entry].min, row };
				coordinate max = { imgData[row][entry].max, row };
				coordinate center = { 0, 0 };

				blob blobData = { min, max, center };

				blobs[imgData[row][entry].blobId] = blobData;
			} else

			{
				if (imgData[row][entry].min
						< blobs[imgData[row][entry].blobId].min.x)

					blobs[imgData[row][entry].blobId].min.x
							= imgData[row][entry].min;

				else if (imgData[row][entry].max
						> blobs[imgData[row][entry].blobId].max.x)

					blobs[imgData[row][entry].blobId].max.x
							= imgData[row][entry].max;

				if (row < (int) blobs[imgData[row][entry].blobId].min.y)

					blobs[imgData[row][entry].blobId].min.y = row;

				else if (row > (int) blobs[imgData[row][entry].blobId].max.y)

					blobs[imgData[row][entry].blobId].max.y = row;
			}
		}
	}

	// Calculate center
	for (map<unsigned int, blob>::iterator i = blobs.begin(); i != blobs.end(); ++i) {
		(*i).second.center.x = (*i).second.min.x + ((*i).second.max.x
				- (*i).second.min.x) / 2;
		(*i).second.center.y = (*i).second.min.y + ((*i).second.max.y
				- (*i).second.min.y) / 2;

		int size = ((*i).second.max.x - (*i).second.min.x) * ((*i).second.max.y
				- (*i).second.min.y);

		// Print coordinates on image, if it is large enough


		if (size > 50) {
			CvFont font;
			cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, CV_AA);

			char textBuffer[128];

			coordinate temp = { (*i).second.center.x, (*i).second.center.y };

			// Draw crosshair and print coordinates (just for debugging, not necessary for later multi-touch use)
			cvLine(finalFrame, cvPoint(temp.x - 5, temp.y), cvPoint(temp.x + 5,
					temp.y), cvScalar(0, 255, 0), 1);

			cvLine(finalFrame, cvPoint(temp.x, temp.y - 5), cvPoint(temp.x,
					temp.y + 5), cvScalar(0, 255, 0), 1);

			sprintf(textBuffer, "(%d, %d)", temp.x, temp.y);

			cvPutText(finalFrame, textBuffer, cvPoint(temp.x + 5, temp.y - 5),
					&font, cvScalar(0, 255, 0));

			cvRectangle(finalFrame, cvPoint((*i).second.min.x,
					(*i).second.min.y), cvPoint((*i).second.max.x,
					(*i).second.max.y), cvScalar(0, 255, 0), 1);

			results.push_back(temp);
		}
	}
	return results;
}

// this is algorithm A1 - linear search on all the frame
coordinate Algorithms::singlePointProcess(IplImage* workFrame) {
	cleanVector();

	for (int row = 0; row < workFrame->height; ++row) {
		for (int column = 0; column < workFrame->width; ++column) {
			//unsigned char byte = (unsigned char) workFrame->imageData[(row*workFrame->width)+ column];

			if (((unsigned char) workFrame->imageData[(row * workFrame->width)
					+ column]) >= threshold) {
				xValues.push_back(column);
				yValues.push_back(row);
				//printf("Ponto com informação x: %d y: %d \n", column, row);
			}
		}
	}

	coordinate center = { -1, -1 };

	if (xValues.size() > 0) {
		center = processCenter();
		center.intensity = ((unsigned char) workFrame->imageData[(center.y
				* workFrame->width) + center.x]);
	}

	return center;
}

// this is algorithm A2 - linear search on all the frame with a stepping in columns
coordinate Algorithms::singlePointProcess(IplImage* workFrame, int step) {
	cleanVector();

	for (int row = 0; row < workFrame->height; ++row) {
		for (int column = 0; column < workFrame->width; column += step) {
			//unsigned char byte = (unsigned char) workFrame->imageData[(row*workFrame->width)+ column];

			if (((unsigned char) workFrame->imageData[(row * workFrame->width)
					+ column]) >= threshold) {
				xValues.push_back(column);
				yValues.push_back(row);
				//printf("Ponto com informação x: %d y: %d \n", column, row);
			}
		}
	}

	coordinate center = { -1, -1 };

	if (xValues.size() > 0) {
		center = processCenter();
		center.intensity = ((unsigned char) workFrame->imageData[(center.y
				* workFrame->width) + center.x]);
	}

	return center;
}

// this is algorithm A1 - second version - use pointer increment instead of two for cicle
coordinate Algorithms::singlePointProcessPTR(IplImage* workFrame) {
	int max = (workFrame->height * workFrame->width) - 1;
	for (int count = 0; count < max; count++) {
		//unsigned char byte = (unsigned char) workFrame->imageData[count];

		if (((unsigned char) workFrame->imageData[count]) >= threshold) {
			int column = (int) (count % workFrame->width);
			int row = (int) (count / workFrame->width);

			coordinate temp = { column, row };

			coordinate center = findCenter(temp, workFrame);
			center.intensity = ((unsigned char) workFrame->imageData[(center.y
					* workFrame->width) + center.x]);

			//printf("Ponto com informação x: %d y: %d \n", center.x , center.y );

			return center;
			//printf("Ponto com informação x: %d y: %d \n", x, y);
		}
	}

	coordinate center = { -1, -1 };

	return center;
}

// this is algorithm A3 - it use a linear seach, this will be interupted each time it finds a PI. After it will use findCenter to process the center.
coordinate Algorithms::singlePointProcessPTR(IplImage* workFrame, int step) {
	int max = (workFrame->height * workFrame->width) - 1;
	for (int count = 0; count < max; count += step) {
		//unsigned char byte = (unsigned char) workFrame->imageData[count];

		if (((unsigned char) *(workFrame->imageData + count)) >= threshold) {
			int column = (int) (count % workFrame->width);
			int row = (int) (count / workFrame->width);

			coordinate temp = { column, row };

			coordinate center = findCenter(temp, workFrame);
			center.intensity = ((unsigned char) workFrame->imageData[(center.y
					* workFrame->width) + center.x]);

			//printf("Ponto com informação x: %d y: %d \n", center.x , center.y );

			return center;
			//printf("Ponto com informação x: %d y: %d \n", x, y);
		}
	}

	coordinate center = { -1, -1 };

	return center;
}

// part of the Algorithm A4 - first version - process jump spiral value in real time
coordinate Algorithms::spiral(IplImage* workFrame, int x, int y) {
	int seX = x - 1, seY = y - 1;
	int idX = x + 1, idY = y + 1;
	int i;
	int cicles = 1;
	coordinate c = { -1, -1 };

	bool keepon = true;

	do {
		//printf("\n\nCiclo numero: %d\n",cicles);

		//printf("SE (%d / %d)\n",seX,seY);
		//printf("ID (%d / %d)\n",idX,idY);


		for (i = seX; i <= idX && i <= workFrame->width && i >= 0; i += step) {
			//printf("Pesquisa na coordenada c(%d / %d)\n",i,seY);
			if ((unsigned char) workFrame->imageData[(seY * workFrame->width)
					+ i] >= threshold) {
				//puts("primeiro ciclo spiral");
				c.x = i;
				c.y = seY;
				return c;
			}
		}

		for (i = seY + 1; i <= idY && i <= workFrame->height && i >= 0; i
				+= step) {
			//printf("Pesquisa na coordenada c(%d / %d)\n",sdX,i);
			if ((unsigned char) workFrame->imageData[(i * workFrame->width)
					+ idX] >= threshold) {
				//puts("segundo ciclo spiral");
				c.x = idX;
				c.y = i;
				return c;
			}
		}

		for (i = idX - 1; i >= seX && i <= workFrame->width && i >= 0; i
				-= step) {
			//printf("Pesquisa na coordenada c(%d / %d)\n",i,idY);
			if ((unsigned char) workFrame->imageData[(idY * workFrame->width)
					+ i] >= threshold) {
				//puts("terceiro ciclo spiral");
				c.x = i;
				c.y = idY;
				return c;
			}
		}

		for (i = idY - 1; i > seY && i <= workFrame->height && i >= 0; i
				-= step) {
			//printf("Pesquisa na coordenada c(%d / %d)\n",ieX,i);
			if ((unsigned char) workFrame->imageData[(i * workFrame->width)
					+ seX] >= threshold) {
				//puts("quarto ciclo spiral");
				c.x = seX;
				c.y = i;
				return c;
			}
		}

		if (seX == 0 && seY == 0 && idX == workFrame->width && idY
				== workFrame->height)
			keepon = false;

		seX = seX > 0 ? seX - step : 0;
		seY = seY > 0 ? seY - step : 0;

		idX = idX < workFrame->width ? idX + step : workFrame->width;
		idY = idY < workFrame->height ? idY + step : workFrame->height;

		cicles++;
	} while (keepon);

	//puts("no PI !!\n");
	return c;
}

// part of the Algorithm A4 - second version - use a lookup table for the jump spiral value
coordinate Algorithms::spiral2(IplImage* workFrame, int x, int y) {
	coordinate temp = { -1, -1 };
	spiralTentatives++;
	for (int i = 0; i < (int) vDistance.size(); i++) {
		int pointer = (y + vDistance.at(i).y) * workFrame->width + (x
				+ vDistance.at(i).x);
		//printf("\nPOINTER %d\n",pointer);
		//unsigned char byte= (unsigned char) workFrame->imageData[pointer];
		if (((unsigned char) workFrame->imageData[pointer]) >= threshold) {
			//puts("\n\tEncontrou ponto na espiral !!");
			temp.x = (int) pointer % workFrame->width;
			temp.y = (int) pointer / workFrame->width;
			//printf("\t (%d,%d)\n",temp.x,temp.y);
			spiralSuceeded++;
			return temp;
		}
	}

	//puts("\n\tTentativa fora da espiral !!");
	temp = singlePointProcessPTR(workFrame, step);
	return temp;
}

// this is algorithm A4 - use some prediction between concurrent frames
coordinate Algorithms::smartSinglePointProcess(IplImage* workFrame) {

	//puts("\n\n---------------------\nStart - Smart\n------------------------\n\n");

	int xPrediction = 0, yPrediction = 0;
	coordinate center = { -1, -1 };
	coordinate temp = { -1, -1 };
	bool prevision = false;

	//printf("\n------------------------------------------------------------------------------------------------------------\nvalor do ponto anterior %d / %d\n",xCenter,yCenter);

	// se a previsão for fora dos limites da frame começar do centro
	// if((xCenter+xDelta)<=0 || (xCenter+xDelta)>workFrame->width)
	// {
	//	xPrediction=(workFrame->width/2);
	// }
	// else
	// {
	//	xPrediction=xCenter+xDelta;
	// }
	// if((yCenter+yDelta)<=0 || (yCenter+yDelta)>workFrame->height)
	// {
	//	yPrediction=(workFrame->height/2);
	// }
	// else
	// {
	//	yPrediction=yCenter+yDelta;
	// }

	//printf("spiral começa sobre o ponto estimado %d / %d\n",xPrediction,yPrediction);
	//printf("valor de delta %d / %d\n",xDelta,yDelta);


	//coordinate temp=spiral(workFrame,xPrediction, yPrediction);

	if (presentPoint.x != -1 && presentPoint.y != -1 && lastPoint.x != -1
			&& lastPoint.y != -1) {
		xDelta = (presentPoint.x - lastPoint.x);
		yDelta = (presentPoint.y - lastPoint.y);
		prevision = true;
	} else {
		xDelta = 0;
		yDelta = 0;
		prevision = false;
	}

	//printf("valor de delta %d / %d\n",xDelta,yDelta);

	if (prevision && presentPoint.x + xDelta <= workFrame->width
			&& presentPoint.y + yDelta <= workFrame->height) {
		xPrediction = presentPoint.x + xDelta;
		yPrediction = presentPoint.y + yDelta;
		// printf("\nHeight: %d  Width: %d\n",workFrame->height,workFrame->width);
		// printf("\nPrediction x: %d y:%d \n", xPrediction,yPrediction);
		temp = spiral2(workFrame, xPrediction, yPrediction);
	} else {
		// puts("Sem previsão");
		// metodo normal não ha previsão

		//puts("\n\n---------------------\nEnd 1 - Smart\n------------------------\n\n");

		return singlePointProcessPTR(workFrame, step);

	}

	if (temp.x != -1 && temp.y != -1) {

		//printf("\nSpiral encontrou este ponto TEMP: %d %d \n",temp.x,temp.y);

		// processar centro do PI
		center = findCenter(temp, workFrame);
		center.intensity = ((unsigned char) workFrame->imageData[(center.y
				* workFrame->width) + center.x]);

		//printf("\nCenter Value after findCenter temp (%d,%d)\n",center.x,center.y);

		// actualizar o delta
		//if((temp.x-xCenter)<=workFrame->width && (temp.y-yCenter)<=workFrame->height)
		//{
		//	xDelta=temp.x-xCenter;
		//	yDelta=temp.y-yCenter;
		//}

		//printf("novo valor de delta %d / %d\n------------------------------------------------------------------------------------------------------------\n\n\n\n\n\n\n\n\n\n\n",xDelta,yDelta);
		//getchar();

		// guardar nova posição do PI
		//center.x=temp.x;
		//center.y=temp.y;

	}

	//puts("\n\n---------------------\nEnd 2 - Smart\n------------------------\n\n");

	return center;

}

// this method allow to report on the console line the values of PI
void Algorithms::reportCoordinate(coordinate temp) {
	if (temp.x != -1)
		printf("\nTEMP INFO X: %d Y: %d SIZE: %f INTENSITY:%d\n", temp.x,
				temp.y, temp.sizeRect, temp.intensity);
}

// use this method to initialize the Classe Algorithms
void Algorithms::initialize(int argc, char *argv[]) {
	capture = 0;
	if (cam) {
		if (camQUANTITY == 1) {
			capture = cvCreateCameraCapture(CV_CAP_ANY);
		} else {
			capture = cvCaptureFromCAM(-1);
			capture2 = cvCaptureFromCAM(-1);
		}
	} else {
		// começa os testes a partir do mesmo film
		if (multiPoint)
			capture = cvCaptureFromAVI("../../videos/Filme2.avi");
		else
			capture = cvCaptureFromAVI("../../videos/3.avi");
	}
	if (!capture) {
		fprintf(stderr, "Could not initialize capturing...\n");
		exit(-1);
	}

	// initialize precache for spiral
	if (deep != 0)
		distance(deep);
	else
		distance(DEEP);


	spiralTentatives = 0;
	spiralSuceeded = 0;

#ifdef __WIN32__
    /*
    captureWidth = 0;
	captureHeight = 0;

    if(cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH,captureWidth))
    printf("Cant set width property\n");

    if(cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, captureHeight))
    printf("Cant set height property\n");

    if(cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, captureFPS))
    printf("Cant set FPS\n");
    */
#endif

	IplImage* frame = cvQueryFrame(capture);

	if (!frame) {
		printf("\nProgram Fail in image acquisition! bad video (Initialize Method) \n");
		exit(0);
	}

	captureHeight = frame->height;
	captureWidth = frame->width;

	// initialize precache for metric value process
	setTestCoordinates(captureHeight, captureWidth);

	// Background subtraction
	if (bgdMODEL == 1) {
		bg_model = cvCreateGaussianBGModel(frame);
	} else {
		bg_model = cvCreateFGDStatModel(frame);
	}

	showCaptureProperties(capture);

	// TUIO PROTOCOL
#ifdef USING_OSC_TBETA
	blobID = 1;
#endif /* USING_OSC_TBETA */

	puts("These are the comands to control the runtime aplication: \n\n");
	puts("\tPress Escape to quit\n");
	puts("\tPress Space to Change the process image algorithm\n");
	puts("\tPress \"C\" to print a circle on the estimate Point of interest\n");
	puts("\tPress \"M\" to start/stop the application control your mouse\n ");
	puts("\tPress \"R\" to the aplication start reporting the coordinate of the Interest Point\n ");
	puts("\tPress \"+\" or \"-\" to control the threshold value\n ");
	puts("\tPress Backspace to reinitialize the Background Model\n\n ");
	puts("Background\n\n ");
	puts("\tPress \"B\" to calibrate the background\n ");
	puts("\tPress \"D\" to start/stop Background subtraction process\n ");
	puts("\tPress \"A\" to start/stop adpatative Background\n ");

}


// Threads to capture the input keyboard
THREAD_RETURN_TYPE Algorithms::_threadGUI_Function(void * obj) {
	((Algorithms *) obj)->threadGUI_MainLoop();
	//return NULL;
}

// use this method after initialize to start the threads Input/Output and Capture Video
void Algorithms::startProcessingMain() {
	cvNamedWindow("Whiteboard Process", CV_WINDOW_AUTOSIZE);
	cvCreateTrackbar("Threshold", "Whiteboard Process", &threshold, 255, NULL);
	//cvCreateTrackbar("Step","Whiteboard Process",&step,20,NULL);


	if (debug) {
		cvNamedWindow("View", CV_WINDOW_AUTOSIZE);
	}

	//int fps=0;
	printf("\nSTART - %d\n", choice);
	printTime();

#if defined(WIN32) || defined(WIN64)
	ioThread = (HANDLE)_beginthread(_threadGUI_Function, 0, this);
	threadVideoProcessing_MainLoop();
#else
	pthread_create(&ioThread, NULL, _threadGUI_Function, this);
	threadVideoProcessing_MainLoop();

	void *status;
	pthread_join(ioThread, &status);
#endif
	puts("\nEND\n");
	printTime();
	//printf("\nTotal frame processed %d\n", i);
	if (spiralTentatives)
		printf("Spiral analysis\n\tTry: %d\tSucess: %d (%d)\tFailed %d (%d)",
				(int) (spiralTentatives), (int) spiralSuceeded, (spiralSuceeded
						* 100) / spiralTentatives, spiralTentatives
						- spiralSuceeded, 100 - ((spiralSuceeded * 100)
						/ spiralTentatives));

}

// this method will manage the press keyboard characters to take some action
void Algorithms::threadGUI_MainLoop() {
	//SLEEP(500);

	for (; !Quit;) {
		//puts("THREAD IO");
		//keyPress= cvWaitKey(50);

		// escape
		if (keyPressValue == 27 || keyPressValue == 'q' || keyPressValue == 'Q')
			Algorithms::Quit = true;

		// space
		if (keyPressValue == ' ') {
			if (choice < 5)
				choice++;
			else
				choice = 1;

			printf("\n\nNew Algorithm: %d\n\n", choice);

		}
		// backspace
		if (keyPressValue == 8) {
			puts("\nRestart background\n");
			Algorithms::restartBackground = true;
		}
		// a or A
		if (keyPressValue == 'a' || keyPressValue == 'A') {
			puts("\nAdaptative Background\n");
			Algorithms::adpatativeBackground
					= !Algorithms::adpatativeBackground;
		}

		// b or B
		if (keyPressValue == 'b' || keyPressValue == 'B') {
			puts("\nCalibrate\n");
			Algorithms::calibrate = true;
		}

		// c or C
		if (keyPressValue == 'c' || keyPressValue == 'C') {
			Algorithms::circle = !Algorithms::circle;
			printf("\nCircle %s\n", circle ? "on" : "off");

		}

		// d or D
		if (keyPressValue == 'd' || keyPressValue == 'D') {
			Algorithms::difer = !Algorithms::difer;
			printf("\nDifer %s\n", difer ? "on" : "off");
		}

		// m or M
		if (keyPressValue == 'm' || keyPressValue == 'M') {
			Algorithms::move = !Algorithms::move;
			printf("\nMove %s\n", move ? "on" : "off");
		}

		// r or R
		if (keyPressValue == 'r' || keyPressValue == 'R') {
			Algorithms::report = !Algorithms::report;
			printf("\nReport %s\n", report ? "on" : "off");
		}

		// f or F
		if (keyPressValue == 'f' || keyPressValue == 'F') {
			Algorithms::flipVertical = !Algorithms::flipVertical;
			printf("\nFLIP %s\n", flipVertical ? "on" : "off");
		}

		// +
		if (keyPressValue == '+') {
			threshold++;
			printf("\n\nNew threshold value: %d\n\n", threshold);
		}

		// -
		if (keyPressValue == '-') {
			threshold--;
			printf("\n\nNew threshold value: %d\n\n", threshold);

		}

		keyPressValue = -1;

		if (Algorithms::adpatativeBackground) {
			//puts("\nAdaptative Background\n");

			// Metric Value
			tempMetricValue = returnMetricValue(workingMetricFrame);
			float backgroundDelta = (float) tempMetricValue - metricValue;
			if (backgroundDelta >= 50) {

				if (bg_model != NULL)
					cvReleaseBGStatModel(&bg_model);

				Algorithms::restartBackground = true;
				puts("\nReset Background\n");

			}
			metricValue = tempMetricValue;
			printf("\nMetric Value: %f\n", backgroundDelta);
		}
	}
}

// this method will be the main loop of collecting images and process them with the chosen algorithm
// Threads  to control the video capture & Algorithms
coordinate Algorithms::threadVideoProcessing_MainLoop() {

	coordinate temp = { -1, -1, 0.0, 0 };
	//int i=0;

	for (; !Quit;) {
		//printf("\n%d\n",i);
		//puts("THREAD CAM");

		// Use this image to Ground Truth Avaliation
		// IplImage* frame =cvLoadImage("../../images/losango 217x220.png");


		IplImage* frame = cvQueryFrame(capture);

		if (!frame) {
			fprintf(stderr,
					"Error: frame is null in Process Method - Program will close now !\n");
			getchar();
			exit(-1);
		}

		// calc will be process in workFrame
		IplImage* workFrame = NULL;
		IplImage* finalFrame = NULL;

		workFrame = cvCreateImage(cvSize(frame->width, frame->height),
				IPL_DEPTH_8U, 1);
		finalFrame = cvCloneImage(frame);

		if (flipVertical) {
			cvFlip(finalFrame, finalFrame, 1);
		}

		// blur image
		if (Blur) {
			cvSmooth(finalFrame, finalFrame, CV_BLUR);
		}

		if (restartBackground) {
			// Background subtraction
			if (bgdMODEL == 1) {

				bg_model = cvCreateGaussianBGModel(finalFrame);
			} else {

				bg_model = cvCreateFGDStatModel(finalFrame);
			}
			restartBackground = false;
		}

		if (calibrate) {
			puts("Start Calibration Process");
			haveCalibrate = true;

			for (int i = 0; i < 5; i++, frame = cvQueryFrame(capture)) {
				// bacground subtraction
				cvUpdateBGStatModel(frame, bg_model);
				calibrate = false;
			}
			puts("Finish Calibration Process");
		}

		if (difer && haveCalibrate) {
			cvAbsDiff(finalFrame, bg_model->background, finalFrame);
			//cvThreshold(finalFrame, finalFrame, int((2*threshold)/3), 255, CV_THRESH_BINARY);
			cvUpdateBGStatModel(finalFrame, bg_model);
		}

		// grayscale
		cvCvtColor(finalFrame, workFrame, CV_BGR2GRAY);

		if (Algorithms::adpatativeBackground) {
			workingMetricFrame = cvCloneImage(workFrame);
		}

		switch (choice) {
		case 1:
			temp = singlePointProcess(workFrame);
			break;

		case 2:
			temp = singlePointProcess(workFrame, step);
			break;

		case 3:
			temp = singlePointProcessPTR(workFrame, step);
			break;

		case 4:
			temp = smartSinglePointProcess(workFrame);
			break;

		case 5:
			results = detectBlobs(workFrame, finalFrame);
			break;

		default:
			temp = singlePointProcessPTR(workFrame, step);
			break;
		}

		if (choice == 5 && !multiPoint) {
			if (results.size() > 0)
				temp = results.at(0);
			else {
				temp.x = -1;
				temp.y = -1;
			}
		}

		//printf("\n\nSIZE RESULTS %d\n\n",results.size());

		if (!multiPoint) {
			if (report)
				reportCoordinate(temp);
			if (circle && choice != 5)
				printCircle(temp, finalFrame);
			if (move) {

				if (PROJECTION) {
					if (!haveCalibrateScreen)
						startCalibrate();

					temp = convertToScale(temp);

				} else {
					temp.x = convertToResolution(temp.x, workFrame->width,
							screenWidth);
					temp.y = convertToResolution(temp.y, workFrame->height,
							screenHeight);
				}

				lastPoint = presentPoint;
				presentPoint = temp;

				mouseMove(temp.x, temp.y);
				interpreter();

			} else {

				if (PROJECTION) {
					if (!haveCalibrateScreen)
						startCalibrate();

					temp = convertToScale(temp);

				} else {
					temp.x = convertToResolution(temp.x, workFrame->width,
							screenWidth);
					temp.y = convertToResolution(temp.y, workFrame->height,
							screenHeight);
				}
			}
		} else {
			// Multipoint for 2 points
			if (results.size() == 0) {
				temp.x = -1;
				temp.y = -1;

				cleanVectorResults();
				//results.push_back(temp);
			}
			if (results.size() >= 2) {
				coordinate t1 = results.at(0);
				coordinate t2 = results.at(1);

				if (PROJECTION) {
					if (!haveCalibrateScreen)
						startCalibrate();

					t1 = convertToScale(t1);
					t2 = convertToScale(t2);

				} else {

					t1.x = convertToResolution(t1.x, workFrame->width,
							screenWidth);
					t1.y = convertToResolution(t1.y, workFrame->height,
							screenHeight);
					t2.x = convertToResolution(t2.x, workFrame->width,
							screenWidth);
					t2.y = convertToResolution(t2.y, workFrame->height,
							screenHeight);

				}

				cleanVectorResults();
				results.push_back(t1);
				results.push_back(t2);

			}
		}

		if (debug) {
			// Print Calibration Points
			for (int i = 0; i <= 3; i++)
				printCircle(projection[i], finalFrame);
		}

		cvShowImage("Whiteboard Process", finalFrame);

		if (debug) {
			cvShowImage("View", frame);
		}

		cvReleaseImage(&workFrame);
		cvReleaseImage(&finalFrame);

		//fps= (int) cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);
		//printf("\n\nFPS: %d", fps);


		// TUIO REPORT
        #ifdef USING_OSC_TBETA
		if (!multiPoint) {
			float X = 0.0, Y = 0.0, dX = 0.0, dY = 0.0;

			if (temp.x != -1 && temp.y != -1) {
				X = convertToResolutionF(temp.x, screenWidth, 1.0);
				Y = convertToResolutionF(temp.y, screenHeight, 1.0);
				//printf("\nX: %f\n",X);
				//printf("\nY: %f\n",Y);
				TouchData activeBlob = { blobID, 0, X, Y, 0, 0, 0, 0, dX, dY, 0 };
				app.setActiveFinger(activeBlob);
				//printf("\nActive Finger BlobID: %d\n",blobID);

			} else {
				app.clearFingers();
				//  puts("\nInactive Finger\n");
			}

			app.frame();

		} else {
			float X = 0.0, Y = 0.0, dX = 0.0, dY = 0.0;
			bool single = true;

			if (results.size() != 0) {

				temp = results.at(0);
				X = convertToResolutionF(temp.x, screenWidth, 1.0);
				Y = convertToResolutionF(temp.y, screenHeight, 1.0);

				//printf("\nX: %f\n",X);
				//printf("\nY: %f\n",Y);

				TouchData activeBlob = { 1, 0, X, Y, 0, 0, 0, 0, dX, dY, 0 };
				if (results.size() != 1) {
					temp = results.at(1);
					X = convertToResolutionF(temp.x, screenWidth, 1.0);
					Y = convertToResolutionF(temp.y, screenHeight, 1.0);

					//printf("\nX: %f\n",X);
					//printf("\nY: %f\n",Y);

					TouchData activeBlob2 = { 2, 0, X, Y, 0, 0, 0, 0, dX, dY, 0 };

					app.setActiveFinger(activeBlob, activeBlob2);
					single = false;
					//printf("\nMulti Active Finger\n");

				} else {
					app.setActiveFinger(activeBlob);
					//printf("\nActive Finger\n");
				}

			} else {
				app.clearFingers();
				//  puts("\nInactive Finger\n");
			}
			if (single)
				app.frame();
			else
				app.frameMulti();
		}
		#endif /* USING_OSC_TBETA */

		//printf("keyPressValue before %d\n\n", keyPressValue);
		if(keyPressValue == -1)
			keyPressValue = cvWaitKey(100);
		else
			cvWaitKey(100);
		//printf("keyPressValue after %d\n\n", keyPressValue);
		//SLEEP(200);
	}

	return temp;
}

// this method will calculate a metric value based on the coordinates
float Algorithms::returnMetricValue(IplImage * frame) {
	float temp = 0.0;
	for (int i = 0; i < (int) testingCoordinates.size(); i++) {
		temp = temp + ((unsigned char) (frame->imageData[testingCoordinates.at(
				i)]));
	}
	//printf("\nTemp: %f\n",temp);
	return (temp / testingCoordinates.size());
}

// this method will emulate the mouse function based on the detected PI
void Algorithms::interpreter() {

	aproximation = 5;

	if (keyPress) {

		if (presentPoint.x != -1 && presentPoint.y != -1) {

			// Is this point permanent light up in the same place ?
			if ((tryPoint.x + aproximation) >= presentPoint.x && (tryPoint.x
					- aproximation) <= presentPoint.x && (tryPoint.y
					+ aproximation) >= presentPoint.y && (tryPoint.y
					- aproximation) <= presentPoint.y)
				countRightClic++;

#ifdef __APPLE__
			postMouseEventApple(presentPoint.x, presentPoint.y, 1);
#endif
#ifdef LINUX
			postMouseEventLinux(presentPoint.x,presentPoint.y,1);
#endif
#if defined(WIN32) || defined(WIN64)
			postMouseEventWindows(presentPoint.x,presentPoint.y,1);
#endif

			puts("\nKey Press Move");
		} else {
#ifdef __APPLE__
			postMouseEventApple(lastPoint.x, lastPoint.y, 0);
#endif
#ifdef LINUX
			postMouseEventLinux(lastPoint.x,lastPoint.y,0);
#endif
#if defined(WIN32) || defined(WIN64)
			postMouseEventWindows(lastPoint.x,lastPoint.y,0);
#endif

			keyPress = false;

			puts("\nKey Up");

#ifdef USING_OSC_TBETA
			blobID++;
#endif /* USING_OSC_TBETA */
		}

	} else {
		// find a right clic
		if (wantToClic && (tryPoint.x + aproximation) >= presentPoint.x
				&& (tryPoint.x - aproximation) <= presentPoint.x && (tryPoint.y
				+ aproximation) >= presentPoint.y
				&& (tryPoint.y - aproximation) <= presentPoint.y) {
			wantToClic = false;
			// emular o clic direito
#ifdef __APPLE__
			postMouseEventApple(presentPoint.x, presentPoint.y, 1);
#endif
#ifdef LINUX
			postMouseEventLinux(presentPoint.x,presentPoint.y,1);
#endif
#if defined(WIN32) || defined(WIN64)
			postMouseEventWindows(presentPoint.x,presentPoint.y,1);
#endif

			keyPress = true;

			puts("\nKey press");

		}

		if (lastPoint.x == -1 && lastPoint.y == -1 && presentPoint.x != -1
				&& presentPoint.y != -1) {
			tryPoint.x = presentPoint.x;
			tryPoint.y = presentPoint.y;
			wantToClic = true;

			keyPress = false;

			puts("\nWant to clic");
		}

	}
	if (countRightClic == 5) {
		rightClic = true;
	}

	if (rightClic) {
		if (presentPoint.x != -1 && presentPoint.y != -1) {
#ifdef __APPLE__
			postMouseEventApple(presentPoint.x, presentPoint.y, 0);
			postMouseEventApple(presentPoint.x, presentPoint.y, 1);
			postMouseEventApple(presentPoint.x, presentPoint.y, 0);
			postMouseEventApple(presentPoint.x, presentPoint.y, 1);
			postMouseEventApple(presentPoint.x, presentPoint.y, 0);
#endif
#ifdef LINUX
			postMouseEventLinux(presentPoint.x,presentPoint.y,0);
			postMouseEventLinux(presentPoint.x,presentPoint.y,1);
			postMouseEventLinux(presentPoint.x,presentPoint.y,0);
			postMouseEventLinux(presentPoint.x,presentPoint.y,1);
			postMouseEventLinux(presentPoint.x,presentPoint.y,0);
#endif
#if defined(WIN32) || defined(WIN64)
			postMouseEventWindows(presentPoint.x,presentPoint.y,0);
			postMouseEventWindows(presentPoint.x,presentPoint.y,1);
			postMouseEventWindows(presentPoint.x,presentPoint.y,0);
			postMouseEventWindows(presentPoint.x,presentPoint.y,1);
			postMouseEventWindows(presentPoint.x,presentPoint.y,0);
#endif

		} else {
#ifdef __APPLE__
			postMouseEventApple(lastPoint.x, lastPoint.y, 0);
			postMouseEventApple(lastPoint.x, lastPoint.y, 1);
			postMouseEventApple(lastPoint.x, lastPoint.y, 0);
			postMouseEventApple(lastPoint.x, lastPoint.y, 1);
			postMouseEventApple(lastPoint.x, lastPoint.y, 0);
#endif
#ifdef LINUX
			postMouseEventLinux(lastPoint.x,lastPoint.y,0);
			postMouseEventLinux(lastPoint.x,lastPoint.y,1);
			postMouseEventLinux(lastPoint.x,lastPoint.y,0);
			postMouseEventLinux(lastPoint.x,lastPoint.y,1);
			postMouseEventLinux(lastPoint.x,lastPoint.y,0);
#endif
#if defined(WIN32) || defined(WIN64)
			postMouseEventWindows(lastPoint.x,lastPoint.y,0);
			postMouseEventWindows(lastPoint.x,lastPoint.y,1);
			postMouseEventWindows(lastPoint.x,lastPoint.y,0);
			postMouseEventWindows(lastPoint.x,lastPoint.y,1);
			postMouseEventWindows(lastPoint.x,lastPoint.y,0);
#endif
		}

		countRightClic = 0;
		rightClic = false;
		keyPress = false;

		puts("Double right clic");

#ifdef USING_OSC_TBETA
		blobID++;
#endif /* USING_OSC_TBETA */
	}
}

// MOUSE EVENTS CLIC for APPLE
#ifdef __APPLE__
void Algorithms::postMouseEventApple(int x, int y, int width, int height,
		int click) {
	CGPoint pt;
	//CGEventRef eventRef=NULL;

	pt.x = x;
	pt.y = y;

	CGPostMouseEvent(pt, 1, 1, click);

	/*
	 if(click)
	 {
	 eventRef=CGEventCreateMouseEvent (NULL,kCGEventLeftMouseDown,pt,0);
	 CGEventPost (kCGHIDEventTap,eventRef);
	 CFRelease(eventRef);
	 }
	 else
	 {
	 eventRef=CGEventCreateMouseEvent (NULL,kCGEventLeftMouseUp,pt,0);
	 CGEventPost (kCGHIDEventTap,eventRef);
	 CFRelease(eventRef);
	 }
	 */
}

// MOUSE EVENTS CLIC for APPLE
void Algorithms::postMouseEventApple(int x, int y, int click) {
	CGPoint pt;

	pt.x = x;
	pt.y = y;

	CGPostMouseEvent(pt, 1, 1, click);

}
#endif

#if defined(WIN32) || defined(WIN64)
// MOUSE EVENTS CLIC for WIN
void Algorithms::postMouseEventWindows(int x, int y,int click)
{
	//http://msdn.microsoft.com/en-us/library/ms646310(VS.85).aspx#Mtps_DropDownFilterText
	//http://msdn.microsoft.com/en-us/library/ms646273(VS.85).aspx
	//Coordinate (0,0) maps onto the upper-left corner of the display surface; coordinate (65535,65535) maps onto the lower-right corner

	//INPUT input[1];
	//memset(input, 0, sizeof(input));
	//input[0].type = INPUT_MOUSE;

	//input[0].mi.dx = x * (65335/ScreenWidth)
	//input[0].mi.dy = y * (65335/ScreenHeight)
	//input[0].mi.dx = x;
	//input[0].mi.dy = y;

	//if(click)
	//	input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
	//else
	//	input[0].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	//input[0].mi.time = 0;
	//SendInput(1, input, sizeof(INPUT));

	mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, 0); //Click Down
	mouse_event(MOUSEEVENTF_LEFTUP, x, y, 0, 0); //Click Up

}
#endif

#ifdef LINUX
// MOUSE EVENTS CLIC for LINUX
void Algorithms::postMouseEventLinux(int x, int y,int click)
{
	Display *display = XOpenDisplay (0);
	XTestFakeMotionEvent (display, -1, x, y, 0);
	XTestFakeButtonEvent (display, 1, click, 0);
	XCloseDisplay (display);
}
#endif

// this method turn on the vertical FLIP on frame
void Algorithms::verticalFlipON() {
	Algorithms::flipVertical = true;
}

// this method turn off the vertical FLIP on frame
void Algorithms::verticalFlipOFF() {
	Algorithms::flipVertical = false;
}

// this method turn on the BLURR on frame
void Algorithms::blurON() {
	Algorithms::Blur = true;
}

// this method turn off the BLURR on frame
void Algorithms::blurOFF() {
	Algorithms::Blur = false;
}

// this method will allow to define the value of the stepping - ie the number of pixel to be ignored between each reading
void Algorithms::setStep(int value) {
	Algorithms::step = value;
}

// this method wiil allow to define the value of the stepping in the spiral movement
void Algorithms::setJumpSpiral(int value) {
	Algorithms::jumpValue = value;
}

// this method will allow us to define the deep of the spiral
void Algorithms::setSpiralDeep(int value) {
	Algorithms::deep = value;
}

// this method will print on the console line the current time of the system
void Algorithms::printTime() {
	time_t now;
	struct tm ts;
	char buf[80];

	// Get the current time
	time(&now);

	// Format and print the time, "hh:mm:ss "
	ts = *localtime(&now);
	strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
	printf("%s\n", buf);
}

// this method will print the lookup table of the spiral on the comand line
void Algorithms::printSpiralDistance() {
	puts("\nStart - Spiral distances\n");
	for (int i = 0; i < (int) vDistance.size(); i++) {
		printf("\n%d\tx: %d\ty: %d\n", i, vDistance.at(i).x, vDistance.at(i).y);
	}
	puts("\nEnd - Spiral distances\n");
}

// this method allow us to set some capture parameter to the camera - FPS / Width / Height
void Algorithms::setCapture(int captureFPS, int captureWidth, int captureHeight) {
#ifdef __WIN32__
	Algorithms::captureFPS=captureFPS;
	Algorithms::captureHeight=captureHeight;
	Algorithms::captureWidth=captureWidth;
#else
	printf("\nThis functionality only work in windows platform (setCapture).\n");
#endif
}

// this method will print on the console line the properties of the capture
void Algorithms::showCaptureProperties(CvCapture* cap) {
#ifdef __WIN32__
	int w,h,f;
	w=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FRAME_WIDTH);
	h=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FRAME_HEIGHT);
	f=(int)cvGetCaptureProperty(cap,CV_CAP_PROP_FPS);
	printf("Capture properties (widthxheight - frames): %dx%d - %d\n",w,h,f);
#else
	printf("\nThis functionality only work in windows platform (showCaptureProperties).\n");
#endif
}

// this method will start the calibration process to calculate the resolution of the projection
void Algorithms::startCalibrate() {
	cvNamedWindow("Calib", CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("DEBUG", CV_WINDOW_AUTOSIZE);

	/*

	 #ifdef __APPLE__
	 char * fileName= "./images/background800x600White.png";
	 //char * fileName="../../images/background800x600White.png";
	 #endif
	 #ifdef WIN32
	 char * fileName="C:\\projects\\christophe_v03\\images\\background.JPG";
	 #endif
	 IplImage* img = cvLoadImage(fileName);
	 if (!img)
	 printf("Could not load image file: %s\n", fileName);

	 */

	IplImage* img = cvCreateImage(cvSize(screenWidth, screenHeight),
			IPL_DEPTH_8U, 3);
	cvZero(img);

	IplImage * frame = cvQueryFrame(capture);

	cvMoveWindow("Calib", 0, 0);
	cvShowImage("Calib", img);
	imgHeight = img->height;
	//printf("\nImg Height : %d \n",imgHeight);
	imgWidth = img->width;
	//printf("\nImg Width : %d \n",imgWidth);

	int d = 50;

	printf("\n ScreenWidth : %d \t ScreenHeight : %d\n", screenWidth,
			screenHeight);
	cvResizeWindow("Calib", screenWidth, screenHeight);

	// Background subtraction
	if (bgdMODEL == 1) {
		bg_model = cvCreateGaussianBGModel(frame);
	} else {
		bg_model = cvCreateFGDStatModel(frame);
	}

	puts("Start Calibration Process");

	for (int i = 0; i < 5; i++, frame = cvQueryFrame(capture)) {
		// background subtraction
		if (flipVertical) {
			cvFlip(frame, frame, 1);
		}
		cvUpdateBGStatModel(frame, bg_model);
	}

	puts("Finish Calibration Process");

	if (captureWidth == 0) {
		captureWidth = frame->width;
		captureHeight = frame->height;
	}

	// Referecences points in the image background
	coordinate p1 = { d, d }, p2 = { imgWidth - d, d },
			p3 = { d, imgHeight - d }, p4 = { imgWidth - d, imgHeight - d };
	references[0] = p1;
	references[1] = p2;
	references[2] = p3;
	references[3] = p4;

	for (int i = 0; i < 4; i++) {
		//img = cvLoadImage(fileName);

		img = cvCreateImage(cvSize(screenWidth, screenHeight), IPL_DEPTH_8U, 3);
		cvZero(img);

		printCircle(references[i], img);
		cvShowImage("Calib", img);
		bool point = false;
		coordinate temp;
		while (!point) {
			frame = cvQueryFrame(capture);

			if (flipVertical) {
				cvFlip(frame, frame, 1);
			}

			IplImage* workFrame = NULL;

			cvAbsDiff(frame, bg_model->background, frame);
			//cvThreshold(frame, frame, int((2*threshold)/3),	255, CV_THRESH_BINARY);

			cvUpdateBGStatModel(frame, bg_model);

			workFrame = cvCreateImage(cvSize(frame->width, frame->height),
					IPL_DEPTH_8U, 1);
			cvCvtColor(frame, workFrame, CV_BGR2GRAY);

			temp = singlePointProcess(workFrame);

			if (temp.x == -1 || temp.y == -1) {
				keyPressValue = cvWaitKey(200);
			} else {
				if (temp.x > -1 && temp.y > -1 && temp.x < frame->width
						&& temp.y < frame->height && temp.x
						!= projection[i - 1].x && temp.y != projection[i - 1].y) {
					cvReleaseImage(&img);
					//img = cvLoadImage(fileName);
					//cvShowImage("Calib", img);

					cvReleaseImage(&workFrame);
					cout << '\a' << flush;
					point = true;
				}
			}

		}

		printf("\nCalib Point : %d\n", i + 1);
		projection[i] = temp;

		// Clean FRAME
		//img = cvLoadImage(fileName);

		img = cvCreateImage(cvSize(screenWidth, screenHeight), IPL_DEPTH_8U, 3);
		cvZero(img);

		cvShowImage("Calib", img);

		//printCircle(temp, frame);
		//cvShowImage("DEBUG",frame);

		// DELAY BETWEEN READS
		keyPressValue = cvWaitKey(0); //waits for a key to be pressed
	}

#ifdef __WIN32__
	cvDestroyWindow("Calib");
	cvReleaseImage(&img);
	puts("Destroy calibrate");
	//cvDestroyWindow("DEBUG");
#endif
#ifdef __APPLE__
	cvMoveWindow("Calib", screenHeight + d, screenWidth + d);
	cvMoveWindow("DEBUG", screenHeight + d, screenWidth + d);
#endif

	// Referecences assume screen resolution rectangle
	coordinate r1 = { d, d }, r2 = { screenWidth - d, d }, r3 = { d,
			screenHeight - d }, r4 = { screenWidth - d, screenHeight - d };
	references[0] = r1;
	references[1] = r2;
	references[2] = r3;
	references[3] = r4;

	printf(
			"\nRectangle Original [%d,%d][%d,%d][%d,%d][%d,%d]\nRectangle Projectado [%d,%d][%d,%d][%d,%d][%d,%d]\n",
			references[0].x, references[0].y, references[1].x, references[1].y,
			references[2].x, references[2].y, references[3].x, references[3].y,
			projection[0].x, projection[0].y, projection[1].x, projection[1].y,
			projection[2].x, projection[2].y, projection[3].x, projection[3].y);

	// set scale
	setScale();
}

// this method based on the calibration process will calculate a matrix to process the conversion
void Algorithms::setScale() {
	// initialize transfer matrix
	mat_trf = cvCreateMat(3, 3, CV_32FC1);

	srcQuad[0].x = projection[0].x;
	srcQuad[0].y = projection[0].y; // src Top Left
	srcQuad[1].x = projection[1].x;
	srcQuad[1].y = projection[1].y; // src Top Right
	srcQuad[2].x = projection[2].x;
	srcQuad[2].y = projection[2].y; // src Bottom Left
	srcQuad[3].x = projection[3].x;
	srcQuad[3].y = projection[3].y; // src Bottom Right

	dstQuad[0].x = references[0].x;
	dstQuad[0].y = references[0].y; // dst Top Left
	dstQuad[1].x = references[1].x;
	dstQuad[1].y = references[1].y; // src Top Right
	dstQuad[2].x = references[2].x;
	dstQuad[2].y = references[2].y; // src Bottom Left
	dstQuad[3].x = references[3].x;
	dstQuad[3].y = references[3].y; // src Bottom Rightset

	cvGetPerspectiveTransform(srcQuad, dstQuad, mat_trf);

	haveCalibrateScreen = true;

}

// this method based on the matrix will allow us to convert a PI from the projection scale to the scale of our laptop, ie resolution
coordinate Algorithms::convertToScale(coordinate temp) {
	if (temp.x == -1 && temp.y == -1)
		return temp;
	CvMat* src_point = cvCreateMat(1, 1, CV_32FC2);
	CvMat* dst_point = cvCreateMat(1, 1, CV_32FC2);

	// sample of transformation
	float *data_src = src_point->data.fl;
	float *data_dst = dst_point->data.fl;

	data_src[0] = temp.x;
	data_src[1] = temp.y; // src point (x,y)


	// transforma points
	cvPerspectiveTransform(src_point, dst_point, mat_trf);

	//printf("src(x=%f,y=%f) ->\t dst(x=%f,y=%f)\n",data_src[0],data_src[1],data_dst[0],data_dst[1]);

	temp.x = data_dst[0];
	temp.y = data_dst[1];
	return temp;
}

