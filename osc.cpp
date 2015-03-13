/*
 * File:   osc.cpp
 * Author: Christophe Soares
 *
 * Edited on 6 de Outubro de 2009, 19:46, from oscpack
 * Copyright 2009 . All rights reserved.
 */

#include "osc.h"

OSCApp::OSCApp() {
	transmitSocket = NULL;
	frameSeq = 0;
	connectSocket();
}

OSCApp::~OSCApp() {
	delete transmitSocket;
}

// Set IP and Port with commandline parameters
void OSCApp::connectSocket() {
	std::string ip_address = ADDRESS;
	int port = PORT;

	transmitSocket = new UdpTransmitSocket(IpEndpointName(ip_address.c_str(),
			port));
	printf("Socket Initialized : %s Port : %i\n\n", ip_address.c_str(), port);
	frameSeq = 0;
}

// Set IP and Port with commandline parameters
void OSCApp::connectSocket(std::string ip_address, int port) {
	transmitSocket = new UdpTransmitSocket(IpEndpointName(ip_address.c_str(),
			port));
	printf("Socket Initialized : %s Port : %i\n\n", ip_address.c_str(), port);
	frameSeq = 0;
}

//! Notify that a finger has just been made active.
void OSCApp::fingerDown(TouchData data) {
	empty = false;
	if (!(data.X == 0.00 && data.Y == 0.00)) {
		activeFinger = data;
		printf("Blob Detected | X: %f Y: %f Area: %f Weight: %f\n", data.X,
				data.Y, data.area, data.weight);
	}

}

//! Notify that a finger has moved
void OSCApp::fingerUpdate(TouchData data) {
	empty = false;
	activeFinger = data;
}

//! A finger is no longer active..
void OSCApp::fingerUp(TouchData data) {
	empty = true;
}

void OSCApp::frame() {
	if (!transmitSocket)
		return;

	// send update messages..

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

	TouchData temp;

	if (!empty) {

		temp = activeFinger;
		p.Clear();
		p << osc::BeginBundle();

		float m = sqrtf((temp.dX * temp.dX) + (temp.dY * temp.dY));

		if (!(temp.X == 0 && temp.Y == 0)) {
			if (temp.tagID == 0) {
				// don't send extra info
#ifdef OSC_STRICT
				p << osc::BeginMessage( "/tuio/2Dcur" ) << "set" << temp.ID << temp.X << temp.Y << temp.dX << temp.dY << m << osc::EndMessage;
#else
				p << osc::BeginMessage("/tuio/2Dcur") << "set" << temp.ID
						<< temp.X << temp.Y << temp.dX << temp.dY << m
						<< temp.width << temp.height << osc::EndMessage;
				//pressure
				//p << osc::BeginMessage( "/tuio/2Dcur" ) << "set" << temp.ID << temp.X << temp.Y << temp.dX << temp.dY << m << temp.width << temp.height << temp.weight << osc::EndMessage;

#endif
			}
		}

		p << osc::BeginMessage("/tuio/2Dcur");
		p << "alive";

		TouchData temp2 = activeFinger;
		if (temp2.tagID == 0) {
			if (!(temp2.X == 0 && temp2.Y == 0)) {
				p << temp2.ID;
			}
		}

		p << osc::EndMessage;
		p << osc::BeginMessage("/tuio/2Dcur") << "fseq" << frameSeq
				<< osc::EndMessage;
		p << osc::EndBundle;

		frameSeq++;
		if (p.IsReady())
			transmitSocket->Send(p.Data(), p.Size());

	}

	else {
		p.Clear();
		p << osc::BeginBundle();

		p << osc::BeginMessage("/tuio/2Dcur");
		p << "alive";
		p << osc::EndMessage;

		p << osc::BeginMessage("/tuio/2Dcur") << "fseq" << frameSeq
				<< osc::EndMessage;

		p << osc::EndBundle;

		frameSeq++;

		if (p.IsReady())
			transmitSocket->Send(p.Data(), p.Size());
	}

}

void OSCApp::frameMulti() {
	if (!transmitSocket)
		return;

	// send update messages..

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p(buffer, OUTPUT_BUFFER_SIZE);

	TouchData temp;

	if (!empty) {

		temp = activeFinger;

		p.Clear();
		p << osc::BeginBundle();

		for (int i = 0; i != 2; i++) {
			float m = sqrtf((temp.dX * temp.dX) + (temp.dY * temp.dY));

			if (!(temp.X == 0 && temp.Y == 0)) {
				if (temp.tagID == 0) {
					// don't send extra info
#ifdef OSC_STRICT
					p << osc::BeginMessage( "/tuio/2Dcur" ) << "set" << temp.ID << temp.X << temp.Y << temp.dX << temp.dY << m << osc::EndMessage;
#else
					p << osc::BeginMessage("/tuio/2Dcur") << "set" << temp.ID
							<< temp.X << temp.Y << temp.dX << temp.dY << m
							<< temp.width << temp.height << osc::EndMessage;
					//pressure
					//p << osc::BeginMessage( "/tuio/2Dcur" ) << "set" << temp.ID << temp.X << temp.Y << temp.dX << temp.dY << m << temp.width << temp.height << temp.weight << osc::EndMessage;

#endif
				}
			}
			temp = activeFinger2;
		}

		p << osc::BeginMessage("/tuio/2Dcur");
		p << "alive";

		TouchData temp2 = activeFinger;
		for (int i = 0; i != 2; i++) {
			if (temp2.tagID == 0) {
				if (!(temp2.X == 0 && temp2.Y == 0)) {
					p << temp2.ID;
				}
			}
			temp2 = activeFinger2;
		}

		p << osc::EndMessage;
		p << osc::BeginMessage("/tuio/2Dcur") << "fseq" << frameSeq
				<< osc::EndMessage;
		p << osc::EndBundle;

		frameSeq++;
		if (p.IsReady())
			transmitSocket->Send(p.Data(), p.Size());

	}

	else {
		p.Clear();
		p << osc::BeginBundle();

		p << osc::BeginMessage("/tuio/2Dcur");
		p << "alive";
		p << osc::EndMessage;

		p << osc::BeginMessage("/tuio/2Dcur") << "fseq" << frameSeq
				<< osc::EndMessage;

		p << osc::EndBundle;

		frameSeq++;

		if (p.IsReady())
			transmitSocket->Send(p.Data(), p.Size());
	}

}

void OSCApp::clearFingers() {
	empty = true;
}

void OSCApp::setActiveFinger(struct TouchData temp) {
	activeFinger = temp;
	empty = false;
}

void OSCApp::setActiveFinger(struct TouchData temp, struct TouchData temp2) {
	activeFinger = temp;
	activeFinger2 = temp2;
	empty = false;
}

struct TouchData OSCApp::getActiveFinger() {
	return activeFinger;
}

bool OSCApp::isEmpty() {
	return empty;
}

///////////////////////////////////////////////////////////////////////
// Main Function to test OSC                                         //
///////////////////////////////////////////////////////////////////////


/*

 int main(int argc, char * argv[])
{
	std::string ip_address = ADDRESS;
	int port = PORT;

	// Check if command arguments are specified
	if (argc == 3)
	{
		// Convert parsed values
		ip_address = argv[1];
		port = (int)strtol (argv[2], NULL, 0);
	}

	// Set ip and port
	app.connectSocket(ip_address, port);


	TouchData t1= {1,0,0,0,0,0,0,0,0,0,0};
	TouchData t2= {1,0,0.1,0.1,0,0,0,0,1,1,0};
	TouchData t3= {1,0,0.2,0.2,0,0,0,0,1,1,0};
	TouchData t4= {1,0,0.3,0.3,0,0,0,0,1,1,0};
	TouchData t5= {1,0,0.4,0.4,0,0,0,0,1,1,0};
	TouchData t6= {1,0,0.5,0.5,0,0,0,0,1,1,0};
	TouchData t7= {1,0,0.6,0.6,0,0,0,0,1,1,0};
	TouchData t8= {1,0,0.7,0.7,0,0,0,0,1,1,0};
	TouchData t9= {1,0,0.8,0.8,0,0,0,0,1,1,0};
	TouchData t10= {1,0,0.9,0.9,0,0,0,0,1,1,0};
	TouchData t11= {1,0,0.8,0.8,0,0,0,0,1,1,0};
	TouchData t12= {1,0,0.7,0.7,0,0,0,0,1,1,0};
	TouchData t13= {1,0,0.6,0.6,0,0,0,0,1,1,0};
	TouchData t14= {1,0,0.5,0.5,0,0,0,0,1,1,0};
	TouchData t15= {1,0,0.4,0.4,0,0,0,0,1,1,0};

	TouchData array []={t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14,t15};

	app.activeFinger=array[0];
	int number=0,size=14;


    cvNamedWindow( "Touch Listener", CV_WINDOW_AUTOSIZE );
	do
	{
		int keypressed = cvWaitKey(10) & 255;

		if(keypressed != 255 && keypressed > 0)
			printf("KP: %d\n", keypressed);

        if( keypressed == 27) break;		// ESC = quit


		if(number==size)
			number=0;
		app.activeFinger=array[number];
		number++;



		app.frame();

		sleep(1);




	} while( ok );

	return 0;
}

 */

///////////////////////////////////////////////////////////////////////

